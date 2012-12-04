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
#include "btRFCOMM.h"
#include "btA2DP.h"
#include "btSDP.h"
#include "btL2CAP.h"
#include "dbg.h"
#include "Audio.h"
#include <string.h>
#include "coop.h"


#define UGLY_SCARY_DEBUGGING_CODE	0

//endpoint media types
	#define AVDTP_MEDIA_TYP_AUDIO		0
	#define AVDTP_MEDIA_TYP_VIDEO		1
	#define AVDTP_MEDIA_TYP_MULTIMEDIA	2

//andpoint directions
	#define AVDTP_DIR_SOURCE		0
	#define AVDTP_DIR_SINK			1


//A2DP packet header (2bytes)
	//byte 0
	#define AVDTP_HDR_MASK_TRANS	0xF0
	#define AVDTP_HDR_SHIFT_TRANS	4
	#define AVDTP_HDR_MASK_PKT_TYP	0x0C
	#define AVDTP_HDR_SHIFT_PKT_TYP	2
	#define AVDTP_HDR_MASK_MSG_TYP	0x03
	#define AVDTP_HDR_SHIFT_MSG_TYP	0
	//byte 1
	#define AVDTP_HDR_MASK_SIG_ID	0x3F
	#define AVDTP_HDR_SHIFT_SIG_ID	0

//sigId
	#define AVDTP_SIG_DISCOVER			0x01
	#define AVDTP_SIG_GET_CAPABILITIES		0x02
	#define AVDTP_SIG_SET_CONFIGURATION		0x03
	#define AVDTP_SIG_GET_CONFIGURATION		0x04
	#define AVDTP_SIG_RECONFIGURE			0x05
	#define AVDTP_SIG_OPEN				0x06
	#define AVDTP_SIG_START				0x07
	#define AVDTP_SIG_CLOSE				0x08
	#define AVDTP_SIG_SUSPEND			0x09
	#define AVDTP_SIG_ABORT				0x0A
	#define AVDTP_SIG_SECURITY_CONTROL		0x0B

//pktTyp
	#define AVDTP_PKT_TYP_SINGLE			0x00
	#define AVDTP_PKT_TYP_START			0x01
	#define AVDTP_PKT_TYP_CONTINUE			0x02
	#define AVDTP_PKT_TYP_END			0x03

//msgTyp
	#define AVDTP_MSG_TYP_CMD			0x00
	#define AVDTP_MSG_TYP_ACCEPT			0x02
	#define AVDTP_MSG_TYP_REJ			0x03

//seid info (2 bytes)
	//byte0
	#define AVDTP_SEID_NFO_MASK_SEID		0xFC
	#define AVDTP_SEID_NFO_SHIFT_SEID		2
	#define AVDTP_SEID_NFO_MASK_INUSE		0x02
	#define AVDTP_SEID_NFO_SHIFT_INUSE		1
	//byte1
	#define AVDTP_SEID_NFO_MASK_MEDIA_TYP		0xF0
	#define AVDTP_SEID_NFO_SHIFT_MEDIA_TYP		4
	#define AVDTP_SEID_NFO_MASK_TYP			0x08
	#define AVDTP_SEID_NFO_SHIFT_TYP		3


//service capability categoris
	#define AVDTP_SVC_CAT_MEDIA_TRANSPORT		1
	#define AVDTP_SVC_CAT_REPORTING			2
	#define AVDTP_SVC_CAT_RECOVERY			3
	#define AVDTP_SVC_CAT_CONTENT_PROTECTION	4
	#define AVDTP_SVC_CAT_HEADER_COMPRESSION	5
	#define AVDTP_SVC_CAT_MULTIPLEXING		6
	#define AVDTP_SVC_CAT_MEDIA_CODEC		7

//codec types
	#define A2DP_CODEC_TYP_SBC			0
	//others exist...done don't implement them

#define MY_ENDPT_ID					1		//we support just one, and this is it's ID

#define A2DP_CHAN_MODE_MONO				0
#define A2DP_CHAN_MODE_DUAL_CHANNEL			1
#define A2DP_CHAN_MODE_STEREO				2
#define A2DP_CHAN_MODE_JOINT_STEREO			3


#define DATA_STATE_ENCODE(state)			((void*)(((uint32_t)(state)) ^ 0x80000000))
#define IS_DATA_STATE(state)				(!!(((uint32_t)(state)) & 0x80000000))

typedef struct A2DPstate{

    //L2CAP data
    uint16_t aclConn;
    uint16_t remChan; //remote channel num for control channel
    uint16_t datChan; //remote channel num for data channel

    //codec data
    uint16_t samplingRate;

    //our state
    char needAudioConn;

    //link
    struct A2DPstate* next;

}A2DPstate;

static A2DPstate* conns = NULL;

//copyright notice for SBC decoder
/*
Copyright (c) 2011, Dmitry Grinberg (as published on http://dmitrygr.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following condition is met: 

* Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#define QUALITY_LOWEST	1	//you may notice the quality reduction
#define QUALITY_MEDIUM	2	//pretty good
#define QUALITY_GREAT	3	//as good as it will get without an FPU


///config options begin

/*
	This is a rather clever little SBC decoder that I've put together 

*/

#define QUALITY	QUALITY_MEDIUM	
#define SPEED_OVER_ACCURACY			//set to cheat a bit with shifts (saves a divide per sample)
#define ITER		uint32_t			//iterator up to 180 use fastest type for your platform

///config options end


#if QUALITY == QUALITY_LOWEST

	#define CONST(x)		(x >> 24)
	#define SAMPLE_CVT(x)		(x >> 8)
	#define INSAMPLE		int8_t
	#define OUTSAMPLE		uint8_t	//no point producing 16-bit samples using the 8-bit decoder
	#define FIXED			int8_t
	#define FIXED_S			int16_t
	#define OUT_CLIP_MAX		0x7F
	#define OUT_CLIP_MIN		-0x80

	#define NUM_FRAC_BITS_PROTO	8
	#define NUM_FRAC_BITS_COS	6

#elif QUALITY == QUALITY_MEDIUM

	#define CONST(x)		(x >> 16)
	#define SAMPLE_CVT(x)		(x)
	#define INSAMPLE		int16_t
	#define OUTSAMPLE		uint16_t
	#define FIXED			int16_t
	#define FIXED_S			int32_t
	#define OUT_CLIP_MAX		0x7FFF
	#define OUT_CLIP_MIN		-0x8000

	#define NUM_FRAC_BITS_PROTO	16
	#define NUM_FRAC_BITS_COS	14

#elif QUALITY == QUALITY_GREAT

	#define CONST(x)		(x)
	#define SAMPLE_CVT(x)		(x)
	#define INSAMPLE		int16_t
	#define OUTSAMPLE		uint16_t
	#define FIXED			int32_t
	#define FIXED_S			int64_t
	#define OUT_CLIP_MAX		0x7FFF
	#define OUT_CLIP_MIN		-0x8000

	#define NUM_FRAC_BITS_PROTO	32
	#define NUM_FRAC_BITS_COS	30

#else

	#error "You did not define SBC decoder synthesizer quality to use"

#endif



static const FIXED proto_4_40[] =
{
	CONST(0x00000000), CONST(0x00FB7991), CONST(0x02CB3E8B), CONST(0x069FDC59),
	CONST(0x22B63DA5), CONST(0x4B583FE6), CONST(0xDD49C25B), CONST(0x069FDC59),
	CONST(0xFD34C175), CONST(0x00FB7991), CONST(0x002329CC), CONST(0x00FF11CA),
	CONST(0x053B7546), CONST(0x0191E578), CONST(0x31EAB920), CONST(0x4825E4A3),
	CONST(0xEC1F5E6D), CONST(0x083DDC80), CONST(0xFF3773A8), CONST(0x00B32807),
	CONST(0x0061C5A7), CONST(0x007A4737), CONST(0x07646684), CONST(0xF89F23A7),
	CONST(0x3F23948D), CONST(0x3F23948D), CONST(0xF89F23A7), CONST(0x07646684),
	CONST(0x007A4737), CONST(0x0061C5A7), CONST(0x00B32807), CONST(0xFF3773A8),
	CONST(0x083DDC80), CONST(0xEC1F5E6D), CONST(0x4825E4A3), CONST(0x31EAB920),
	CONST(0x0191E578), CONST(0x053B7546), CONST(0x00FF11CA), CONST(0x002329CC)
};

