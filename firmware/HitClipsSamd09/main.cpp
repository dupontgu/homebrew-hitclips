/*
* HitClipsSamd09.cpp
*
* Created: 8/21/2021 11:30:05 AM
* Author : Guy Dupont
*/

#include "sam.h"
#include "pff.h"
#include "diskio.h"
#include "util.h"

#define BUFF_SIZE 720

#define NONE_READ 1
#define READ_BUFF_L 2
#define READ_BUFF_R 4
#define DONE_READING 8

#define DD_GPIO_PORTA       0
#define DD_GPIO_PORTB       1
#define DD_GPIO_PORTC       2

#define PWM_MAX 128
#define PLAY_TGL_PRESSED ((REG_PORT_IN0 & PLY_TGL_PIN) == 0)

// Set the peripheral multiplexer for a pin
#define _GPIO_pmuxen_(port, pin, mux)	(PORT->Group[DD_GPIO_PORT##port].PINCFG[pin].reg |= PORT_PINCFG_PMUXEN);		\
if (pin & 1) { PORT->Group[DD_GPIO_PORT##port].PMUX[pin/2].bit.PMUXO = mux;		\
	} else {       PORT->Group[DD_GPIO_PORT##port].PMUX[pin/2].bit.PMUXE = mux;	}
	#define GPIO_pmuxen(...)				_GPIO_pmuxen_(__VA_ARGS__)

volatile uint8_t readBuffStatus = NONE_READ;
volatile uint8_t playBuffStatus = READ_BUFF_L;
volatile uint16_t readHead = 0;
volatile uint8_t throttle = 0;
volatile bool plyTglReset = false;
const uint16_t amplitude_mask = PWM_MAX - 1;
BYTE buffL[BUFF_SIZE], buffR[BUFF_SIZE];
BYTE *activeReadBuff, *activePlayBuff = buffR;

// Playback interrupt
// updates the duty cycle of the audio PWM to match the next sample of the sound file.
void TC2_Handler() {
	
	// throttle "adjusts" clock to match bit-rate of audio file
	if (++throttle < 3 || (readBuffStatus & (NONE_READ | DONE_READING))){
		REG_TC2_INTFLAG = TC_INTFLAG_OVF;
		return;
	}
	throttle = 0;
	
	uint16_t currentSample = (activePlayBuff[readHead++]);
	if (currentSample & PWM_MAX)
	{
		REG_PORT_OUTSET0 = PWM_SELECT_PIN;;
		REG_TC1_COUNT16_CC1 = (currentSample & amplitude_mask);
	} else {
		// If we swap decoder output pins from Y1 to Y0, can skip this subtraction
		REG_PORT_OUTCLR0 = PWM_SELECT_PIN;
		REG_TC1_COUNT16_CC1 = amplitude_mask - (currentSample & amplitude_mask);
	}
	
	if (readHead >= BUFF_SIZE) {
		if (playBuffStatus == READ_BUFF_R) {
			playBuffStatus = READ_BUFF_L;
			activePlayBuff = buffL;
		} else {
			playBuffStatus = READ_BUFF_R;
			activePlayBuff = buffR;
		}
		readHead = 0;
	}
	REG_TC2_INTFLAG = TC_INTFLAG_OVF;
	
}

void initPwm() {
	//setup clock for timer/counters
	REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TC1_TC2;
	REG_PM_APBCMASK |= PM_APBCMASK_TC1;
	REG_TC1_CTRLA |= TC_CTRLA_PRESCALER_DIV1;
	REG_TC1_CTRLA |= TC_CTRLA_MODE(TC_CTRLA_MODE_COUNT16_Val) | TC_CTRLA_WAVEGEN(TC_CTRLA_WAVEGEN_MPWM_Val);
	REG_TC1_COUNT16_CC1 = 0;
	REG_TC1_COUNT16_CC0 = PWM_MAX;
	GPIO_pmuxen(A, 5, 4);
	REG_TC1_CTRLA |= TC_CTRLA_ENABLE;
	while(TC1->COUNT16.STATUS.bit.SYNCBUSY==1);
}

void initPlaybackInterrupt() {
	//setup clock for timer/counters
	REG_PM_APBCMASK |= PM_APBCMASK_TC2;
	REG_TC2_CTRLA |=TC_CTRLA_PRESCALER_DIV4;	// prescaler: 8
	REG_TC2_INTENSET = TC_INTENSET_OVF;		// enable overflow interrupt
	REG_TC2_CTRLA |= TC_CTRLA_MODE(TC_CTRLA_MODE_COUNT8_Val) | TC_CTRLA_ENABLE;
	while(TC2->COUNT8.STATUS.bit.SYNCBUSY==1);	// wait on sync
	NVIC_EnableIRQ(TC2_IRQn);			// enable TC1 interrupt in the nested interrupt controller

}

void flushAudio() {
	for (int i = 0; i < BUFF_SIZE; i++) {
		buffL[i] = buffR[i] = 0;
	}
}

void readLoop(FATFS* fatfs) {
	UINT br;
	FRESULT rc = pf_open("audio.pcm");
	if (rc) die(3);
	
	// wait for user to hit the play button
	while(!PLAY_TGL_PRESSED){}
	while(true) {
		// wait until playback is not reading from the same buffer
		while(readBuffStatus == playBuffStatus) {}
		
		activeReadBuff = (readBuffStatus == READ_BUFF_R) ? buffR : buffL;
		rc = pf_read(activeReadBuff, BUFF_SIZE, &br);	/* Read a chunk of file */
		if (rc || !br) {
			readBuffStatus = DONE_READING;
			return;			
		} else {
			readBuffStatus = (readBuffStatus == READ_BUFF_R) ? READ_BUFF_L : READ_BUFF_R;
		}
		if (plyTglReset) {
			// user presses pause
			if(PLAY_TGL_PRESSED) {
				flushAudio();
				return;
			}
		} else {
			// flag if user has released the play button, so it can become the pause button
			plyTglReset = !PLAY_TGL_PRESSED;
		}
	}
}


int main(void)
{
	/* Initialize the SAM system */
	SystemInit();
	setClockTo48();
	initPwm();
	initPlaybackInterrupt();
	
	// output pins
	REG_PORT_DIRSET0 = LED_PIN | PWM_PIN | PWM_SELECT_PIN;
	
	// configure play toggle button as input with pullup
	PORT->Group[PLY_TGL_GROUP].PINCFG[PLY_TGL_INDEX].bit.INEN = 1;
	PORT->Group[PLY_TGL_GROUP].PINCFG[PLY_TGL_INDEX].bit.PULLEN = 1;
	REG_PORT_OUTSET0 = PLY_TGL_PIN;
	
	DelayMs(200);
	FRESULT rc = (FRESULT) disk_initialize();
	if (rc) die(2);
	
	FATFS fatfs;
	rc = pf_mount(&fatfs);
	if (rc) die(3);
	
	while (true)
	{
		readLoop(&fatfs);
		// what for user to release pause button if that's what caused music to end
		DelayMs(50);
		while(PLAY_TGL_PRESSED){}
		plyTglReset = false;
	}
}
