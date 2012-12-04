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
#include "btSDP.h"
#include "btL2CAP.h"
#include "dbg.h"
#include <string.h>



static void elzSetup(void);
static char* elzTalk(char* Is);

#define MAGIX	0xFA

static uint8_t sdpDescrEliza[] =  //we are connectible to on unsecured channel of ADK chat app
{
        //service class ID list
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x00, 0x01, SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 17,
            SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_16), 0x8c, 0xe2, 0x55, 0xc0, 0x20, 0x0a, 0x11, 0xe0, 0xac, 0x64, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66,
        //ServiceId
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x00, 0x03, SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_2), 0x11, 0x01,
        //ProtocolDescriptorList
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x00, 0x04, SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 15,
            SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 6,
                SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_2), 0x01, 0x00, // L2CAP
                SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), L2CAP_PSM_RFCOMM >> 8, L2CAP_PSM_RFCOMM & 0xFF, // L2CAP PSM
            SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 5,
                SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_2), 0x00, 0x03, // RFCOMM
                SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_1), MAGIX, // port ###
        //browse group list
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x00, 0x05, SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 3,
            SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_2), 0x10, 0x02, // Public Browse Group
        //name
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x01, 0x00, SDP_ITEM_DESC(SDP_TYPE_TEXT, SDP_SZ_u8), 5, 'E', 'L', 'I', 'Z', 'A'
};


static void elzPortOpen(void* port, uint8_t dlci){

    dbgPrintf("Remote client joined...\n");
    elzSetup();
}

static void elzPortClose(void* port, uint8_t dlci){

    dbgPrintf("Remote client left...\n");
}

static void elzPortRx(void* port, uint8_t dlci, const uint8_t* data, uint16_t sz){

    char *c, *r;

    while(data[sz - 1] == '\n') sz--;

    c = malloc(sz + 1);
    if(c){

        memcpy(c, data, sz);
        c[sz] = 0;
        r = elzTalk(c);
        if(r){
            btRfcommPortTx(port, dlci, r, strlen(r));
            free(r);
            return;
        }
    }
    btRfcommPortTx(port, dlci, "OUT OF MEMORY", 13);
}

void eliza(void){

    uint8_t i, dlci = btRfcommReserveDlci(RFCOMM_DLCI_NEED_EVEN);
    int f;

    if(!dlci) dbgPrintf("ELIZA: failed to allocate DLCI\n");
    else{

        //change descriptor to be valid...
        for(i = 0, f = -1; i < sizeof(sdpDescrEliza); i++){

            if(sdpDescrEliza[i] == MAGIX){
                if(f == -1) f = i;
                else break;
            }
        }

        if(i != sizeof(sdpDescrEliza) || f == -1){

            dbgPrintf("ELIZA: failed to find a single marker in descriptor\n");
            btRfcommReleaseDlci(dlci);
            return;
        }

        sdpDescrEliza[f] = dlci >> 1;

        btRfcommRegisterPort(dlci, elzPortOpen, elzPortClose, elzPortRx);
        btSdpServiceDescriptorAdd(sdpDescrEliza, sizeof(sdpDescrEliza));
    }
}



/////////////////utils

char* cat(char* a, char *b){

    char* r = malloc(strlen(a) + strlen(b) + 1);

    strcpy(r, a);
    strcat(r, b);
    free(a);
    free(b);
    return r;
}

#define conststr(s)	strdup(s)
#define dup(s)		strdup(s)

char streq(char* a, char* b){

    char ret = !strcmp(a,b);

    free(a);
    free(b);

    return ret;
}

char* mid(char* str, long start, long len){

    char* r = malloc(len + 1);
    memcpy(r, str + start, len);
    r[len] = 0;

    return r;
}

char* right(char* str, long len){

    if(strlen(str) < len) len = strlen(str);

    return mid(str, strlen(str) - len, len);
}









