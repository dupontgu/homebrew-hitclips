/*
 * util.c
 *
 * Created: 8/22/2021 7:56:35 PM
 *  Author: Guy Dupont
 */ 

#include "util.h"

// stop execution and blink an LED to indicate the type of error
void die(int dly_time) {
	uint8_t i = 0;
	
	while (1) {
		NVIC_DisableIRQ(TC1_IRQn);
		for (i = 0; i < dly_time; i++)
		{
			// set PB1 high
			REG_PORT_OUTSET0 = LED_PIN;
			
			DelayMs(250);
			
			// set PB1 low
			REG_PORT_OUTCLR0 = LED_PIN;
			
			DelayMs(250);
		}
		DelayMs(1500);
		NVIC_EnableIRQ(TC1_IRQn);	
	}
	for (;;) ;
}

void setClockTo48() {
	/* Set the correct number of wait states for 48 MHz @ 3.3v */
	NVMCTRL->CTRLB.bit.RWS = 1;
	
	/* This works around a quirk in the hardware (errata 1.2.1) -
   the DFLLCTRL register must be manually reset to this value before
   configuration. */
	while(!SYSCTRL->PCLKSR.bit.DFLLRDY);
	SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE;
	while(!SYSCTRL->PCLKSR.bit.DFLLRDY);

	/* Write the coarse and fine calibration from NVM. */
	uint32_t coarse =
		((*(uint32_t*)FUSES_DFLL48M_COARSE_CAL_ADDR) & FUSES_DFLL48M_COARSE_CAL_Msk) >> FUSES_DFLL48M_COARSE_CAL_Pos;
	uint32_t fine =
		((*(uint32_t*)FUSES_DFLL48M_FINE_CAL_ADDR) & FUSES_DFLL48M_FINE_CAL_Msk) >> FUSES_DFLL48M_FINE_CAL_Pos;

	SYSCTRL->DFLLVAL.reg = SYSCTRL_DFLLVAL_COARSE(coarse) | SYSCTRL_DFLLVAL_FINE(fine);

	/* Wait for the write to finish. */
	while (!SYSCTRL->PCLKSR.bit.DFLLRDY) {};
		
	/* Enable the DFLL */
	SYSCTRL->DFLLCTRL.bit.ENABLE = 1;

	/* Wait for the write to finish */
	while (!SYSCTRL->PCLKSR.bit.DFLLRDY) {};
		
	/* Setup GCLK0 using the DFLL @ 48 MHz */
	GCLK->GENCTRL.reg =
		GCLK_GENCTRL_ID(0) |
		GCLK_GENCTRL_SRC_DFLL48M |
		/* Improve the duty cycle. */
		GCLK_GENCTRL_IDC |
		GCLK_GENCTRL_GENEN;

	/* Wait for the write to complete */
	while(GCLK->STATUS.bit.SYNCBUSY);
}
