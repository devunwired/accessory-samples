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
#include "Audio.h"
#include "coop.h"

#define DEFAULT_AUDIO_SAMPLERATE 44100

static uint32_t sampleRates[AUDIO_MAX_SOURCE];
static uint32_t audioActive; // bitmap of active audio sources

static int highestPriAudio(void)
{
    int i;

    if (audioActive == 0)
        return -1;

    return (31 - __builtin_clz(audioActive));
}

void audioInit(void)
{
    PMC_EnablePeripheral(ID_PWM);
    PWMC_ConfigureClocks(0, 0, BOARD_MCK);
    PWMC_ConfigureChannel(PWM, 0, PWM_CMR_CPRE_MCK, 0, 0);
    PWMC_ConfigureEventLineMode(PWM, 0, 1);

    audioSetSample(AUDIO_NULL, DEFAULT_AUDIO_SAMPLERATE);

    PWMC_EnableChannel(PWM, 0);
    PMC_EnablePeripheral(ID_DACC);
    DACC_Initialize(DACC, ID_DACC, 1, 4, 0, 0, BOARD_MCK, 8, DACC_CHANNEL_0, 0, 16 );
    DACC_EnableChannel(DACC, DACC_CHANNEL_0);
}

void audioOn(int source, uint32_t samplerate)
{
    audioActive |= (1 << source);

    audioSetSample(source, samplerate);
}

void audioOff(int source)
{
    int resetsample = 0;
    if (source == highestPriAudio())
        resetsample = 1;

    audioActive &= ~(1 << source);

    // switch to the new highest priority audio's sample rate
    int highest = highestPriAudio();
    if (resetsample && highest >= 0) {
        audioSetSample(highest, sampleRates[highest]);
    }
}

void audioSetSample(int source, uint32_t samplerate)
{
    sampleRates[source] = samplerate;

    // if we're not the highest priority audio source, dont set it
    if (source != highestPriAudio())
        return;

    samplerate = (BOARD_MCK + samplerate - 1) / samplerate; //err on the side of slower audio

    PWMC_SetPeriod(PWM, 0, samplerate);
    PWMC_ConfigureComparisonUnit(PWM, 0, (samplerate + 1) >> 1, 1);
}

void audioAddBuffer(int source, const uint16_t* samples, uint32_t numSamples)
{
    // see if we're the highest priority audio, drop on the floor if we're not
    if (source != highestPriAudio())
        return;

    while(!DACC_WriteBuffer(DACC, samples, numSamples)) coopYield();
}

int audioTryAddBuffer(int source, const uint16_t* samples, uint32_t numSamples)
{
    // see if we're the highest priority audio, drop on the floor if we're not
    if (source != highestPriAudio())
        return 1; // pretend we queued

    int first_free = (DACC->DACC_TCR == 0) && (DACC->DACC_TNCR == 0);

    uint32_t res = DACC_WriteBuffer(DACC, samples, numSamples);

    if (!res)
        return 0;

    if (first_free)
        return 1;
    else
        return DACC->DACC_TCR;
}