///////////////////////////////////////////////////////////////////////////
// ORIGINAL: http://www.vintagecomputer.net/commodore/64/TOK64/ELIZA.txt //
//         BASIC TO C PORT: Dmitry Grinberg (dmitrygr@gmail.com)         //
///////////////////////////////////////////////////////////////////////////



static const char* data0[] =
{
	"CAN YOU","CAN I","YOU ARE","YOURE","I DONT","I FEEL",
	"WHY DONT YOU","WHY CANT I","ARE YOU","I CANT","I AM"," IM ",
	"YOU","I WANT","WHAT","HOW","WHO","WHERE","WHEN","WHY",
	"NAME","CAUSE","SORRY","DREAM","HELLO","HI","MAYBE",
	"NO","YOUR","ALWAYS","THINK","ALIKE","YES","FRIEND",
	"COMPUTER","NOKEYFOUND"
};

static const char* data1[] =
{
	" ARE "," AM ","WERE ","WAS "," YOU "," I ","YOUR ","MY ",
	" IVE "," YOUVE "," IM "," YOURE ", " YOU ", " ME "
};

static const char* data2[] =
{
	"DON'T YOU BELIEVE THAT I CAN.","PERHAPS YOU WOULD LIKE TO BE ABLE TO.",
	"YOU WANT ME TO BE ABLE TO*","PERHAPS YOU DON'T WANT TO*",
	"DO YOU WANT TO BE ABLE TO*","WHAT MAKES YOU THINK I AM*",
	"DOES IT PLEASE YOU TO BELIEVE I AM*","PERHAPS YOU WOULD LIKE TO BE*",
	"DO YOU SOMETIMES WISH YOU WERE*","DON'T YOU REALLY*","WHY DON'T YOU*",
	"DO YOU WISH TO BE ABLE TO*","DOES THAT TROUBLE YOU?",
	"TELL ME MORE ABOUT SUCH FEELINGS*","DO YOU OFTEN FEEL*",
	"DO YOU ENJOY FEELING*","DO YOU REALLY BELIEVE I DON'T*",
	"PERHAPS IN GD TIME I WILL*","DO YOU WANT ME TO*",
	"DO YOU THINK YOU SHOULD BE ABLE TO*","WHY CAN'T YOU*",
	"WHY ARE YOU INTERESTED IN WHETHER OR NOT I AM*",
	"WOULD YOU PREFER IF I WERE NOT*","PERHAPS IN YOUR FANTASIES I AM*",
	"HOW DO YOU KNOW YOU CAN'T*","HAVE YOU TRIED?","PERHAPS YOU CAN NOW*",
	"DID YOU COME TO ME BECAUSE YOU ARE*","HOW LONG HAVE YOU BEEN*",
	"DO YOU BELIEVE IT IS NORMAL TO BE*","DO YOU ENJOY BEING*",
	"WE WERE DISCUSSING YOU-- NOT ME.","OH, I*",
	"YOU'RE NOT REALLY TALKING ABOUT ME. ARE YOU?",
	"WHAT WOULD IT MEAN TO YOU IF YOU GOT*","WHY DO YOU WANT*",
	"SUPPOSE YOU SOON GOT*","WHAT IF YOU NEVER GOT*","I SOMETIMES ALSO WANT*",
	"WHY DO YOU ASK?","DOES THAT QUESTION INTEREST YOU?",
	"WHAT ANSWER WOULD PLEASE YOU THE MOST?","WHAT DO YOU THINK?",
	"ARE SUCH QUESTIONS ON YOUR MIND OFTEN?",
	"WHAT IS IT THAT YOU REALLY WANT TO KNOW?","HAVE YOU ASKED ANYONE ELSE?",
	"HAVE YOU ASKED SUCH QUESTIONS BEFORE?",
	"WHAT ELSE COMES TO MIND WHEN YOU ASK THAT?","NAMES DON'T INTEREST ME.",
	"I DON'T CARE ABOUT NAMES-- PLEASE GO ON.","IS THAT THE REAL REASON?",
	"DON'T ANY OTHER REASONS COME TO MIND?",
	"DOES THAT REASON EXPLAIN ANYTHING ELSE?","WHAT OTHER REASONS MIGHT THERE BE?",
	"PLEASE DON'T APOLOGIZE.","APOLOGIES ARE NOT NECESSARY.",
	"WHAT FEELINGS DO YOU HAVE WHEN YOU APOLOGIZE.","DON'T BE SO DEFENSIVE!",
	"WHAT DOES THAT DREAM SUGGEST TO YOU?","DO YOU DREAM OFTEN?",
	"WHAT PERSONS APPEAR IN YOUR DREAMS?","ARE YOU DISTURBED BY YOUR DREAMS?",
	"HOW DO YOU DO .,. PLEASE STATE YOUR PROBLEM.","YOU DON'T SEEM QUITE CERTAIN.",
	"WHY THE UNCERTAIN TONE?","CAN'T YOU BE MORE POSITIVE?","YOU AREN'T SURE?",
	"DON'T YOU KNOW?","ARE YOU SAYING NO JUST TO BE NEGATIVE?",
	"YOU ARE BEING A BIT NEGATIVE.","WHY NOT?","ARE YOU SURE?","WHY NO?",
	"WHY ARE YOU CONCERNED ABOUT MY*","WHAT ABOUT YOUR OWN*",
	"CAN'T YOU THINK OF A SPECIFIC EXAMPLE?","WHEN?","WHAT ARE YOU THINKING OF?",
	"REALLY. ALWAYS?","DO YOU REALLY THINK SO?","BUT YOU ARE NOT SURE YOU.",
	"DO YOU DOUBT YOU.","IN WHAT WAY?","WHAT RESEMBLANCE DO YOU SEE?",
	"WHAT DOES THE SIMILARITY SUGGEST TO YOU?",
	"WHAT OTHER CONNECTIONS DO YOU SEE?","COULD THERE REALLY BE SOME CONNECTION?",
	"HOW?","YOU SEEM QUITE POSITIVE.","ARE YOU SURE?","I SEE.","I UNDERSTAND.",
	"WHY DO YOU BRING UP THE TOPIC OF FRIENDS?","DO YOUR FRIENDS WORRY YOU?",
	"DO YOUR FRIENDS PICK ON YOU?","ARE YOU SURE YOU HAVE ANY FRIENDS?",
	"DO YOU IMPOSE ON YOUR FRIENDS?","PERHAPS YOUR LOVE FOR FRIENDS WORRIES YOU.",
	"DO COMPUTERS WORRY YOU?","ARE YOU TALKING ABOUT ME IN PARTICULAR?",
	"ARE YOU FRIGHTENED BY MACHINES?","WHY DO YOU MENTION COMPUTERS?",
	"WHAT DO YOU THINK MACHINES HAVE TO DO WITH YOUR PROBLEM?",
	"DON'T YOU THINK COMPUTERS CAN HELP PEOPLE?",
	"WHAT IS IT ABOUT MACHINES THAT WORRIES YOU?",
	"SAY, DO YOU HAVE ANY PSYCHOLOGICAL PROBLEMS?",
	"WHAT DOES THAT SUGGEST TO YOU?","I SEE.",
	"I'M NOT SURE I UNDERSTAND YOU FULLY.","COME COME ELUCIDATE YOUR THOUGHTS.",
	"CAN YOU ELABORATE ON THAT?","THAT IS QUITE INTERESTING."
};