static const FIXED proto_8_80[] =
{
	CONST(0x00000000), CONST(0x0083D8D4), CONST(0x0172E691), CONST(0x034FD9E0),
	CONST(0x116860F5), CONST(0x259ED8EB), CONST(0xEE979F0B), CONST(0x034FD9E0),
	CONST(0xFE8D196F), CONST(0x0083D8D4), CONST(0x000A42E6), CONST(0x0089DE90),
	CONST(0x020E372C), CONST(0x02447D75), CONST(0x153E7D35), CONST(0x253844DE),
	CONST(0xF2625120), CONST(0x03EBE849), CONST(0xFF1ACF26), CONST(0x0074E5CF),
	CONST(0x00167EE3), CONST(0x0082B6EC), CONST(0x02AD6794), CONST(0x00BFA1FF),
	CONST(0x18FAB36D), CONST(0x24086BF5), CONST(0xF5FF2BF8), CONST(0x04270CA8),
	CONST(0xFF93E21B), CONST(0x0060C1E9), CONST(0x002458FC), CONST(0x0069F16C),
	CONST(0x03436717), CONST(0xFEBDD6E5), CONST(0x1C7762DF), CONST(0x221D9DE0),
	CONST(0xF950DCFC), CONST(0x0412523E), CONST(0xFFF44825), CONST(0x004AB4C5),
	CONST(0x0035FF13), CONST(0x003B1FA4), CONST(0x03C04499), CONST(0xFC4086B8),
	CONST(0x1F8E43F2), CONST(0x1F8E43F2), CONST(0xFC4086B8), CONST(0x03C04499),
	CONST(0x003B1FA4), CONST(0x0035FF13), CONST(0x004AB4C5), CONST(0xFFF44825),
	CONST(0x0412523E), CONST(0xF950DCFC), CONST(0x221D9DE0), CONST(0x1C7762DF),
	CONST(0xFEBDD6E5), CONST(0x03436717), CONST(0x0069F16C), CONST(0x002458FC),
	CONST(0x0060C1E9), CONST(0xFF93E21B), CONST(0x04270CA8), CONST(0xF5FF2BF8),
	CONST(0x24086BF5), CONST(0x18FAB36D), CONST(0x00BFA1FF), CONST(0x02AD6794),
	CONST(0x0082B6EC), CONST(0x00167EE3), CONST(0x0074E5CF), CONST(0xFF1ACF26),
	CONST(0x03EBE849), CONST(0xF2625120), CONST(0x253844DE), CONST(0x153E7D35),
	CONST(0x02447D75), CONST(0x020E372C), CONST(0x0089DE90), CONST(0x000A42E6)
};

static const FIXED costab_4[] =
{
	CONST(0x2D413CCD), CONST(0xD2BEC333), CONST(0xD2BEC333), CONST(0x2D413CCD),
	CONST(0x187DE2A7), CONST(0xC4DF2862), CONST(0x3B20D79E), CONST(0xE7821D59),
	CONST(0x00000000), CONST(0x00000000), CONST(0x00000000), CONST(0x00000000),
	CONST(0xE7821D59), CONST(0x3B20D79E), CONST(0xC4DF2862), CONST(0x187DE2A7),
	CONST(0xD2BEC333), CONST(0x2D413CCD), CONST(0x2D413CCD), CONST(0xD2BEC333),
	CONST(0xC4DF2862), CONST(0xE7821D59), CONST(0x187DE2A7), CONST(0x3B20D79E),
	CONST(0xC0000000), CONST(0xC0000000), CONST(0xC0000000), CONST(0xC0000000),
	CONST(0xC4DF2862), CONST(0xE7821D59), CONST(0x187DE2A7), CONST(0x3B20D79E)
};

static const FIXED costab_8[] =
{
	CONST(0x2D413CCD), CONST(0xD2BEC333), CONST(0xD2BEC333), CONST(0x2D413CCD),
	CONST(0x2D413CCD), CONST(0xD2BEC333), CONST(0xD2BEC333), CONST(0x2D413CCD),
	CONST(0x238E7673), CONST(0xC13AD060), CONST(0x0C7C5C1E), CONST(0x3536CC52),
	CONST(0xCAC933AE), CONST(0xF383A3E2), CONST(0x3EC52FA0), CONST(0xDC71898D),
	CONST(0x187DE2A7), CONST(0xC4DF2862), CONST(0x3B20D79E), CONST(0xE7821D59),
	CONST(0xE7821D59), CONST(0x3B20D79E), CONST(0xC4DF2862), CONST(0x187DE2A7),
	CONST(0x0C7C5C1E), CONST(0xDC71898D), CONST(0x3536CC52), CONST(0xC13AD060),
	CONST(0x3EC52FA0), CONST(0xCAC933AE), CONST(0x238E7673), CONST(0xF383A3E2),
	CONST(0x00000000), CONST(0x00000000), CONST(0x00000000), CONST(0x00000000),
	CONST(0x00000000), CONST(0x00000000), CONST(0x00000000), CONST(0x00000000),
	CONST(0xF383A3E2), CONST(0x238E7673), CONST(0xCAC933AE), CONST(0x3EC52FA0),
	CONST(0xC13AD060), CONST(0x3536CC52), CONST(0xDC71898D), CONST(0x0C7C5C1E),
	CONST(0xE7821D59), CONST(0x3B20D79E), CONST(0xC4DF2862), CONST(0x187DE2A7),
	CONST(0x187DE2A7), CONST(0xC4DF2862), CONST(0x3B20D79E), CONST(0xE7821D59),
	CONST(0xDC71898D), CONST(0x3EC52FA0), CONST(0xF383A3E2), CONST(0xCAC933AE),
	CONST(0x3536CC52), CONST(0x0C7C5C1E), CONST(0xC13AD060), CONST(0x238E7673),
	CONST(0xD2BEC333), CONST(0x2D413CCD), CONST(0x2D413CCD), CONST(0xD2BEC333),
	CONST(0xD2BEC333), CONST(0x2D413CCD), CONST(0x2D413CCD), CONST(0xD2BEC333),
	CONST(0xCAC933AE), CONST(0x0C7C5C1E), CONST(0x3EC52FA0), CONST(0x238E7673),
	CONST(0xDC71898D), CONST(0xC13AD060), CONST(0xF383A3E2), CONST(0x3536CC52),
	CONST(0xC4DF2862), CONST(0xE7821D59), CONST(0x187DE2A7), CONST(0x3B20D79E),
	CONST(0x3B20D79E), CONST(0x187DE2A7), CONST(0xE7821D59), CONST(0xC4DF2862),
	CONST(0xC13AD060), CONST(0xCAC933AE), CONST(0xDC71898D), CONST(0xF383A3E2),
	CONST(0x0C7C5C1E), CONST(0x238E7673), CONST(0x3536CC52), CONST(0x3EC52FA0),
	CONST(0xC0000000), CONST(0xC0000000), CONST(0xC0000000), CONST(0xC0000000),
	CONST(0xC0000000), CONST(0xC0000000), CONST(0xC0000000), CONST(0xC0000000),
	CONST(0xC13AD060), CONST(0xCAC933AE), CONST(0xDC71898D), CONST(0xF383A3E2),
	CONST(0x0C7C5C1E), CONST(0x238E7673), CONST(0x3536CC52), CONST(0x3EC52FA0),
	CONST(0xC4DF2862), CONST(0xE7821D59), CONST(0x187DE2A7), CONST(0x3B20D79E),
	CONST(0x3B20D79E), CONST(0x187DE2A7), CONST(0xE7821D59), CONST(0xC4DF2862),
	CONST(0xCAC933AE), CONST(0x0C7C5C1E), CONST(0x3EC52FA0), CONST(0x238E7673),
	CONST(0xDC71898D), CONST(0xC13AD060), CONST(0xF383A3E2), CONST(0x3536CC52)
};

