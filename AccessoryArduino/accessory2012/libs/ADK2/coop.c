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

#define NUM_REGS	10	//r4-r11,sp,pc

typedef struct CoopTask{

    uint32_t regs[NUM_REGS];
    void* stackPtr;
    struct CoopTask* next;
    struct CoopTask* prev;
}CoopTask;

CoopTask* cur = 0;

static CoopTask* __attribute__((noinline)) coopSchedule(char taskDied){

    CoopTask* next = cur->next;

    if(taskDied){		//delete task

        if(next == cur){
            dbgPrintf("Last task died. Halting.\n");
            while(1);
        }

        if(cur->stackPtr) free(cur->stackPtr);
        cur->next->prev = cur->prev;
        cur->prev->next = cur->next;
        free(cur);
    }
    cur = next;
    return next;
}


static void __attribute__((naked)) __attribute__((noinline)) coopTaskStart(void){

    asm(
        "mov r0, r5;"
        "blx r4;"
        "mov r0, #1;"
	"bl  coopSchedule;"
        "ldmia r0, {r4-r12,lr};"
        "mov sp, r12;"
        "bx lr;"
    );
}

static void __attribute__((naked)) __attribute__((noinline)) coopDoYield(CoopTask* curTask){

    asm(
        "mov r12, sp;"
        "stmia r0, {r4-r12,lr};"
        "mov r0, #0;"
	"bl coopSchedule;"
	"ldmia r0, {r4-r12,lr};"
        "mov sp, r12;"
        "bx lr;"
    );
}

int coopInit(void){

    CoopTask* task;

    task = malloc(sizeof(CoopTask));
    if(!task) return 0;

    task->next = task;
    task->prev = task;
    task->stackPtr = 0;
    cur = task;

    return 1;
}

int coopSpawn(CoopTaskF taskF, void* taskData, uint32_t stackSz){

    uint8_t* stack;
    CoopTask* task;

    stack = malloc(stackSz);
    if(!stack) return 0;

    task = malloc(sizeof(CoopTask));
    if(!task){
        free(stack);
        return 0;
    }

    task->stackPtr = stack;
    task->regs[0] = (uint32_t)taskF;
    task->regs[1] = (uint32_t)taskData;
    task->regs[8] = ((uint32_t)(stack + stackSz)) &~ 7;
    task->regs[9] = (uint32_t)&coopTaskStart;

    task->prev = cur;
    task->next = cur->next;
    cur->next->prev = task;
    cur->next = task;

    //these are here so compiler is sure that function is referenced in both variants (cancels a warning)
    if(stackSz == 0xFFFFFFFF) coopSchedule(0);
    if(stackSz == 0xFFFFFFFE) coopSchedule(1);

    return 1;
}

void coopYield(void){
    coopDoYield(cur);
}

void sleep(uint32_t ms)
{
	uint64_t a = fwkGetUptime();
	while (fwkGetUptime() - a < ms)
		coopYield();
}

