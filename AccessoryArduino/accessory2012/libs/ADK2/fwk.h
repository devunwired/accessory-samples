/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef ADK_INTERNAL
#ifndef _ADK_FWK_H_
#define _ADK_FWK_H_

#include <stdint.h>
#include <stdlib.h>

//stuff we need from Atmel's build system
	#define sam3x8
	#include "SAM3XA.h"
	void PMC_EnablePeripheral( uint32_t dwId );
	void SPI_Configure( Spi* spi, uint32_t dwId, uint32_t dwConfiguration );
	void SPI_ConfigureNPCS( Spi* spi, uint32_t dwNpcs, uint32_t dwConfiguration );
	void SPI_Enable( Spi* spi );
	void DACC_Initialize( Dacc* pDACC,uint8_t idDACC,uint8_t trgEn,uint8_t trgSel,uint8_t word,uint8_t sleepMode,uint32_t mck,uint8_t refresh,uint8_t user_sel, uint32_t tag_mode,uint32_t startup);
	uint32_t DACC_WriteBuffer( Dacc* pDACC, uint16_t *pwBuffer, uint32_t dwSize );
	#define DACC_EnableChannel(pDACC, channel)	(pDACC)->DACC_CHER = (1 << (channel));
	#define DACC_DisableChannel(pDACC, channel)	(pDACC)->DACC_CHDR = (1 << (channel));
	#define DACC_CHANNEL_0 0
	#define DACC_CHANNEL_1 1
	#define DACC_CfgModeReg(pDACC, mode)	(pDACC)->DACC_MR = (mode);
	#define DACC_SoftReset(pDACC)                 ((pDACC)->DACC_CR = DACC_CR_SWRST)
	void PWMC_ConfigureClocks(uint32_t clka, uint32_t clkb, uint32_t mck);
	void PWMC_ConfigureChannel(Pwm* pPwm,uint8_t channel,uint32_t prescaler,uint32_t alignment,uint32_t polarity);
	void PWMC_SetPeriod( Pwm* pPwm, uint8_t channel, uint16_t period);
	void PWMC_ConfigureComparisonUnit( Pwm* pPwm, uint32_t x, uint32_t value, uint32_t mode);
	void PWMC_ConfigureEventLineMode( Pwm* pPwm, uint32_t x, uint32_t mode);
	void PWMC_EnableChannel( Pwm* pPwm, uint8_t channel);


//common

	#define NULL	0

	#define BOARD_MCK 	84000000ULL
	#define TICKS_PER_MS	((BOARD_MCK) / 2000)
	#define ADK_HEAP_SZ	65536

	void fwkInit(void);
        uint64_t fwkGetUptime(void);
	uint64_t fwkGetTicks(void);
	void fwkDelay(uint64_t ticks);
	
	#define delay_s(s)	fwkDelay(TICKS_PER_MS * 1000ULL * (unsigned long long)(ms))
	#define delay_ms(ms)	fwkDelay(TICKS_PER_MS * (unsigned long long)(ms))
	#define delay_us(us)	fwkDelay((TICKS_PER_MS * (unsigned long long)(ms)) / 1000ULL)

	typedef void (*PeriodicFunc)(void* data);
	void periodicAdd(PeriodicFunc f, void* data, uint32_t periodMs);
	void periodicDel(PeriodicFunc f);

	void cpuGetUniqId(uint32_t* dst);	//produce the 128-bit unique ID

	#define DMA_CHANNEL_LEDS	0
	#define TIMER_FOR_ADK		0	// only in timer unit 0

	uint8_t getVolume(void);
	void setVolume(uint8_t);


//GPIO stuffs

	#define PORTA(x)	(x)
	#define PORTB(x)	((x) + 32)
	#define PORTC(x)	((x) + 64)
	#define PORTD(x)	((x) + 96)
	#define PORTE(x)	((x) + 128)
	#define PORTF(x)	((x) + 160)

	#define GPIO_FUNC_GPIO	0
	#define GPIO_FUNC_A	1
	#define GPIO_FUNC_B	2

	void gpioSetFun(uint8_t pin, uint8_t func);
	void gpioSetDir(uint8_t pin, char in);
	void gpioSetVal(uint8_t pin, char on);
        char gpioGetVal(uint8_t pin);
	void gpioSetPullup(uint8_t pin, char on);


#endif
#endif