static const char data3[] =
{
	1,3,4,2,6,4,6,4,10,4,14,3,17,3,20,2,22,3,25,3,
	28,4,28,4,32,3,35,5,40,9,40,9,40,9,40,9,40,9,40,9,
	49,2,51,4,55,4,59,4,63,1,63,1,64,5,69,5,74,2,76,4,
	80,3,83,7,90,3,93,6,99,7,106,6	
};


char S[36], R[36], N[36];
const char N1 = 36, N2 = 14, N3 = 112;
char* Ps = NULL;

static void elzSetup(void){

	char X;
	const char* ptr = data3;
	
	for(X = 0; X < N1; X++){
	
		R[X] = S[X] = *ptr++;
		N[X] = S[X] - 1 + *ptr++;
	}
}

static char* elzTalk(char* Is){

	const char *Fs;
	char *Cs = NULL;
	char result[128] = {0};
	int X, Si, K, L, T;
	
	//-----USER INPUT SECTION-----
	// * UPCASE
	// * GET RID OF APOSTROPHES
	// * ADD SPACES IN FRONT AND BACK
	{
		char _c, *_s, *_t;
		
		_s = malloc(strlen(Is) + 3);
		_t = Is;
		X = 0;
		_s[X++] = ' ';
		
		while((_c = *_t++)){
			
			if(_c >= 'a' && _c <= 'z') _c += 'A' - 'a';
			if(_c == '\'') continue;
			_s[X++] = _c;
		}
		_s[X++] = ' ';
		_s[X] = 0;
		free(Is);
		Is = _s;
		
		if(strstr(Is, "SHUT")) strcat(result, "SHUT UP...\n");
		
		if(Ps && !strcmp(Is, Ps)){
		
			strcat(result, "PLEASE DON'T REPEAT YOURSELF!\n");
			goto out;
		}
	}

	//-----FIND KEYWORD IN I$-----
	{
	
		const char* Ks;
		const char* _s;
		
		Si = -1;
		for(K = 0; K < N1; K++){
			if(Si >= 0) continue;
			for(L = 0; L <= (signed)strlen(Is) - (signed)strlen(data0[K]); L++){
			
				if(!memcmp(Is + L, data0[K], strlen(data0[K]))){
				
					Si = K;
					T = L;
					Fs = data0[K];	
				}
			}
		}
		
		if(Si < 0) K = 35;//WE DIDN'T FIND ANY KEYWORDS
		else{
		
			const char *Rs, *Ss;
		
			K = Si;
			L = T;
			
			//TAKE RIGHT PART OF STRING AND CONJUGATE IT
			//USING THE LIST OF STRINGS TO BE SWAPPED
			
			Cs = malloc(strlen(Is) - strlen(Fs) - L + 3);
			sprintf(Cs, " %s", Is + L + strlen(Fs));	//maybe + 1
			if(Ps) free(Ps);
			Ps = Is;
			Is = NULL;
			
			for(X = 0; X < N2 / 2; X++){
				
				Ss = data1[X * 2 + 0];
				Rs = data1[X * 2 + 1];
				
				for(L = 0; L < strlen(Cs); L++){
					
					char _i;
					const char* _f = Ss;
					const char* _r = Rs;
					
					for(_i = 0; _i < 2; _i++){
					
						if(!memcmp(Cs + L, _f, strlen(_f))){
							Is = malloc(strlen(Cs) - strlen(_f) + strlen(_r) + 1);
							
							memmove(Is, Cs, L);
							strcpy(Is + L, _r);
							strcat(Is, Cs + L + strlen(_f));
							free(Cs);
							Cs = Is;
							Is = NULL;
												
							L += strlen(_r);
							break;
						}
						_f = Rs;
						_r = Ss;
					}
				}
			}
			
			if(Cs[0] == ' ' && Cs[1] == ' ') memmove(Cs, Cs + 1, strlen(Cs)); //ONLY 1 SPACE
		}
	}
	
	//NOW USING THE KEYWORD NUMBER (K) GET REPLY
	{
	
		//READ RIGHT REPLY
		Fs = data2[R[K] - 1];
		R[K]++;
		if(R[K] > N[K]) R[K] = S[K];
		
		strcat(result, Fs);
		if(result[strlen(result) - 1] == '*'){
		
			result[strlen(result) - 1] = 0;
			strcat(result, Cs);
		}
	}
	if(Cs) free(Cs);
	
out:
	if(Is) free(Is);
	return strdup(result);
}