static const int8_t loudness_4[4][4] =
{
	{ -1, 0, 0, 0 }, { -2, 0, 0, 1 },
	{ -2, 0, 0, 1 }, { -2, 0, 0, 1 }
};

static const int8_t loudness_8[4][8] =
{
	{ -2, 0, 0, 0, 0, 0, 0, 1 }, { -3, 0, 0, 0, 0, 0, 1, 2 },
	{ -4, 0, 0, 0, 0, 0, 1, 2 }, { -4, 0, 0, 0, 0, 0, 1, 2 }
};


#if QUALITY != QUALITY_MEDIUM  //for medium we provide nice assembly routines

    static void synth_4(OUTSAMPLE* dst, const INSAMPLE* src, FIXED* V){  //A2DP figure 12.3

        ITER i, j;
        const FIXED* tabl = proto_4_40;
        const FIXED* costab = costab_4;

        //shift
        for(i = 79; i >= 8; i--) V[i] = V[i - 8];

        //matrix
        for(i = 0; i < 8; i++){

            FIXED_S t =    (FIXED_S)costab[0] * (FIXED_S)src[0] +
                           (FIXED_S)costab[1] * (FIXED_S)src[1] +
                           (FIXED_S)costab[2] * (FIXED_S)src[2] +
                           (FIXED_S)costab[3] * (FIXED_S)src[3];
            costab += 4;
            V[i] = t >> NUM_FRAC_BITS_COS;
        }

        //calculate audio samples
        for(j = 0; j < 4; j++){

            OUTSAMPLE s;
            FIXED_S sample =    (FIXED_S)V[j +  0] * (FIXED_S)tabl[0] +
                                (FIXED_S)V[j + 12] * (FIXED_S)tabl[1] +
                                (FIXED_S)V[j + 16] * (FIXED_S)tabl[2] +
                                (FIXED_S)V[j + 28] * (FIXED_S)tabl[3] +
                                (FIXED_S)V[j + 32] * (FIXED_S)tabl[4] +
                                (FIXED_S)V[j + 44] * (FIXED_S)tabl[5] +
                                (FIXED_S)V[j + 48] * (FIXED_S)tabl[6] +
                                (FIXED_S)V[j + 60] * (FIXED_S)tabl[7] +
                                (FIXED_S)V[j + 64] * (FIXED_S)tabl[8] +
                                (FIXED_S)V[j + 76] * (FIXED_S)tabl[9];
            tabl += 10;

            sample >>= (NUM_FRAC_BITS_PROTO - 1 - 2);    //-2 is for the -4 we need to multiply by :)
            sample = -sample;

            if(sample > OUT_CLIP_MAX) sample = OUT_CLIP_MAX;
            else if(sample < OUT_CLIP_MIN) sample = OUT_CLIP_MIN;
            s = sample;

            #ifndef DESKTOP
                s += 0x8000;
                s += 8;
                s >>= 4;
            #endif

            dst[j] = s;
        }
    }

    static void synth_8(OUTSAMPLE* dst, const INSAMPLE* src, FIXED* V){  //A2DP figure 12.3

        ITER i, j;
        const FIXED* tabl = proto_8_80;
        const FIXED* costab = costab_8;

        //shift
        for(i = 159; i >= 16; i--) V[i] = V[i - 16];

        //matrix
        for(i = 0; i < 16; i++){

            FIXED_S t =    (FIXED_S)costab[0] * (FIXED_S)src[0] +
                           (FIXED_S)costab[1] * (FIXED_S)src[1] +
                           (FIXED_S)costab[2] * (FIXED_S)src[2] +
                           (FIXED_S)costab[3] * (FIXED_S)src[3] +
                           (FIXED_S)costab[4] * (FIXED_S)src[4] +
                           (FIXED_S)costab[5] * (FIXED_S)src[5] +
                           (FIXED_S)costab[6] * (FIXED_S)src[6] +
                           (FIXED_S)costab[7] * (FIXED_S)src[7];
            costab += 8;
            V[i] = t >> NUM_FRAC_BITS_COS;
        }

        //calculate audio samples
        for(j = 0; j < 8; j++){

            OUTSAMPLE s;
            FIXED_S sample =    (FIXED_S)V[j +  0] * (FIXED_S)tabl[0] +
                                (FIXED_S)V[j + 24] * (FIXED_S)tabl[1] +
                                (FIXED_S)V[j + 32] * (FIXED_S)tabl[2] +
                                (FIXED_S)V[j + 56] * (FIXED_S)tabl[3] +
                                (FIXED_S)V[j + 64] * (FIXED_S)tabl[4] +
                                (FIXED_S)V[j + 88] * (FIXED_S)tabl[5] +
                                (FIXED_S)V[j + 96] * (FIXED_S)tabl[6] +
                                (FIXED_S)V[j +120] * (FIXED_S)tabl[7] +
                                (FIXED_S)V[j +128] * (FIXED_S)tabl[8] +
                                (FIXED_S)V[j +152] * (FIXED_S)tabl[9];
            tabl += 10;

            sample >>= (NUM_FRAC_BITS_PROTO - 1 - 3);    //-3 is for the -8 we need to multiply by :)
            sample = -sample;

            if(sample > OUT_CLIP_MAX) sample = OUT_CLIP_MAX;
            else if(sample < OUT_CLIP_MIN) sample = OUT_CLIP_MIN;
            s = sample;

            #ifndef DESKTOP
                s += 0x8000;
                s += 8;
                s >>= 4;
            #endif

            dst[j] = s;
        }
    }

    static void synth(OUTSAMPLE* dst, const INSAMPLE* src, uint8_t nBands, FIXED* V){  //A2DP sigure 12.3

        //efficient SBC synth by Dmitry Grinberg (as published May 26, 2012)
        if(nBands == 4) synth_4(dst, src, V);
        else synth_8(dst, src, V);
    }

