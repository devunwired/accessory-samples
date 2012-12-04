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
#include <ADK.h>
#include <HCI.h>


ADK L;

extern "C"{
 void dbgPrintf(const char* fmt,...);  //arduino print sucks... 
 void* malloc(unsigned sz);  //who the hell programs BT without these?
 void free(void*);  //who the hell programs BT without these?
}



void adkPutchar(char c){
 Serial.write(c);
}

void setup(void)
{

 Serial.begin(115200);
 
 L.adkSetPutchar(adkPutchar);
 L.adkInit();
}

typedef struct BTD{

    struct BTD* next;
    uint8_t mac[BLUETOOTH_MAC_SIZE];
    uint8_t PSRM;
    uint8_t PSPM;
    uint8_t PSM;
    uint16_t CO;
    uint32_t dc;

}BTD;

BTD* devs;

char adkBtDiscoveryResult(const uint8_t* mac, uint8_t PSRM, uint8_t PSPM, uint8_t PSM, uint16_t CO, uint32_t devClass){ //return 0 to stop scan immediately

    int i;
    static int nres = 0;

    dbgPrintf("%d results so far...\n", ++nres);

    BTD* btd = devs;
    while(btd){

        if(btd->mac[0] == mac[0] && btd->mac[1] == mac[1] && btd->mac[2] == mac[2] && 
		btd->mac[3] == mac[3] && btd->mac[4] == mac[4] && btd->mac[5] == mac[5]) return 1;
        btd = btd->next;
    }

    btd = (BTD*)malloc(sizeof(BTD));
    if(!btd) return 0;

    for(i = 0; i < BLUETOOTH_MAC_SIZE; i++) btd->mac[i] = mac[i];
    btd->PSRM = PSRM;
    btd->PSPM = PSPM;
    btd->PSM = PSM;
    btd->CO = CO;
    btd->dc = devClass;
    btd->next = devs;
    devs = btd;
    return 1;
}

