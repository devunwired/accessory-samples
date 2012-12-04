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
#define ADK_INTERNAL
#include "fwk.h"
#include "coop.h"
#include "RTC.h"



static volatile uint16_t gYear = 2012;
static volatile uint8_t gMonth = 2, gDay = 27, gHour = 19, gMinute = 8, gSecond = 8;


void rtcInit(void){

    SUPC->SUPC_CR = SUPC_CR_KEY(0xA5) | SUPC_CR_XTALSEL_CRYSTAL_SEL;	//enable oscillator
    RTC->RTC_IDR = 0x1f;
    RTC->RTC_IER = RTC_IER_SECEN; //enable interrupt
    NVIC_EnableIRQ(RTC_IRQn);
}

void RTC_Handler(void){

    static const uint8_t days_per_month[2][12] =	{
								{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
								{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
							};

    RTC->RTC_SCCR = RTC_SCCR_SECCLR;
    if(++gSecond < 60) return;
    gSecond = 0;
    if(++gMinute < 60) return;
    gMinute = 0;
    if(++gHour < 24) return;
    gHour = 0;
    if(++gDay < days_per_month[!(gYear & 3)][gMonth]) return;
    gDay = 0;
    if(gMonth++ < 12) return;
    gMonth = 0;
    gYear++;
}

void rtcGet(uint16_t* yearP, uint8_t* monthP, uint8_t* dayP, uint8_t* hourP, uint8_t* minuteP, uint8_t* secondP){

    uint16_t mYear;
    uint8_t mMonth, mDay, mHour, mMinute, mSecond;

    //read
    do{

        mSecond = gSecond;
        mMinute = gMinute;
        mHour = gHour;
        mDay = gDay;
        mMonth = gMonth;
        mYear = gYear;

    }while(mSecond != gSecond);

    if(yearP) *yearP = mYear;
    if(monthP) *monthP = mMonth + 1;
    if(dayP) *dayP = mDay + 1;
    if(hourP) *hourP = mHour;
    if(minuteP) *minuteP = mMinute;
    if(secondP) *secondP = mSecond;
}

void rtcSet(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second){

    //disable hardware update
    RTC->RTC_IDR = RTC_IDR_SECDIS;

    //write
    gYear = year;
    gMonth = month - 1;
    gDay = day - 1;
    gHour = hour;
    gMinute = minute;
    gSecond = second;

    //re-enable hardware RTC counting
    RTC->RTC_IER = RTC_IER_SECEN;
}



