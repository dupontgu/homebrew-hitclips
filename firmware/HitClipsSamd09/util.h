/*
 * util.h
 *
 * Created: 8/22/2021 10:40:04 AM
 *  Author: Guy Dupont
 */ 

#ifdef __cplusplus
extern "C" {
	#endif
#include "sam.h"

#ifndef UTIL_H_
#define UTIL_H_

#define PWM_PIN PORT_PA05
#define PWM_SELECT_PIN PORT_PA04
#define PLY_TGL_PIN PORT_PA02
#define PLY_TGL_INDEX PIN_PA02
#define PLY_TGL_GROUP 0
#define LED_PIN PORT_PA08
#define CS_PIN PORT_PA25 
#define MOSI_PIN PORT_PA24 
#define CK_PIN PORT_PA15 
#define MISO_PIN PORT_PA14 
#define MISO_GROUP 0
#define MISO_INDEX PIN_PA14

#define F_CPU 48000000
#define CYCLES_IN_DLYTICKS_FUNC        8
#define US_TO_DLYTICKS(ms)          (uint32_t)(F_CPU / 1000000 * ms / CYCLES_IN_DLYTICKS_FUNC) // ((float)(F_CPU)) / 1000.0
#define MS_TO_DLYTICKS(ms)          (uint32_t)(F_CPU / 1000 * ms / CYCLES_IN_DLYTICKS_FUNC) // ((float)(F_CPU)) / 1000.0
#define DelayTicks(ticks)            {volatile uint32_t n=ticks; while(n--);}//takes 8 cycles
#define DelayMs(ms)                    DelayTicks(MS_TO_DLYTICKS(ms))//uses 20bytes
#define DelayUs(us)                    DelayTicks(US_TO_DLYTICKS(us))

void die(int dly_time);
void setClockTo48();

#endif /* UTIL_H_ */

#ifdef __cplusplus
}
#endif