void doScan(){

    devs = NULL;

    dbgPrintf("BT Scan...\n");
    L.btScan();
    dbgPrintf("\n");

    BTD* dev = devs;

    while(dev){

        dbgPrintf("%02X:%02X:%02X:%02X:%02X:%02X - ", dev->mac[5], dev->mac[4], dev->mac[3], dev->mac[2], dev->mac[1], dev->mac[0]);
        dbgPrintf("{%d,%d,%d} 0x%06X %6d - ", dev->PSRM, dev->PSPM, dev->PSM, dev->dc, dev->CO);

        switch((dev->dc & DEVICE_CLASS_MAJOR_MASK) >> DEVICE_CLASS_MAJOR_SHIFT){

            case DEVICE_CLASS_MAJOR_MISC:
                dbgPrintf("MISC");
                break;

            case DEVICE_CLASS_MAJOR_COMPUTER:
                dbgPrintf("Computer - ");

                switch((dev->dc & DEVICE_CLASS_MINOR_COMPUTER_MASK) >> DEVICE_CLASS_MINOR_COMPUTER_SHIFT){

                    case DEVICE_CLASS_MINOR_COMPUTER_UNCATEG:
                        dbgPrintf("uncategorized");
                        break;
                    case DEVICE_CLASS_MINOR_COMPUTER_DESKTOP:
                        dbgPrintf("desktop");
                        break;
                    case DEVICE_CLASS_MINOR_COMPUTER_SERVER:
                        dbgPrintf("server");
                        break;
                    case DEVICE_CLASS_MINOR_COMPUTER_LAPTOP:
                        dbgPrintf("laptop");
                        break;
                    case DEVICE_CLASS_MINOR_COMPUTER_CLAM_PDA:
                        dbgPrintf("clamsghell PDA");
                        break;
                    case DEVICE_CLASS_MINOR_COMPUTER_PALM_PDA:
                        dbgPrintf("Palm PDA");
                        break;
                    case DEVICE_CLASS_MINOR_COMPUTER_WEARABLE:
                        dbgPrintf("wearable");
                        break;
                    default:
                        dbgPrintf("unknown minor %d", (dev->dc & DEVICE_CLASS_MINOR_COMPUTER_MASK) >> DEVICE_CLASS_MINOR_COMPUTER_SHIFT);
                        break;
                }
                break;

            case DEVICE_CLASS_MAJOR_PHONE:
                dbgPrintf("Phone - ");
                switch((dev->dc & DEVICE_CLASS_MINOR_PHONE_MASK) >> DEVICE_CLASS_MINOR_PHONE_SHIFT){

                    case DEVICE_CLASS_MINOR_PHONE_UNCATEG:
                        dbgPrintf("uncategorized");
                        break;
                    case DEVICE_CLASS_MINOR_PHONE_CELL:
                        dbgPrintf("Cellular");
                        break;
                    case DEVICE_CLASS_MINOR_PHONE_CORDLESS:
                        dbgPrintf("Cordless");
                        break;
                    case DEVICE_CLASS_MINOR_PHONE_SMART:
                        dbgPrintf("Smartphone");
                        break;
                    case DEVICE_CLASS_MINOR_PHONE_MODEM:
                        dbgPrintf("Modem");
                        break;
                    case DEVICE_CLASS_MINOR_PHONE_ISDN:
                        dbgPrintf("ISDN");
                        break;
                    default:
                        dbgPrintf("unknown minor %d", (dev->dc & DEVICE_CLASS_MINOR_PHONE_MASK) >> DEVICE_CLASS_MINOR_PHONE_SHIFT);
                        break;
                }
                break;

            case DEVICE_CLASS_MAJOR_LAN:
                dbgPrintf("LAN");
                break;

            case DEVICE_CLASS_MAJOR_AV:
                dbgPrintf("A/V - ");
                switch((dev->dc & DEVICE_CLASS_MINOR_AV_MASK) >> DEVICE_CLASS_MINOR_AV_SHIFT){

                    case DEVICE_CLASS_MINOR_AV_UNCATEG:
                        dbgPrintf("uncategorized");
                        break;
                    case DEVICE_CLASS_MINOR_AV_HEADSET:
                        dbgPrintf("Headset");
                        break;
                    case DEVICE_CLASS_MINOR_AV_HANDSFREE:
                        dbgPrintf("Handsfree");
                        break;
                    case DEVICE_CLASS_MINOR_AV_MIC:
                        dbgPrintf("Microphone");
                        break;
                    case DEVICE_CLASS_MINOR_AV_LOUDSPEAKER:
                        dbgPrintf("Loudspeaker");
                        break;
                    case DEVICE_CLASS_MINOR_AV_HEADPHONES:
                        dbgPrintf("Headphones");
                        break;
                    case DEVICE_CLASS_MINOR_AV_PORTBL_AUDIO:
                        dbgPrintf("Portable Audio");
                        break;
                    case DEVICE_CLASS_MINOR_AV_CAR_AUDIO:
                        dbgPrintf("Car audio");
                        break;
                    case DEVICE_CLASS_MINOR_AV_SET_TOP_BOX:
                        dbgPrintf("Set-Top Box");
                        break;
                    case DEVICE_CLASS_MINOR_AV_HIFI:
                        dbgPrintf("Hi-Fi");
                        break;
                    case DEVICE_CLASS_MINOR_AV_VCR:
                        dbgPrintf("VCR");
                        break;
                    case DEVICE_CLASS_MINOR_AV_VID_CAM:
                        dbgPrintf("Vidoe Camera");
                        break;
                    case DEVICE_CLASS_MINOR_AV_CAMCORDER:
                        dbgPrintf("Camcorder");
                        break;
                    case DEVICE_CLASS_MINOR_AV_VID_MONITOR:
                        dbgPrintf("Video Monitor");
                        break;
                    case DEVICE_CLASS_MINOR_AV_DISPLAY_AND_SPKR:
                        dbgPrintf("Display With Speakers");
                        break;
                    case DEVICE_CLASS_MINOR_AV_VC:
                        dbgPrintf("Video conferencing device");
                        break;
                    case DEVICE_CLASS_MINOR_AV_TOY:
                        dbgPrintf("Toy");
                        break;
                    default:
                        dbgPrintf("unknown minor %d", (dev->dc & DEVICE_CLASS_MINOR_AV_MASK) >> DEVICE_CLASS_MINOR_AV_SHIFT);
                        break;
                }
                break;

            case DEVICE_CLASS_MAJOR_IMAGING:
                dbgPrintf("Imaging");
                break;

            case DEVICE_CLASS_MAJOR_WEARABLE:
                dbgPrintf("Wearable");
                break;

            case DEVICE_CLASS_MAJOR_TOY:
                dbgPrintf("Toy");
                break;

            case DEVICE_CLASS_MAJOR_HEALTH:
                dbgPrintf("Health");
                break;

            case DEVICE_CLASS_MAJOR_UNCATEGORIZED:
                dbgPrintf("Uncategorize");
                break;

            default:
                dbgPrintf("UNKNOWN_MAJOR_%d", (dev->dc & DEVICE_CLASS_MAJOR_MASK) >> DEVICE_CLASS_MAJOR_SHIFT);
                break;
        }

        char name[249];
        if(L.btGetRemoteName(dev->mac, dev->PSRM, dev->PSM, dev->CO, name)) dbgPrintf(" \"%s\"", name);
        else dbgPrintf("{FAILED TO GET DEVICE NAME}");

        dbgPrintf("\n");

        devs = dev;
        dev = dev->next;
        free(devs);
    }
    dbgPrintf("Scan done\n");
}

void loop(void)
{
 
 L.btEnable(0, 0, 0, 0, adkBtDiscoveryResult);
 
 while(1) {
   
     L.adkEventProcess();
     doScan();
 }
}