#else

    static void __attribute__((naked)) synth_4_med(OUTSAMPLE* dst, const INSAMPLE* src, FIXED* V, const FIXED* costabl, const FIXED* prot_Tabl){

        asm(

        //prologue {8 cycles}
            "push {r4-r7}			\n\t"

        //shifting { 9x(9+9 + 1) + 1 = 171 cycles }
            "add   r2, #128			\n\t"
            "ldmia r2!, {r4-r7}		\n\t"
            "stmia r2, {r4-r7}		\n\t"
            "subs  r2, #32			\n\t"
            "ldmia r2!, {r4-r7}		\n\t"
            "stmia r2, {r4-r7}		\n\t"
            "subs  r2, #32			\n\t"
            "ldmia r2!, {r4-r7}		\n\t"
            "stmia r2, {r4-r7}		\n\t"
            "subs  r2, #32			\n\t"
            "ldmia r2!, {r4-r7}		\n\t"
            "stmia r2, {r4-r7}		\n\t"
            "subs  r2, #32			\n\t"
            "ldmia r2!, {r4-r7}		\n\t"
            "stmia r2, {r4-r7}		\n\t"
            "subs  r2, #32			\n\t"
            "ldmia r2!, {r4-r7}		\n\t"
            "stmia r2, {r4-r7}		\n\t"
            "subs  r2, #32			\n\t"
            "ldmia r2!, {r4-r7}		\n\t"
            "stmia r2, {r4-r7}		\n\t"
            "subs  r2, #32			\n\t"
            "ldmia r2!, {r4-r7}		\n\t"
            "stmia r2, {r4-r7}		\n\t"
            "subs  r2, #32			\n\t"
            "ldmia r2!, {r4-r7}		\n\t"
            "stmia r2, {r4-r7}		\n\t"
            "subs  r2, #16			\n\t"	//r2 is right back where it started

        //matrixing
            "movs  r7, #8			\n\t"	//r7 is loop index
        "matrix_loop_4:			\n\t"
            "ldrsh r4, [r3, #0]		\n\t"
            "ldrsh r5, [r1, #0]		\n\t"
            "mul   r6, r4, r5		\n\t"
            "ldrsh r4, [r3, #2]		\n\t"
            "ldrsh r5, [r1, #2]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #4]		\n\t"
            "ldrsh r5, [r1, #4]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #6]		\n\t"
            "ldrsh r5, [r1, #6]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "adds  r3, #8			\n\t"
            "asrs  r6, #14	 		\n\t"
            "strh  r6, [r2], #2		\n\t"
            "subs  r7, #1			\n\t"
            "bne   matrix_loop_4		\n\t"
        //return r2 to point where it should
            "subs  r2, #16			\n\t"

        //sample production
            "movs  r7, #4			\n\t"	//r7 is loop index
            "ldr   r3, [sp, #16]		\n\t"	//r3 is now proto table
        "sample_loop_4:			\n\t"
            "ldrsh r4, [r3, #0]		\n\t"
            "ldrsh r5, [r2, #0]		\n\t"
            "mul   r6, r4, r5		\n\t"
            "ldrsh r4, [r3, #2]		\n\t"
            "ldrsh r5, [r2, #24]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #4]		\n\t"
            "ldrsh r5, [r2, #32]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #6]		\n\t"
            "ldrsh r5, [r2, #56]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #8]		\n\t"
            "ldrsh r5, [r2, #64]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #10]		\n\t"
            "ldrsh r5, [r2, #88]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #12]		\n\t"
            "ldrsh r5, [r2, #96]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #14]		\n\t"
            "ldrsh r5, [r2, #120]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #16]		\n\t"
            "ldrsh r5, [r2, #128]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #18]		\n\t"
            "ldrsh r5, [r2, #152]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "adds  r3, #20			\n\t"
            "adds  r2, #2			\n\t"
            "negs  r6, r6			\n\t"
            "ssat  r6, #12, r6, asr #16	\n\t"
            "add   r6, #0x0800		\n\t"
            "strh  r6, [r0], #2		\n\t"
            "subs  r7, #1			\n\t"
            "bne   sample_loop_4		\n\t"

        //cleanup
            "pop   {r4-r7}			\n\t"
            "bx    lr			\n\t"
        );
    }

    static void __attribute__((naked)) synth_8_med(OUTSAMPLE* dst, const INSAMPLE* src, FIXED* V, const FIXED* costabl, const FIXED* prot_Tabl){

        asm(

        //prologue {8 cycles}
            "push {r4-r10}			\n\t"

        //shifting { 9x(9+9 + 1) + 1 = 171 cycles }
            "add   r2, #256			\n\t"
            "ldmia r2!, {r4-r10, r12}	\n\t"
            "stmia r2, {r4-r10, r12}	\n\t"
            "subs  r2, #64			\n\t"
            "ldmia r2!, {r4-r10, r12}	\n\t"
            "stmia r2, {r4-r10, r12}	\n\t"
            "subs  r2, #64			\n\t"
            "ldmia r2!, {r4-r10, r12}	\n\t"
            "stmia r2, {r4-r10, r12}	\n\t"
            "subs  r2, #64			\n\t"
            "ldmia r2!, {r4-r10, r12}	\n\t"
            "stmia r2, {r4-r10, r12}	\n\t"
            "subs  r2, #64			\n\t"
            "ldmia r2!, {r4-r10, r12}	\n\t"
            "stmia r2, {r4-r10, r12}	\n\t"
            "subs  r2, #64			\n\t"
            "ldmia r2!, {r4-r10, r12}	\n\t"
            "stmia r2, {r4-r10, r12}	\n\t"
            "subs  r2, #64			\n\t"
            "ldmia r2!, {r4-r10, r12}	\n\t"
            "stmia r2, {r4-r10, r12}	\n\t"
            "subs  r2, #64			\n\t"
            "ldmia r2!, {r4-r10, r12}	\n\t"
            "stmia r2, {r4-r10, r12}	\n\t"
            "subs  r2, #64			\n\t"
            "ldmia r2!, {r4-r10, r12}	\n\t"
            "stmia r2, {r4-r10, r12}	\n\t"
            "subs  r2, #32			\n\t"	//r2 is right back where it started

        //matrixing
            "movs  r7, #16			\n\t"	//r7 is loop index
        "matrix_loop:			\n\t"
            "ldrsh r4, [r3, #0]		\n\t"
            "ldrsh r5, [r1, #0]		\n\t"
            "mul   r6, r4, r5		\n\t"
            "ldrsh r4, [r3, #2]		\n\t"
            "ldrsh r5, [r1, #2]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #4]		\n\t"
            "ldrsh r5, [r1, #4]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #6]		\n\t"
            "ldrsh r5, [r1, #6]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #8]		\n\t"
            "ldrsh r5, [r1, #8]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #10]		\n\t"
            "ldrsh r5, [r1, #10]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #12]		\n\t"
            "ldrsh r5, [r1, #12]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #14]		\n\t"
            "ldrsh r5, [r1, #14]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "adds  r3, #16			\n\t"
            "asrs  r6, #14	 		\n\t"
            "strh  r6, [r2], #2		\n\t"
            "subs  r7, #1			\n\t"
            "bne   matrix_loop		\n\t"
        //return r2 to point where it should
            "subs  r2, #32			\n\t"

        //sample production
            "movs  r7, #8			\n\t"	//r7 is loop index
            "ldr   r3, [sp, #28]		\n\t"	//r3 is now proto table
        "sample_loop:			\n\t"
            "ldrsh r4, [r3, #0]		\n\t"
            "ldrsh r5, [r2, #0]		\n\t"
            "mul   r6, r4, r5		\n\t"
            "ldrsh r4, [r3, #2]		\n\t"
            "ldrsh r5, [r2, #48]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #4]		\n\t"
            "ldrsh r5, [r2, #64]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #6]		\n\t"
            "ldrsh r5, [r2, #112]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #8]		\n\t"
            "ldrsh r5, [r2, #128]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #10]		\n\t"
            "ldrsh r5, [r2, #176]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #12]		\n\t"
            "ldrsh r5, [r2, #192]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #14]		\n\t"
            "ldrsh r5, [r2, #240]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #16]		\n\t"
            "ldrsh r5, [r2, #256]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "ldrsh r4, [r3, #18]		\n\t"
            "ldrsh r5, [r2, #304]		\n\t"
            "mla   r6, r4, r5, r6		\n\t"
            "adds  r3, #20			\n\t"
            "adds  r2, #2			\n\t"
            "negs  r6, r6			\n\t"
            "ssat  r6, #12, r6, asr #16	\n\t"
            "add   r6, #0x0800		\n\t"
            "strh  r6, [r0], #2		\n\t"
            "subs  r7, #1			\n\t"
            "bne   sample_loop		\n\t"

        //cleanup
            "pop   {r4-r10}			\n\t"
            "bx    lr			\n\t"
        );
    }

    static void synth(OUTSAMPLE* dst, const INSAMPLE* src, uint8_t nBands, FIXED* V){  //A2DP sigure 12.3

        //efficient SBC synth by Dmitry Grinberg (as published May 26, 2012)
        if(nBands == 4) synth_4_med(dst, src, V, costab_4, proto_4_40);
        else synth_8_med(dst, src, V, costab_8, proto_8_80);
    }

#endif

static void a2dpSbcPlay(const uint8_t* buf, uint16_t len){  //only supports mono (for simplicity)

    //volume
    int32_t vol = (uint32_t)getVolume();

    //convenience
    const uint8_t* end = buf + len;
    #define left (end - buf)

    //workspace
    ITER i, j, k, ch, numChan;
    uint8_t scaleFactors[2][8];
    int8_t bitneed[2][8];
    uint8_t bits[2][8], join = 0;
    INSAMPLE samples[16][2][8];

    //audio data
    #define bufNumBuffers	3	//if less than 3, we're being suboptimal in speed.
    #define bufNumSamples	1536	//FYI: 128 samples is the maximum number of samples a single packet may have. MUST be a multiple of 8!
    static OUTSAMPLE audio[bufNumBuffers][bufNumSamples];
    static uint8_t whichBuf = 0;
    static uint16_t bufSamples = 0;

    //process the packet header
    if(left < 12) return;	//too short of a packet header
    uint8_t ver = (*buf) >> 6;
    char pad = ((*buf) >> 5) & 1;
    char ext = ((*buf) >> 4) & 1;
    uint8_t cc = (*buf++) & 0x0F;
    char mark = (*buf) >> 7;
    uint8_t pt = (*buf++) & 0x7F;
    uint16_t seq = (((uint16_t)buf[0]) << 8) | buf[1];
    uint32_t time = (((uint32_t)buf[2]) << 24) | (((uint32_t)buf[3]) << 16) | (((uint32_t)buf[4]) << 8) | buf[5];
    uint32_t src = (((uint32_t)buf[6]) << 24) | (((uint32_t)buf[7]) << 16) | (((uint32_t)buf[8]) << 8) | buf[9];
    buf += 10;

    //process the count
    if(left < 1) return;		//too short of a count
    uint8_t numFrames = *buf++;

    //process packets
    while(numFrames--){

        //process frame header
        if(left < 4) break;		//too short a frame header
        uint8_t sync = *buf++;
        uint8_t samplingRate = (*buf) >> 6;	//see A2DP table 12.16
        uint8_t blocks = ((*buf) >> 4) & 3;	//see A2DP table 12.17
        uint8_t chanMode = ((*buf) >> 2) & 3;	//see A2DP table 12.18
        uint8_t snr = ((*buf) >> 1) & 1;	//see A2DP table 12.19
        uint8_t numSubbands = (*buf++) & 1;	//see A2DP table 12.20
        uint8_t bitpoolSz = *buf++;
        uint8_t hdrCRC = *buf++;
        uint8_t bitpos = 0x80;

        numChan = (chanMode == A2DP_CHAN_MODE_MONO) ? 1 : 2;

        //process some numbers based on the tables
        numSubbands = numSubbands ? 8 : 4;
        blocks = (blocks + 1) << 2;

        //read "join" table if expected
        if(chanMode == A2DP_CHAN_MODE_JOINT_STEREO){
            join = *buf;	//we use it as a bitfield starting at top bit. 
            join >>= (8 - (numSubbands - 1));
            join <<= (8 - (numSubbands - 1));
            if(numSubbands == 8) buf++;
            else bitpos = 0x08;
        }

        //read scale factors
        for(ch = 0; ch < numChan; ch++) for(i = 0; i < numSubbands; i++){

            if(bitpos == 0x80){

                scaleFactors[ch][i] = (*buf) >> 4;
                bitpos = 0x08;
            }
            else{

                scaleFactors[ch][i] = (*buf++) & 0x0F;
                bitpos = 0x80;
            }
        }

        //calculate bitneed table and max_bitneed value (A2DP 12.6.3.1)
        int8_t max_bitneed[2] = {0, 0};
        for(ch = 0; ch < numChan; ch++){
            if(snr){

                for(i = 0; i < numSubbands; i++){

                    bitneed[ch][i] = scaleFactors[ch][i];
                    if(bitneed[ch][i] > max_bitneed[ch]) max_bitneed[ch] = bitneed[ch][i];
                }
            }
            else{

                const signed char* tbl;

                if(numSubbands == 4) tbl = loudness_4[samplingRate];
                else tbl = loudness_8[samplingRate];

                for(i = 0; i < numSubbands; i++){

                    if(scaleFactors[ch][i]){

                        int loudness = scaleFactors[ch][i] - tbl[i];

                        if(loudness > 0) loudness /= 2;
                        bitneed[ch][i] = loudness;
                    }
                    else bitneed[ch][i] = -5;
                    if(bitneed[ch][i] > max_bitneed[ch]) max_bitneed[ch] = bitneed[ch][i];
                }
            }
        }

        if(chanMode == A2DP_CHAN_MODE_MONO || chanMode == A2DP_CHAN_MODE_DUAL_CHANNEL){
            for(ch = 0; ch < numChan; ch++){
                //fit bitslices into the bitpool
                int32_t bitcount = 0, slicecount = 0, bitslice = max_bitneed[ch] + 1;
                do{
                    bitslice--;
                    bitcount += slicecount;
                    slicecount = 0;
                    for(i = 0; i < numSubbands; i++){

                        if(bitneed[ch][i] > bitslice + 1 && bitneed[ch][i] < bitslice + 16) slicecount++;
                        else if(bitneed[ch][i] == bitslice + 1) slicecount += 2;
                    }

                }while(bitcount + slicecount < bitpoolSz);

                //distribute bits
                for(i = 0; i < numSubbands; i++){

                    if(bitneed[ch][i] < bitslice + 2) bits[ch][i] = 0;
                    else{

                        int8_t v = bitneed[ch][i] - bitslice;
                        if(v > 16) v = 16;
                        bits[ch][i] = v;
                    }
                }

                //allocate remaining bits
                for(i = 0; i < numSubbands && bitcount < bitpoolSz; i++){

                    if(bits[ch][i] >= 2 && bits[ch][i] < 16){

                        bits[ch][i]++;
                        bitcount++;
                    }
                    else if(bitneed[ch][i] == bitslice + 1 && bitpoolSz > bitcount + 1){

                        bits[ch][i] = 2;
                        bitcount += 2;
                    }
                }
                for(i = 0; i < numSubbands && bitcount < bitpoolSz; i++){

                    if(bits[ch][i] < 16){

                        bits[ch][i]++;
                        bitcount++;
                    }
                }
            }
        }
        else{
            //calculate max_bitneed value (A2DP 12.6.3.2)
            uint8_t max_bitneed_val = max_bitneed[0] > max_bitneed[1] ? max_bitneed[0] : max_bitneed[1];

            //fit bitslices into the bitpool
            int32_t bitcount = 0, slicecount = 0, bitslice = max_bitneed_val + 1;
            do{
                bitslice--;
                bitcount += slicecount;
                slicecount = 0;
                for(ch = 0; ch < 2; ch++) for(i = 0; i < numSubbands; i++){

                    if(bitneed[ch][i] > bitslice + 1 && bitneed[ch][i] < bitslice + 16) slicecount++;
                    else if(bitneed[ch][i] == bitslice + 1) slicecount += 2;
                }

            }while(bitcount + slicecount < bitpoolSz);

            //distribute bits
            for(ch = 0; ch < 2; ch++) for(i = 0; i < numSubbands; i++){

                if(bitneed[ch][i] < bitslice + 2) bits[ch][i] = 0;
                else{

                    int8_t v = bitneed[ch][i] - bitslice;
                    if(v > 16) v = 16;
                    bits[ch][i] = v;
                }
            }

            //allocate remaining bits
            i = 0;
            ch = 0;
            while(i < numSubbands && bitcount < bitpoolSz){

                if(bits[ch][i] >= 2 && bits[ch][i] < 16){

                    bits[ch][i]++;
                    bitcount++;
                }
                else if(bitneed[ch][i] == bitslice + 1 && bitpoolSz > bitcount + 1){

                    bits[ch][i] = 2;
                    bitcount += 2;
                }
                if(++ch == 2){
                    ch = 0;
                    i++;
                }
            }
            i = 0;
            ch = 0;
            while(i < numSubbands && bitcount < bitpoolSz){

                if(bits[ch][i] < 16){

                    bits[ch][i]++;
                    bitcount++;
                }
                if(++ch == 2){
                    ch = 0;
                    i++;
                }
            }
        }

        //reconstruct subband samples (A2DP 12.6.4)
        #ifndef SPEED_OVER_ACCURACY
            int32_t levels[2][8];
            for(ch = 0; ch < numChan; ch++) for(i = 0; i < numSubbands; i++) levels[ch][i] = (1 << bits[ch][i]) - 1;
        #endif
        for(j = 0; j < blocks; j++){

            for(ch = 0; ch < numChan; ch++) for(i = 0; i < numSubbands; i++){

                if(bits[ch][i]){

                    uint32_t val = 0;
                    k = bits[ch][i];
                    do{

                        val <<= 1;
                        if(*buf & bitpos) val++;
                        if(!(bitpos >>= 1)){
                            bitpos = 0x80;
                            buf++;
                        }
                    }while(--k);

                    val = (val << 1) | 1;
                    val <<= scaleFactors[ch][i];

                    #ifdef SPEED_OVER_ACCURACY
                        val >>= bits[ch][i];
                    #else
                        val /= levels[ch][i];
                    #endif

                    val -= (1 << scaleFactors[ch][i]);

                    samples[j][ch][i] = SAMPLE_CVT(val);

                }
                else samples[j][ch][i] = SAMPLE_CVT(0);
            }

            //joint processing (if needed - if not needed join will be 0)
            ITER bf = join;
            for(i = 0; i < numSubbands; i++, bf <<= 1) if(bf & 0x80){

                INSAMPLE t = samples[j][0][i];

                samples[j][0][i] += samples[j][1][i];
                samples[j][1][i] = t - samples[j][1][i];
            }
        }

        //synthesis
        static FIXED V[160] = {0, };
        for(j = 0; j < blocks; j++){

            //do the actual synthesis
            synth(audio[whichBuf] + bufSamples, samples[j][0], numSubbands, V);
            for(k = 0; k < numSubbands; k++, bufSamples++) audio[whichBuf][bufSamples] = (((int32_t)audio[whichBuf][bufSamples]) * vol) >> 8;

            //if buffer is full, enqueue it
            if(bufSamples == bufNumSamples){

                audioAddBuffer(AUDIO_BT, audio[whichBuf++], bufSamples);
                
                if(whichBuf == bufNumBuffers) whichBuf = 0;
                bufSamples = 0;
            }
        }
        //if we used a byte partially, skip the rest of it, it is "padding"
        if(bitpos != 0x80) buf++;
        if(left < 0) dbgPrintf("A2DP: buffer over-read: %d\n", left);
    }
}

static void a2dpServiceDataRx(void* service, const uint8_t* req, uint16_t reqSz){

    A2DPstate* state = service;
    uint8_t trans, pktTyp, msgTyp, sigId, eid;
    uint8_t buf[16];
    uint16_t replSz = 0;

    if(IS_DATA_STATE(service)){

        if(DATA_STATE_ENCODE(service) == conns) a2dpSbcPlay(req, reqSz);	//only latest plays data
        return;
    }

    if(reqSz < 2){

        dbgPrintf("A2DP: packet too small\n");
        return;
    }

    trans = (req[0] & AVDTP_HDR_MASK_TRANS) >> AVDTP_HDR_SHIFT_TRANS;
    pktTyp = (req[0] & AVDTP_HDR_MASK_PKT_TYP) >> AVDTP_HDR_SHIFT_PKT_TYP;
    msgTyp = (req[0] & AVDTP_HDR_MASK_MSG_TYP) >> AVDTP_HDR_SHIFT_MSG_TYP;
    sigId = (req[1] & AVDTP_HDR_MASK_SIG_ID) >> AVDTP_HDR_SHIFT_SIG_ID;
    reqSz -= 2;
    req += 2;

    #if UGLY_SCARY_DEBUGGING_CODE
        dbgPrintf("A2DP (%x) data(trans %d, pktTyp %d msgTyp %d sigId %d %ub)\n", state->remChan, trans, pktTyp, msgTyp, sigId, reqSz);
    #endif

    switch(sigId){

        case AVDTP_SIG_DISCOVER:

            if(reqSz) dbgPrintf("A2DP: unexpected further data in AVDTP_SIG_DISCOVER\n");
            //packet header
            buf[0] = (trans << AVDTP_HDR_SHIFT_TRANS) | (AVDTP_PKT_TYP_SINGLE << AVDTP_HDR_SHIFT_PKT_TYP) | (AVDTP_MSG_TYP_ACCEPT << AVDTP_HDR_SHIFT_MSG_TYP);
            buf[1] = (sigId << AVDTP_HDR_SHIFT_SIG_ID);
            //return a single SEID info structure
            buf[2] = (MY_ENDPT_ID << AVDTP_SEID_NFO_SHIFT_SEID) | (0 << AVDTP_SEID_NFO_MASK_INUSE);
            buf[3] = (AVDTP_MEDIA_TYP_AUDIO << AVDTP_SEID_NFO_SHIFT_MEDIA_TYP) | (AVDTP_DIR_SINK << AVDTP_SEID_NFO_SHIFT_TYP);
            replSz = 4;
            break;

        case AVDTP_SIG_GET_CAPABILITIES:

            if(reqSz > 1) dbgPrintf("A2DP: unexpected further data in AVDTP_SIG_GET_CAPABILITIES\n");
            if(reqSz < 1){
                dbgPrintf("A2DP: not enough data in AVDTP_SIG_GET_CAPABILITIES\n");
                break;
            }
            eid = (*req++) >> 2;
            if(eid != MY_ENDPT_ID){
                dbgPrintf("A2DP: bad EID (%d) AVDTP_SIG_GET_CAPABILITIES\n", eid);
                break;
            }
            //packet header
            buf[0] = (trans << AVDTP_HDR_SHIFT_TRANS) | (AVDTP_PKT_TYP_SINGLE << AVDTP_HDR_SHIFT_PKT_TYP) | (AVDTP_MSG_TYP_ACCEPT << AVDTP_HDR_SHIFT_MSG_TYP);
            buf[1] = (sigId << AVDTP_HDR_SHIFT_SIG_ID);
            //capability 0 - media transport
            buf[2] = AVDTP_SVC_CAT_MEDIA_TRANSPORT;
            buf[3] = 0;
            //capability 1 - SDB codec (and its info)
            buf[4] = AVDTP_SVC_CAT_MEDIA_CODEC;
            buf[5] = 6; //generic data is 2 bytes, SBC data is 4 bytes (see A2DP spec section 4.3.2)
            buf[6] = (AVDTP_MEDIA_TYP_AUDIO << 4);
            buf[7] = A2DP_CODEC_TYP_SBC;
            buf[8] = 0xFF; //spec forces us to support all of these (even though ANDROID and LINUX both do not properly support mono [try it :)  ])
            buf[9] = 0xFF; //spec forces us to support all of these
            buf[10] = 0x02; //min allowed by spec
            buf[11] = 0x50; //nice high value
            replSz = 12;
            break;

        case AVDTP_SIG_SET_CONFIGURATION:

            if(reqSz < 8){ //no valid command is shorter
                dbgPrintf("A2DP: not enough data in AVDTP_SIG_SET_CONFIGURATION\n");
                break;
            }
            eid = (*req++) >> 2;
            if(eid != MY_ENDPT_ID){
                dbgPrintf("A2DP: bad EID (%d) AVDTP_SIG_GET_CAPABILITIES\n", eid);
                break;
            }
            req++; //skip INT EID - we don't care
            reqSz -= 2;
            while(reqSz >= 2){

                uint8_t cap = *req++;
                uint8_t len = *req++;
                reqSz -= 2;

                if(len > reqSz) break;
                reqSz -= len;

                if(cap == AVDTP_SVC_CAT_MEDIA_CODEC){

                    if(*req++ != (AVDTP_MEDIA_TYP_AUDIO << 4)){
                        dbgPrintf("A2DP: requested data is not audio\n");
                        break;
                    }
                    else if(*req++ != A2DP_CODEC_TYP_SBC){
                        dbgPrintf("A2DP: requested codec is not SBC\n");
                        break;
                    }
                    else{

                        uint8_t samplingRate = (*req) >> 4;
                        uint8_t channelMode = (*req++) & 0x0F;
                        uint8_t blockLen = (*req) >> 4;
                        uint8_t subbands = (*req >> 2) & 3;
                        uint8_t allocMethod = (*req++) & 3;
                        uint8_t minBitpool = *req++;
                        uint8_t maxBitpool = *req++;

                        switch(samplingRate){
                            case 1: state->samplingRate = 48000; break;
                            case 2: state->samplingRate = 44100; break;
                            case 4: state->samplingRate = 32000; break;
                            case 8: state->samplingRate = 16000; break;
                            default:
                                dbgPrintf("A2DP: invalid sampling rate: %d\n", samplingRate);
                                goto out;
                        }
                        switch(blockLen){
                            case 1: blockLen = 16; break;
                            case 2: blockLen = 12; break;
                            case 4: blockLen = 8; break;
                            case 8: blockLen = 4; break;
                            default:
                                dbgPrintf("A2DP: invalid block len: %d\n", blockLen);
                                goto out;
                        }
                        switch(subbands){
                            case 1: subbands = 8; break;
                            case 2: subbands = 4; break;
                            default:
                                dbgPrintf("A2DP: invalid num subbands: %d\n", subbands);
                                goto out;
                        }
                        switch(allocMethod){
                            case 1: allocMethod = 0; break;
                            case 2: allocMethod = 1; break;
                            default:
                                dbgPrintf("A2DP: invalid allocMethod: %d\n", allocMethod);
                                goto out;
                        }
                        static const char* strMode[] = {NULL, "Joint Stereo", "Stereo", NULL, "Dual Channel", NULL, NULL, NULL, "Mono"};
                        dbgPrintf("A2DP:\n\t* %u %s-framed frames / block\n\t* %u subbands/frame\n\t* %u KHz sampling rate\n\t* '%s' channel mode\n\t* %u-%u bits per piece per channel\n",
                            blockLen, allocMethod ? "SNR" : "Loudness", subbands,
                            state->samplingRate, strMode[channelMode], minBitpool, maxBitpool);
                    }
                }
                else req += len;
            }
            if(reqSz){ //no valid command is shorter
                dbgPrintf("A2DP: malformed config info in AVDTP_SIG_SET_CONFIGURATION\n");
            }
            else{
                //prepare reply
                buf[0] = (trans << AVDTP_HDR_SHIFT_TRANS) | (AVDTP_PKT_TYP_SINGLE << AVDTP_HDR_SHIFT_PKT_TYP) | (AVDTP_MSG_TYP_ACCEPT << AVDTP_HDR_SHIFT_MSG_TYP);
                buf[1] = (sigId << AVDTP_HDR_SHIFT_SIG_ID);
                replSz = 2;
                #if UGLY_SCARY_DEBUGGING_CODE
                    dbgPrintf("A2DP: %u samp/sec, %db block w/%d subbands in %s mode with pool sizes %d-%d\n",
                        state->samplingRate, state->blockLen, state->subbands, state->allocTypeSNR ? "SNR" : "Loudness",
                        state->minBitpool, state->maxBitpool);
                #endif
            }
            break;

        case AVDTP_SIG_OPEN:

            if(reqSz > 1) dbgPrintf("A2DP: unexpected further data in AVDTP_SIG_OPEN\n");
            if(reqSz < 1){
                dbgPrintf("A2DP: not enough data in AVDTP_SIG_OPEN\n");
                break;
            }
            eid = (*req++) >> 2;
            if(eid != MY_ENDPT_ID){
                dbgPrintf("A2DP: bad EID (%d) AVDTP_SIG_OPEN\n", eid);
                break;
            }
            state->needAudioConn = 1;
            //packet header
            buf[0] = (trans << AVDTP_HDR_SHIFT_TRANS) | (AVDTP_PKT_TYP_SINGLE << AVDTP_HDR_SHIFT_PKT_TYP) | (AVDTP_MSG_TYP_ACCEPT << AVDTP_HDR_SHIFT_MSG_TYP);
            buf[1] = (sigId << AVDTP_HDR_SHIFT_SIG_ID);
            replSz = 2;
            break;

        case AVDTP_SIG_START:

            if(reqSz > 1) dbgPrintf("A2DP: unexpected further data in AVDTP_SIG_OPEN\n");
            if(reqSz < 1){
                dbgPrintf("A2DP: not enough data in AVDTP_SIG_OPEN\n");
                break;
            }
            eid = (*req++) >> 2;
            if(eid != MY_ENDPT_ID){
                dbgPrintf("A2DP: bad EID (%d) AVDTP_SIG_OPEN\n", eid);
                break;
            }
            audioOn(AUDIO_BT, state->samplingRate);
            //packet header
            buf[0] = (trans << AVDTP_HDR_SHIFT_TRANS) | (AVDTP_PKT_TYP_SINGLE << AVDTP_HDR_SHIFT_PKT_TYP) | (AVDTP_MSG_TYP_ACCEPT << AVDTP_HDR_SHIFT_MSG_TYP);
            buf[1] = (sigId << AVDTP_HDR_SHIFT_SIG_ID);
            replSz = 2;
            break;

        case AVDTP_SIG_CLOSE:
        case AVDTP_SIG_SUSPEND:
        case AVDTP_SIG_ABORT:

            if(reqSz > 1) dbgPrintf("A2DP: unexpected further data in AVDTP_SIG_CLOSE/ABORT/SUSPEND\n");
            if(reqSz < 1){
                dbgPrintf("A2DP: not enough data in AVDTP_SIG_CLOSE/ABORT/SUSPEND\n");
                break;
            }
            eid = (*req++) >> 2;
            if(eid != MY_ENDPT_ID){
                dbgPrintf("A2DP: bad EID (%d) AVDTP_SIG_CLOSE/ABORT/SUSPEND\n", eid);
                break;
            }
            audioOff(AUDIO_BT);
            //packet header
            buf[0] = (trans << AVDTP_HDR_SHIFT_TRANS) | (AVDTP_PKT_TYP_SINGLE << AVDTP_HDR_SHIFT_PKT_TYP) | (AVDTP_MSG_TYP_ACCEPT << AVDTP_HDR_SHIFT_MSG_TYP);
            buf[1] = (sigId << AVDTP_HDR_SHIFT_SIG_ID);
            replSz = 2;
            break;

        default:

            dbgPrintf("A2DP (%x) data(trans %d, pktTyp %d msgTyp %d sigId %d %ub)\n", state->remChan, trans, pktTyp, msgTyp, sigId, reqSz);
            dbgPrintf("DATA: ");
            while(reqSz--) dbgPrintf(" %02X", *req++);
            dbgPrintf("\n");
            break;
    }

out:
    if(replSz){
        sg_buf* sgbuf = sg_alloc();
        if(!sgbuf) return;
        if(sg_add_front(sgbuf, buf, replSz, SG_FLAG_MAKE_A_COPY)){

            l2capServiceTx(state->aclConn, state->remChan, sgbuf);
        }
        else{

            sg_free(sgbuf);
            free(sgbuf);
        }
    }
}

static void* a2dpServiceAlloc(uint16_t conn, uint16_t chan, uint16_t remChan){

    A2DPstate* state = conns;

    while(state){

        if(state->needAudioConn && state->aclConn == conn){

            state->needAudioConn = 0;
            return DATA_STATE_ENCODE(state);
        }
        state = state->next;
    }
    
    state = malloc(sizeof(A2DPstate));

    if(state){

        state->aclConn = conn;
        state->remChan = remChan;
        state->needAudioConn = 0;

        state->next = conns;
        conns = state;
    }

    return state;
}

static void a2dpServiceFree(void* service){

    if(IS_DATA_STATE(service)) return;
    
    A2DPstate *state = conns, *prev = NULL;

    while(state){

        if(state == service){

            if(prev) prev->next = state->next;
            else conns = state->next;

            dbgPrintf("A2DP: connection %d.%d closed\n", state->aclConn, state->remChan);
            if(conns) audioOn(AUDIO_BT, conns->samplingRate);

            free(state);
            return;
        }

        prev = state;
        state = state->next;
    }
}

static uint8_t sdpDescrA2DP[] =
{
        //service class ID list
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x00, 0x01, SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 3,
            SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_2), 0x11, 0x0B,		//Audio Sink
        //ServiceId
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x00, 0x03, SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_2), 0x11, 0x0B,	//Audio Sink
        //ProtocolDescriptorList
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x00, 0x04, SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 16,
            SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 6,
                SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_2), 0x01, 0x00, // L2CAP
                SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), L2CAP_PSM_AVDTP >> 8, L2CAP_PSM_AVDTP & 0xFF, // AVDTP PSM
            SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 6,
                SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_2), L2CAP_PSM_AVDTP >> 8, L2CAP_PSM_AVDTP & 0xFF, // AVDTP
                SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x01, 0x00,	//AVDTP version (as per spec)
        //browse group list
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x00, 0x05, SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 3,
            SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_2), 0x10, 0x02, // Public Browse Group
        //profile descriptor list
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x00, 0x09, SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 8,
            SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 6,
                SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_2), 0x11, 0x0d, // Advanced Audio
                SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x01, 0x00, // Version 1.0
        //name
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x01, 0x00, SDP_ITEM_DESC(SDP_TYPE_TEXT, SDP_SZ_u8), 8, 'A', 'D', 'K', ' ', 'A', '2', 'D', 'P'
};


