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
#include "capsense.h"
#include "I2C.h"
#include "dbg.h"

#define I2C_MUX_ADDR    0x70
#define QT_ADDR         0x1C

#define QT_ID           0x3E
#define QT_REG_ID       0
#define QT_REG_SLIDER   5
#define QT_REG_KEY0     3
#define QT_REG_KEY1     4
#define QT_REG_CAL      6
#define QT_REG_SIG      52
#define QT_REG_REF      76

#define SLIDER_SENSE_TH  30
#define ICON_SENSE_TH    18
#define ICON_SENSE_TH2   19
#define BUTTON_SENSE_TH  22
#define BUTTON_SENSE_TH2 15
#define SLIDER_PS        0x41
#define ICON_PS          0x42
#define ICON_PS_NOISY    0x64  /* For extra noisy icon buttons */
#define BUTTON_PS        0x42
#define BUTTON_PS_NOISY  0x64

#define AKS_G1           1 << 2     /* Adjacent key suppression groups */
#define AKS_G2           2 << 2
#define AKS_G3           3 << 2

const uint8_t capsense1_init_table[45] = {  8,  /* Start address = 8 */
                                            1,               /* 08 - LP mode = 1 (16ms) */
                                           20,              /* 09 - TTD = 20 (3.2s/ref level) */
                                           5,               /* 10 - ATD = 5 (0.8s/ref level) */
                                           5,               /* 11 - DI = 5 (5 values integrated) */
                                           63,              /* 12 - Touch recal = 63 (10s) */
                                           25,              /* 13 - Drift hold = 25 (4 seconds) */
                                           0x80,            /* 14 - Slider = Enabled, no wheel */
                                           0,               /* 15 - Charge time = 0 */
                                           SLIDER_SENSE_TH, /* 16 - Detect threshold CH0 */
                                           SLIDER_SENSE_TH, /* 17 - Detect threshold CH1 */
                                           SLIDER_SENSE_TH, /* 18 - Detect threshold CH2 */
                                           ICON_SENSE_TH,   /* 19 - Detect threshold CH3 */
                                           ICON_SENSE_TH,   /* 20 - Detect threshold CH4 */
                                           ICON_SENSE_TH,   /* 21 - Detect threshold CH5 */
                                           ICON_SENSE_TH,   /* 22 - Detect threshold CH6 */
                                           ICON_SENSE_TH2,  /* 23 - Detect threshold CH7 */
                                           ICON_SENSE_TH,   /* 24 - Detect threshold CH8 */
                                           ICON_SENSE_TH,   /* 25 - Detect threshold CH9 */
                                           ICON_SENSE_TH,   /* 26 - Detect threshold CH10 */
                                           ICON_SENSE_TH,   /* 27 - Detect threshold CH11 */
                                           0,               /* 28 - Key control CH0 */
                                           0,               /* 29 - Key control CH1 */
                                           0,               /* 30 - Key control CH2 */
                                           AKS_G1,          /* 31 - Key control CH3 */
                                           AKS_G1,          /* 32 - Key control CH4 */
                                           AKS_G1,          /* 33 - Key control CH5 */
                                           AKS_G1,          /* 34 - Key control CH6 */
                                           AKS_G1,          /* 35 - Key control CH7 */
                                           AKS_G1,          /* 36 - Key control CH8 */
                                           AKS_G1,          /* 37 - Key control CH9 */
                                           AKS_G1,          /* 38 - Key control CH10 */
                                           AKS_G1,          /* 39 - Key control CH11 */
                                           SLIDER_PS,       /* 40 - Pulse/Scale CH0 */
                                           SLIDER_PS,       /* 41 - Pulse/Scale CH1 */
                                           SLIDER_PS,       /* 42 - Pulse/Scale CH2 */
                                           ICON_PS,         /* 43 - Pulse/Scale CH3 */
                                           ICON_PS_NOISY,   /* 44 - Pulse/Scale CH4 */
                                           ICON_PS,         /* 45 - Pulse/Scale CH5 */
                                           ICON_PS,         /* 46 - Pulse/Scale CH6 */
                                           ICON_PS_NOISY,   /* 47 - Pulse/Scale CH7 */
                                           ICON_PS_NOISY,   /* 48 - Pulse/Scale CH8 */
                                           ICON_PS,         /* 49 - Pulse/Scale CH9 */
                                           ICON_PS,         /* 50 - Pulse/Scale CH10 */
                                           ICON_PS          /* 51 - Pulse/Scale CH11 */ };

