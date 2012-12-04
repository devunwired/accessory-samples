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
#ifndef _AUDIO_H_
#define _AUDIO_H_

#define AUDIO_NULL 0
#define AUDIO_USB 1
#define AUDIO_BT  2
#define AUDIO_ALARM 3

#define AUDIO_MAX_SOURCE 4

void audioInit(void);
void audioOn(int source, uint32_t samplerate);
void audioOff(int source);
void audioSetSample(int source, uint32_t samplerate);

void audioAddBuffer(int source, const uint16_t* samples, uint32_t numSamples);	//if buffers full, will block until they arent...
int audioTryAddBuffer(int source, const uint16_t* samples, uint32_t numSamples);	//0 if failed, positive number of existing queued samples

#define AUDIO_BITS	12



#endif
#endif