char btA2dpRegister(void){

    const L2capService a2dp = {L2CAP_FLAG_SUPPORT_CONNECTIONS, a2dpServiceAlloc, a2dpServiceFree, a2dpServiceDataRx};
    
    if(!l2capServiceRegister(L2CAP_PSM_AVDTP, &a2dp)) return 0;
    btSdpServiceDescriptorAdd(sdpDescrA2DP, sizeof(sdpDescrA2DP));

    return 1;
}




/* original tables


proto_4_40:
	0.00000000E+00,5.36548976E-04,1.49188357E-03,2.73370904E-03,
	3.83720193E-03,3.89205149E-03,1.86581691E-03,-3.06012286E-03,
	1.09137620E-02,2.04385087E-02,2.88757392E-02,3.21939290E-02,
	2.58767811E-02,6.13245186E-03,-2.88217274E-02,-7.76463494E-02,
	1.35593274E-01,1.94987841E-01,2.46636662E-01,2.81828203E-01,
	2.94315332E-01,2.81828203E-01,2.46636662E-01,1.94987841E-01,
	-1.35593274E-01,-7.76463494E-02,-2.88217274E-02,6.13245186E-03,
	2.58767811E-02,3.21939290E-02,2.88757392E-02,2.04385087E-02,
	-1.09137620E-02,-3.06012286E-03,1.86581691E-03,3.89205149E-03,
	3.83720193E-03,2.73370904E-03,1.49188357E-03,5.36548976E-04

proto_8_80:

	0.00000000E+00,1.56575398E-04,3.43256425E-04,5.54620202E-04,
	8.23919506E-04,1.13992507E-03,1.47640169E-03,1.78371725E-03,
	2.01182542E-03,2.10371989E-03,1.99454554E-03,1.61656283E-03,
	9.02154502E-04,-1.78805361E-04,-1.64973098E-03,-3.49717454E-03,
	5.65949473E-03,8.02941163E-03,1.04584443E-02,1.27472335E-02,
	1.46525263E-02,1.59045603E-02,1.62208471E-02,1.53184106E-02,
	1.29371806E-02,8.85757540E-03,2.92408442E-03,-4.91578024E-03,
	-1.46404076E-02,-2.61098752E-02,-3.90751381E-02,-5.31873032E-02,
	6.79989431E-02,8.29847578E-02,9.75753918E-02,1.11196689E-01,
	1.23264548E-01,1.33264415E-01,1.40753505E-01,1.45389847E-01,
	1.46955068E-01,1.45389847E-01,1.40753505E-01,1.33264415E-01,
	1.23264548E-01,1.11196689E-01,9.75753918E-02,8.29847578E-02,
	-6.79989431E-02,-5.31873032E-02,-3.90751381E-02,-2.61098752E-02,
	-1.46404076E-02,-4.91578024E-03,2.92408442E-03,8.85757540E-03,
	1.29371806E-02,1.53184106E-02,1.62208471E-02,1.59045603E-02,
	1.46525263E-02,1.27472335E-02,1.04584443E-02,8.02941163E-03,
	-5.65949473E-03,-3.49717454E-03,-1.64973098E-03,-1.78805361E-04,
	9.02154502E-04,1.61656283E-03,1.99454554E-03,2.10371989E-03,
	2.01182542E-03,1.78371725E-03,1.47640169E-03,1.13992507E-03,
	8.23919506E-04,5.54620202E-04,3.43256425E-04,1.56575398E-04


js code to convert to fixpoint:

	var xa = new Array(values here...);

	var num = 0;
	var perRow = 4;
	var L = parseInt(xa.length);

	for(i = 0; i < L; i++){
		x = xa[i];

		var neg = 0;


		if(x < 0){
		  neg = 1;
		  x = -x;
		}
		x *= (1 << 26);   //this 26 should be the number of fraction bits
		x = parseInt(x + 0.5);
		s = x >> 28
		x &= 0x0FFFFFFF;
		if(neg){

			x = x ^ 0x0FFFFFFF;
			x++;
			s ^= 0x0F;
			if(x & 0x10000000) s++;
			x &= 0x0FFFFFFF;
			s &= 0x0F;
		}

		x = x.toString(16);
		while(x.length < 7) x = "0" + x;
		x = s.toString(16) + x;
		x = x.toUpperCase();


		document.write("0x" + x);
		if(i != L - 1) document.write(",");
		if(++num == perRow){
			num = 0;
			document.write("<BR>");
		}
		else document.write(" ");
	}



js code to produce costab (adjust loop variables as needed for both table vairants)

	for(k = 0; k <= 7; k++) for(i = 0 ;i <= 3; i++){

		document.write(Math.cos((i + 0.5) * (k + 2) * Math.PI / 4) + ", ")
	}



js code to generate order tables (they are used for those strange offsets into the V array in synth_* when generating samples):

	var L = 200;
	var V = new Array(L);
	var U = new Array(80);
	var i, j;
	var nBands = 4;

	for(i = 0; i <L; i++) V[i] = i + 1;


	for(i = 0; i <= 4; i++) for(j = 0; j < nBands; j++){

		if(nBands == 4){

			U[i * 8 + j] = V[i * 16 + j];
			U[i * 8 + 4 + j] = V[i * 16 + 12 + j];
		}
		else{

			U[i * 16 + j] = V[i * 32 + j];
			U[i * 16 + 8 + j] = V[i * 32 + 24 + j];
		}
	}


	for(j = 0; j < nBands; j++) for(i = 0; i < 10; i++) document.write((U[j + nBands * i] - 1) + ",\t");







C code for reordering the proto_* tables to access order. insert table values and modify nBands as needed

	#include <stdio.h>
	#include <stdint.h>



	int32_t tabl[] =
	{
		table data here
	};


	int main(int argc, char** argv){

		int i, j;
		int nBands = 8;

		for(j = 0; j < nBands; j++) for(i = 0; i < 10; i++) printf("0x%08X, ",tabl[j + nBands * i]);
	}



*/