const uint8_t capsense2_init_table[45] = {  8,  /* Start address = 8 */
                                            1,               /* 08 - LP mode = 1 (16ms) */
                                           20,              /* 09 - TTD = 20 (3.2s/ref level) */
                                           5,               /* 10 - ATD = 5 (0.8s/ref level) */
                                           5,               /* 11 - DI = 5 (5 values integrated) */
                                           63,              /* 12 - Touch recal = 63 (10s) */
                                           25,              /* 13 - Drift hold = 25 (4 seconds) */
                                           0x00,            /* 14 - Slider = Disabled, no wheel */
                                           0,               /* 15 - Charge time = 0 */
                                           BUTTON_SENSE_TH, /* 16 - Detect threshold CH0 */
                                           BUTTON_SENSE_TH, /* 17 - Detect threshold CH1 */
                                           BUTTON_SENSE_TH, /* 18 - Detect threshold CH2 */
                                           BUTTON_SENSE_TH, /* 19 - Detect threshold CH3 */
                                           BUTTON_SENSE_TH, /* 20 - Detect threshold CH4 */
                                           BUTTON_SENSE_TH, /* 21 - Detect threshold CH5 */
                                           BUTTON_SENSE_TH, /* 22 - Detect threshold CH6 */
                                           BUTTON_SENSE_TH, /* 23 - Detect threshold CH7 */
                                           BUTTON_SENSE_TH, /* 24 - Detect threshold CH8 */
                                           BUTTON_SENSE_TH, /* 25 - Detect threshold CH9 */
                                           BUTTON_SENSE_TH2,/* 26 - Detect threshold CH10 */
                                           BUTTON_SENSE_TH2,/* 27 - Detect threshold CH11 */
                                           AKS_G1,          /* 28 - Key control CH0 */
                                           AKS_G1,               /* 29 - Key control CH1 */
                                           AKS_G1,               /* 30 - Key control CH2 */
                                           AKS_G1,               /* 31 - Key control CH3 */
                                           AKS_G2,               /* 32 - Key control CH4 */
                                           AKS_G2,               /* 33 - Key control CH5 */
                                           AKS_G2,               /* 34 - Key control CH6 */
                                           AKS_G2,               /* 35 - Key control CH7 */
                                           AKS_G3,               /* 36 - Key control CH8 */
                                           AKS_G3,               /* 37 - Key control CH9 */
                                           AKS_G3,               /* 38 - Key control CH10 */
                                           AKS_G3,               /* 39 - Key control CH11 */
                                           BUTTON_PS_NOISY,       /* 40 - Pulse/Scale CH0 */
                                           BUTTON_PS_NOISY,       /* 41 - Pulse/Scale CH1 */
                                           BUTTON_PS,       /* 42 - Pulse/Scale CH2 */
                                           BUTTON_PS,       /* 43 - Pulse/Scale CH3 */
                                           BUTTON_PS,       /* 44 - Pulse/Scale CH4 */
                                           BUTTON_PS,       /* 45 - Pulse/Scale CH5 */
                                           BUTTON_PS,       /* 46 - Pulse/Scale CH6 */
                                           BUTTON_PS,       /* 47 - Pulse/Scale CH7 */
                                           BUTTON_PS,       /* 48 - Pulse/Scale CH8 */
                                           BUTTON_PS,       /* 49 - Pulse/Scale CH9 */
                                           BUTTON_PS,       /* 50 - Pulse/Scale CH10 */
                                           BUTTON_PS_NOISY        /* 51 - Pulse/Scale CH11 */ };


void I2C_Mux(uint8_t position)
{
    uint8_t reg;
    switch(position){
        case 1:
            reg = 4;
            break;
        case 2:
            reg = 5;
            break;
        default:
            reg = 0;
            break;
    }
    i2cQuick(1, I2C_MUX_ADDR, reg);
}

char capSenseInit(void)
{
    I2C_Mux(1);

    if (QT_ID != i2cSingleRead(1, QT_ADDR, QT_REG_ID))
        return 0;

    if (I2C_ALL_OK != i2cOp(1, QT_ADDR, capsense1_init_table, sizeof(capsense1_init_table), NULL, 0))
        return 0;

    i2cSingleWrite(1, QT_ADDR, QT_REG_CAL, 1);

    I2C_Mux(2);

    if (QT_ID != i2cSingleRead(1, QT_ADDR, QT_REG_ID))
        return 0;

    if (I2C_ALL_OK != i2cOp(1, QT_ADDR, capsense2_init_table, sizeof(capsense2_init_table), NULL, 0))
        return 0;

    i2cSingleWrite(1, QT_ADDR, QT_REG_CAL, 1);

    return 1;
}

uint8_t capSenseSlider(void)
{
    uint8_t cmd = QT_REG_SLIDER;
    uint8_t dat;
    I2C_Mux(1);
    i2cOp(1, QT_ADDR, &cmd, 1, &dat, 1);
    //return i2cSingleRead(1, QT_ADDR, QT_REG_SLIDER);
    return dat;
}

uint16_t capSenseButtons(void)
{
    uint8_t cmd = QT_REG_KEY0;
    uint8_t dat[2];
    I2C_Mux(2);
    i2cOp(1, QT_ADDR, &cmd, 1, dat, sizeof(dat));
    return dat[0] + ((uint16_t)dat[1] << 8);
}

uint16_t capSenseIcons(void)
{
    uint8_t cmd = QT_REG_KEY0;
    uint8_t dat[2];
    I2C_Mux(1);
    i2cOp(1, QT_ADDR, &cmd, 1, dat, sizeof(dat));
    return dat[0] + ((uint16_t)dat[1] << 8);
}

void capSenseDump(void)
{
    int i;
    uint8_t cmd;
    uint16_t dat[48];
    I2C_Mux(1);
    cmd = QT_REG_SIG;
    i2cOp(1, QT_ADDR, &cmd, 1, (uint8_t *)dat, 48);
    I2C_Mux(2);
    i2cOp(1, QT_ADDR, &cmd, 1, (uint8_t *)(&dat[24]), 48);

    for (i = 0; i < 12; i++) {
        dbgPrintf("%04x %04x ", dat[i], dat[i+12]);
    }
    for (i = 0; i < 12; i++) {
        dbgPrintf("%04x %04x ", dat[i+24], dat[i+24+12]);
    }
    dbgPrintf("\n");

}
