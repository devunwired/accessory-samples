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
#ifndef _COOP_H_
#define _COOP_H_


typedef void (*CoopTaskF)(void* ptr);

int coopInit(void);							//0 on fail
int coopSpawn(CoopTaskF task, void* taskData, uint32_t stackSz);	//0 on fail
void coopYield(void);

void sleep(uint32_t ms);

#endif
#endif

