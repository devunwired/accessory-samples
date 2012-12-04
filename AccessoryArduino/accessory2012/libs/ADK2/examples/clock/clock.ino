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

//android app needs to match this
#define BT_ADK_UUID	0x1d, 0xd3, 0x50, 0x50, 0xa4, 0x37, 0x11, 0xe1, 0xb3, 0xdd, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66


void adkPutchar(char c){Serial.write(c);}
extern "C" void dbgPrintf(const char *, ... );

enum AdkStates{
  AdkClock,
  AdkAlarm,
  AdkBrightness,
  AdkColor,
  AdkVolume,
  AdkDisplay,
  AdkPresets,
  
  //always last
  AdkInvalid
};

enum AdkDisplayMode{

  AdkShowAnimation,
  AdkShowAccel,
  AdkShowMag,
  AdkShowTemp,
  AdkShowHygro,
  AdkShowBaro,
  AdkShowProx,
  AdkShowColor,

  AdkShowLast	//always last
};

enum AdkAlarmStates {
  AdkAlarmIdle,
  AdkAlarmIdleWait,
  AdkAlarmAlarm,
  AdkAlarmSnooze
};
/*

  button numbering (all in hex)

    __   __   __   __   __   __
   |1A| |18| |16| |14| |12| |11|     03  04  05  06
    --   --   --   --   --   --      0A  09  08  07
   |1B| |19| |17| |15| |13| |10|
    --   --   --   --   --   --

*/

#define BTN_SLIDER_0   0x00
#define BTN_SLIDER_1   0x01
#define BTN_SLIDER_2   0x02
#define BTN_0_UP       0x1A
#define BTN_1_UP       0x18
#define BTN_2_UP       0x16
#define BTN_3_UP       0x14
#define BTN_4_UP       0x12
#define BTN_5_UP       0x11
#define BTN_0_DN       0x1B
#define BTN_1_DN       0x19
#define BTN_2_DN       0x17
#define BTN_3_DN       0x15
#define BTN_4_DN       0x13
#define BTN_5_DN       0x10
#define BTN_CLOCK      0x03
#define BTN_ALARM      0x04
#define BTN_BRIGHTNESS 0x05
#define BTN_COLOR      0x06
#define BTN_LOCK       0x07
#define BTN_PRESETS    0x08
#define BTN_DISPLAY    0x09
#define BTN_VOLUME     0x0A



#define BTN_MASK_PRESS     0x8000
#define BTN_MASK_HOLD      0x4000
#define BTN_MASK_RELEASE   0x2000
#define BTN_MASK_ID        0x003F

#define BTN_INITIAL_DELAY  10  //10ms before a click registers
#define BTN_HOLD_DELAY     400 //400mas before a click becomes a hold
#define BTN_AUTOREPEAT     100 //auto-repeat every 100ms



#define SETTINGS_NAME    "/AdkSettings.bin"
#define SETTINGS_MAGIX   0xAF
typedef struct AdkSettings{
  
  uint8_t magix;
  uint8_t ver;

//v1 settings:

  uint8_t R, G, B, bri, vol, almH, almM, almOn;
  char btName[249]; //null terminated
  char btPIN[17];  //null terminated

  uint16_t almSnooze;
  char almTune[256]; // null terminated
  
  uint8_t speed, displayMode;
  
//later settings
  
}AdkSettings;



AdkSettings settings;
const char* btPIN = 0;


volatile static int32_t hTemp, hHum, bPress, bTemp;
volatile static uint16_t prox[7]; //prox,  clear, IR ,R, G, B, temp pProx, pClear, pR, pG, pB, pIR, pTemp;
volatile static uint16_t proxNormalized[3];
volatile static int16_t accel[3], mag[3];
volatile static uint32_t btSSP = ADK_BT_SSP_DONE_VAL;
volatile static char locked = 0;


void btStart();

static uint16_t btnProcess(){
 
    static uint64_t lastActionTime[32] = {0, }; 
    static uint32_t clickSent = 0;
    static uint32_t holdSent = 0;
    static uint32_t lastStates = 0;
    uint32_t curState, t, i, mask;
    
    curState = (((uint32_t)L.capSenseButtons()) << 16) | L.capSenseIcons();
    t = lastStates ^ curState;
    lastStates = curState;
    
    //update states for all buttons
    for(mask = 1, i = 0; i < 32; i++, mask <<= 1) if(t & mask){
      
      lastActionTime[i] = L.getUptime();
    }

    //generate events
    for(mask = 1, i = 0; i < 32; i++, mask <<= 1){
      if(curState & mask){
        
        uint64_t time = L.getUptime();
        uint64_t lapsed = time - lastActionTime[i];
        
        if(holdSent & mask){  //maybe resend hold
        
          if(lapsed > BTN_AUTOREPEAT){
           
             lastActionTime[i] = time;
             return i | BTN_MASK_HOLD;
          }
        }
        else if(clickSent & mask){  //maybe time for first hold
        
          if(lapsed > BTN_HOLD_DELAY){
           
             holdSent |= mask;
             lastActionTime[i] = time;
             return i | BTN_MASK_HOLD;
          }
        }
        else{  //maybe time to click
        
          if(lapsed > BTN_INITIAL_DELAY){
           
             clickSent |= mask;
             lastActionTime[i] = time;
             return i | BTN_MASK_PRESS;
          }
        }
      }
      else if(clickSent & mask){  //release
      
        clickSent &=~ mask;
        holdSent &=~ mask;
        return i | BTN_MASK_RELEASE;
      }
    }
    return 0;
}

void readSettings(){
 
   uint32_t read;
   FatFileP f;
   char r;
   AdkSettings ts;
   
   //apply defaults
   strcpy(settings.btName, "ADK 2012");
   strcpy(settings.btPIN, "1337");
   settings.magix = SETTINGS_MAGIX;
   settings.ver = 1;
   settings.R = 0;
   settings.G = 0;
   settings.B = 255;
   settings.bri = 255;
   settings.vol = 255;
   settings.almH = 6;
   settings.almM = 0;
   settings.almOn = 0;
   settings.speed = 1;
   settings.displayMode = AdkShowAnimation;
   settings.almSnooze = 10 * 60;	//10-minute alarm
   strcpy(settings.almTune, "/Tunes/Alarm_Rooster_02.ogg");
   
   r = L.fatfsOpen(&f, SETTINGS_NAME, FATFS_READ);
   if(!r){
    
      r = L.fatfsRead(f, &ts, sizeof(AdkSettings), &read);
      
      if(r || read != sizeof(AdkSettings) || settings.magix != SETTINGS_MAGIX || settings.ver != 1){
      
       //in future, check for other versions and read as needed here...
       dbgPrintf("ADK: settings: file read failed: %d, %u/%u, 0x%x\n", r, read, sizeof(AdkSettings), settings.magix);
     }
     else settings = ts;
     L.fatfsClose(f);
   }
   else dbgPrintf("ADK: settings: file open failed: %d\n", r);
}

void writeSettings(){
 
   FatFileP f;
   char r;
   
   L.fatfsUnlink(SETTINGS_NAME); 
   r = L.fatfsOpen(&f, SETTINGS_NAME, FATFS_WRITE | FATFS_CREATE | FATFS_TRUNCATE);
   if(!r){
     uint32_t written = 0;
     
     r = L.fatfsWrite(f, &settings, sizeof(AdkSettings), &written);
 
     if(r || written != sizeof(AdkSettings)) dbgPrintf("ADK: settings: file write failed: %d, %u/%u\n", r, written, sizeof(AdkSettings));
     L.fatfsClose(f);
   }
   else dbgPrintf("ADK: settings: file open failed: %d\n", r);
}

void setup(void)
{

 Serial.begin(115200);
 
 L.adkSetPutchar(adkPutchar);
 L.adkInit();
 btStart();
 
 if(L.fatfsMount()) dbgPrintf("ADK: failed to mount SD card\n");
 L.usbStart();
}

static uint8_t rnd(void){
 
  static uint64_t seed = 7454131806685196871ULL;
  
  seed *= 2340027325706224672ULL;
  seed += 7310016643071172983ULL;
  seed %= 9876543210987654321ULL;
  seed ^= seed << 3;
  seed += L.getUptime();
  
  return seed >> 53;
}

void gethue(uint8_t h, uint8_t* Rp, uint8_t* Gp, uint8_t* Bp){
  uint8_t R, G, B, seg, prog;
  
  if(h == 255) h = 254;
  seg = (unsigned)h / 85;
  prog = (((uint32_t)((unsigned)h % 85)) << 8) / 85;
  
  switch(seg){
    case 0: R = prog;       G = 0;          B = 255 - prog; break;
    case 1: R = 255 - prog; G = prog;       B = 0;          break;
    case 2: R = 0;          G = 255 - prog; B = prog;       break;
  }
  
  *Rp = R;
  *Gp = G;
  *Bp = B;
}

static void screenClear(void){
  uint32_t i;

  for(i = 0; i < NUM_LEDS; i++) L.ledWrite(i, 0, 0, 0);  //clear screen if leaving display mode
}

void adkBtSspF(const uint8_t* mac, uint32_t val){

  btSSP = val;
  dbgPrintf("ssp with val %u\n", val);
}

#define PI_SCALE	400	//200 units = pi

static long fpmul(long a, long b){

    int64_t aa = a, bb = b;

    aa *= bb;
    aa >>= 22;

    return aa;
}

static int isin_ser(int i){	//0-pi/2 only

    //x - x^3 /6 + x^5 / 120 - x^7 / 5040
    long x;
    long x_2, x_3, x_5, x_7;


    //convert to 10.22 fixed point and 0..pi/2 range
    x = (13176794LL /* pi */ * (int64_t)i + PI_SCALE ) / (PI_SCALE * 2);

    //calculate x^2
    x_2 = fpmul(x, x);


    //calculate x^3, x^5, x^7
    x_3 = fpmul(x, x_2);
    x_5 = fpmul(x_3, x_2);
    x_7 = fpmul(x_5, x_2);


    //calculate sin
    x = x - (x_3 + 3) / 6 + (x_5 + 60) / 120 - (x_7 + 2520) / 5040;


    //desired result is 10-bit fp, so scale it back
    x >>= 12;


    return x;
}



static int isin(int i){

    char neg = 0;

    if(i < 0){

        i = -i;
        neg ^= 1;
    }
    i %= PI_SCALE * 2;
    if(i >= PI_SCALE){

        i -= PI_SCALE;
        neg ^= 1;
    }
    if(i > PI_SCALE / 2){

        i = PI_SCALE - i;
    }

    i = isin_ser(i);
    if(neg) i = -i;

    return i;
}

static int icos(int v){

    return isin(v + PI_SCALE / 2);
}

void loop(void)
{
  uint8_t r, g, b, dimR, dimG, dimB;
  uint8_t colorSlider, volSlider, briSlider, speedSlider;
  static const uint8_t dimIconFactor = 6;
  AdkStates state = AdkInvalid;
  AdkStates newState = AdkClock;
  AdkAlarmStates alarmState = AdkAlarmIdle;
  uint32_t i, j, k;
  uint8_t loopCounter = 0;
  uint64_t nextTime = 0; //for animation
  unsigned int snoozeStart;
  char alarmEnded = 1;
  char alarmStop, alarmFake = 0;
  int32_t alarmDirPos = -1;
  FatDirP dir = 0;
  char wasBt = 0;
  static const uint8_t btSeq[] = {6,5,8,12,11};

  readSettings();
  
  if(0){ //very helpful to some of us - set alarm song to the first song we find :)
    FatDirP dir;
    if(!L.fatfsOpenDir(&dir, "/Tunes")){

      FatFileInfo fi;
      fi.longName = settings.almTune + 7;
      fi.nameSz = 100;
          
      strcpy(settings.almTune, "/Tunes/");

      if(!L.fatfsReadDir(dir, &fi)){
            
        dbgPrintf("tune: '%s'\n", settings.almTune);
      } else dbgPrintf("file find fail\n");
    } else dbgPrintf("dir open fail\n");   
  }


  btPIN = settings.btPIN;
  L.setVolume(settings.vol);
  dbgPrintf("ADK: setting BT name '%s' and pin '%s'\n", settings.btName, settings.btPIN);
  if(!L.btSetDeviceClass(DEVICE_CLASS_SERVICE_AUDIO | DEVICE_CLASS_SERVICE_RENDERING |
		DEVICE_CLASS_SERVICE_INFORMATION | (DEVICE_CLASS_MAJOR_AV << DEVICE_CLASS_MAJOR_SHIFT) |
		(DEVICE_CLASS_MINOR_AV_PORTBL_AUDIO << DEVICE_CLASS_MINOR_AV_SHIFT))) dbgPrintf("ADK: Failed to set device class\n");
  if(!L.btSetLocalName(settings.btName)) dbgPrintf("ADK: failed to set BT name\n");
  if(!L.btDiscoverable(1)) dbgPrintf("ADK: Failed to set discoverable\n");
  if(!L.btConnectable(1)) dbgPrintf("ADK: Failed to set connectable\n");
  L.btSetSspCallback(adkBtSspF);

  while(1){
  
   loopCounter++;
   
   uint16_t button = btnProcess();

   if(locked == 2){
     if(button == (BTN_MASK_HOLD | BTN_LOCK)) locked = 0;
     else button = 0;
   }
   else if(button == (BTN_MASK_PRESS | BTN_LOCK) && locked == 0) locked = 1;
   else if(button == (BTN_MASK_RELEASE | BTN_LOCK) && locked == 1) locked = 2;

   L.adkEventProcess(); //let the adk framework do its thing
   
   r = (((uint32_t)settings.R) * (settings.bri + 1)) >> 8;
   g = (((uint32_t)settings.G) * (settings.bri + 1)) >> 8;
   b = (((uint32_t)settings.B) * (settings.bri + 1)) >> 8;

   dimR = (r + dimIconFactor - 1) / dimIconFactor;
   dimG = (g + dimIconFactor - 1) / dimIconFactor;
   dimB = (b + dimIconFactor - 1) / dimIconFactor;

   if(newState != AdkInvalid){
     state = newState;
     newState = AdkInvalid;
     for(i = 0; i < 8; i++) L.ledDrawIcon(i, dimR, dimG, dimB);
     L.ledDrawIcon(state, r, g, b);
   }
   L.ledDrawIcon(7, locked ? r : dimR, locked ? g : dimG, locked ? b : dimB);

   if(!(loopCounter & 127)) if(!L.hygroRead((int32_t *)&hTemp, (int32_t *)&hHum)) hTemp = hHum = 0;
   if(!(loopCounter & 7)){
     uint16_t proxMax;

     L.baroRead(3, (long *)&bPress, (long *)&bTemp);
     L.alsRead((uint16_t *)prox + 0, (uint16_t *)prox + 1, (uint16_t *)prox + 3, (uint16_t *)prox + 4, (uint16_t *)prox + 5, (uint16_t *)prox + 2, (uint16_t *)prox + 6);
     L.accelRead((int16_t *)accel + 0, (int16_t *)accel + 1, (int16_t *)accel + 2);
     L.magRead((int16_t *)mag + 0, (int16_t *)mag + 1, (int16_t *)mag + 2);
     for(i = 0; i < 3; i++) mag[i] <<= 4;	//convert to 16-bit value

     //copy
     for(i = 0; i < 3; i++) proxNormalized[i] = prox[i + 3];
     proxNormalized[2] *= 3; //blue needs more sensitivity

     //find max
     proxMax = 0;
     for(i = 0; i < 3; i++) if(proxMax < proxNormalized[i]) proxMax = proxNormalized[i];
     proxMax++;

     //normalize to 8-bits
     for(i = 0; i < 3; i++) proxNormalized[i] = (proxNormalized[i] << 8) / proxMax;

     //exponentize (as per human eyes)
     static const uint16_t exp[] =
     {
       0,19,39,59,79,100,121,143,165,187,209,232,255,279,303,327,352,377,402,428,454,481,508,536,564,592,621,650,680,710,741,772,804, 836,869,902,
       936,970,1005,1040,1076,1113,1150,1187,1226,1264,1304,1344,1385,1426,1468,1511,1554,1598,1643,1688,1734,1781,1829,1877,1926,1976,2026,2078,
       2130,2183,2237,2292,2348,2404,2461,2520,2579,2639,2700,2762,2825,2889,2954,3020,3088,3156,3225,3295,3367,3439,3513,3588,3664,3741,3819,3899,
       3980,4062,4146,4231,4317,4404,4493,4583,4675,4768,4863,4959,5057,5156,5257,5359,5463,5568,5676,5785,5895,6008,6122,6238,6355,6475,6597,
       6720,6845,6973,7102,7233,7367,7502,7640,7780,7922,8066,8213,8362,8513,8666,8822,8981,9142,9305,9471,9640,9811,9986,10162,10342,10524,10710,
       10898,11089,11283,11480,11681,11884,12091,12301,12514,12731,12951,13174,13401,13632,13866,14104,14345,14591,14840,15093,15351,15612,15877,16147,
       16421,16699,16981,17268,17560,17856,18156,18462,18772,19087,19407,19733,20063,20398,20739,21085,21437,21794,22157,22525,22899,23279,23666,
       24058,24456,24861,25272,25689,26113,26544,26982,27426,27878,28336,28802,29275,29756,30244,30740,31243,31755,32274,32802,33338,33883,34436,
       34998,35568,36148,36737,37335,37942,38559,39186,39823,40469,41126,41793,42471,43159,43859,44569,45290,46023,46767,47523,48291,49071,49863,
       50668,51486,52316,53159,54016,54886,55770,56668,57580,58506,59447,60403,61373,62359,63361,64378,65412
     };

     for(i = 0; i < 3; i++) proxNormalized[i] = exp[proxNormalized[i]];
   }

   if(wasBt && btSSP == ADK_BT_SSP_DONE_VAL){ //clear leftover segments

     L.ledWrite(btSeq[wasBt - 1], 0, 0, 0);
     wasBt = 0;
   }
   else if(btSSP != ADK_BT_SSP_DONE_VAL){

     k = btSSP;
     for(i = 0, j = 100000; i < 6; i++, j /= 10){

       L.ledDrawLetter(i, k / j + '0', 0, 0, settings.bri);
       k %= j;
     }

     L.ledWrite(9, 0, 0, 0);
     L.ledWrite(2, 0, 0, 0);

     if(wasBt) L.ledWrite(btSeq[wasBt - 1], 0, 0, 0);
     k = (L.getUptime() >> 7) % sizeof(btSeq);
     wasBt = k + 1;
     L.ledWrite(btSeq[k], 0, 0, settings.bri);

     button = 0;
   }
   else switch(state){
    
    case AdkClock:{
     
      uint16_t year;
      uint8_t month, day, h, m, s, timechange = 1;
      
      //get and draw time
      L.rtcGet(&year, &month, &day, &h, &m, &s);
      L.ledDrawLetter(0, h / 10 + '0', r, g, b);
      L.ledDrawLetter(1, h % 10 + '0', r, g, b);
      L.ledDrawLetter(2, m / 10 + '0', r, g, b);
      L.ledDrawLetter(3, m % 10 + '0', r, g, b);
      L.ledDrawLetter(4, s / 10 + '0', r, g, b);
      L.ledDrawLetter(5, s % 10 + '0', r, g, b);
      L.ledWrite(9, r, g, b);
      L.ledWrite(2, r, g, b);
      
      //handle buttons
      if(button & (BTN_MASK_PRESS | BTN_MASK_HOLD)) switch(button & BTN_MASK_ID){
        
          case BTN_5_DN:
            if(s) s--;
            break;
          case BTN_5_UP:
            if(s < 59) s++;
            break;
          case BTN_4_UP:
            if(s < 50) s += 10;
            break;
          case BTN_4_DN:
            if(s > 9) s -= 10;
            break;
          case BTN_3_UP:
            if(m < 59) m++;
            break;
          case BTN_3_DN:
            if(m) m--;
            break;
          case BTN_2_UP:
            if(m < 50) m += 10;
            break;
          case BTN_2_DN:
            if(m > 9) m -= 10;
            break;
          case BTN_1_UP:
            if(h < 23) h++;
            break;
          case BTN_1_DN:
            if(h) h--;
            break;
          case BTN_0_UP:
            if(h < 14) h += 10;
            break;
          case BTN_0_DN :
            if(h > 9) h -=10;
            break;
          default: timechange = 0;
        } else timechange = 0;
        
        if(timechange){
          L.rtcSet(year, month, day, h, m, s);
        }
        break;
      }
      case AdkAlarm:{
      
        if(!alarmFake || alarmEnded){
          L.ledDrawLetter(0, settings.almH / 10 + '0', r, g, b);
          L.ledDrawLetter(1, settings.almH % 10 + '0', r, g, b);
          L.ledDrawLetter(2, settings.almM / 10 + '0', r, g, b);
          L.ledDrawLetter(3, settings.almM % 10 + '0', r, g, b);
          L.ledDrawLetter(4, 'O', r, g, b);
          L.ledDrawLetter(5, (settings.almOn ? 'N' : 'F'), r, g, b);
          L.ledWrite(9, r, g, b);
          L.ledWrite(2, 0, 0, 0);

          if(button & (BTN_MASK_PRESS | BTN_MASK_HOLD)) switch(button & BTN_MASK_ID){
        
            case BTN_5_DN:
            case BTN_5_UP:
            case BTN_4_UP:
            case BTN_4_DN:
              settings.almOn ^= 1;
              break;
            case BTN_3_UP:
              if(settings.almM < 59) settings.almM++;
              break;
            case BTN_3_DN:
              if(settings.almM) settings.almM--;
              break;
            case BTN_2_UP:
              if(settings.almM < 50) settings.almM += 10;
              break;
            case BTN_2_DN:
              if(settings.almM > 9) settings.almM -= 10;
              break;
            case BTN_1_UP:
              if(settings.almH < 23) settings.almH++;
              break;
            case BTN_1_DN:
              if(settings.almH) settings.almH--;
              break;
            case BTN_0_UP:
              if(settings.almH < 14) settings.almH += 10;
              break;
            case BTN_0_DN :
              if(settings.almH > 9) settings.almH -=10;
              break;
          }
        }
        else{
          L.ledWrite(9, 0, 0, 0);
          L.ledWrite(2, 0, 0, 0);
        }
        if(button == (BTN_MASK_HOLD | BTN_ALARM) && alarmEnded && (alarmState == AdkAlarmIdle || alarmState == AdkAlarmIdleWait)){ //handle sound file selection

          char* name = settings.almTune;
          alarmFake = 1;
          alarmEnded = 0;
          alarmStop = 0;
          dbgPrintf("Playing song '%s'\n", settings.almTune);
          if(name[0] == '/' && (name[1] == 'T' || name[1] == 't') &&
             (name[2] == 'u' || name[2] == 'U') && (name[3] == 'n' || name[3] == 'N') &&
             (name[4] == 'e' || name[4] == 'E') && (name[5] == 's' || name[5] == 'S') &&
             name[6] == '/') name += 7;
          for(i = 0; i < 6; i++) L.ledDrawLetter(i, name[i], r, g, b);
          L.playOggBackground(settings.almTune, &alarmEnded, &alarmStop);
        }
        if(button == (BTN_MASK_PRESS | BTN_SLIDER_0) || button == (BTN_MASK_PRESS | BTN_SLIDER_1) || button == (BTN_MASK_PRESS | BTN_SLIDER_2)){

          char name[256] = "/Tunes/", found = 0;

          dbgPrintf("next file\n");

          for(j = 0; j < 2 && !found; j++) if(!L.fatfsOpenDir(&dir, "/Tunes")){

            alarmDirPos++;
            for(i = 0; i <= alarmDirPos; i++){

              FatFileInfo fi;
              fi.longName = name + 7;
              fi.nameSz =  sizeof(name) - 7;

              if(!L.fatfsReadDir(dir, &fi)){

                if(i == alarmDirPos){
                  dbgPrintf("file: '%s'\n", name);
                  found = 1;
                }
              }
              else{

                alarmDirPos = -1;
                dbgPrintf("no more files\n");
                break;
              }
            }
            L.fatfsCloseDir(dir);
          }
          if(found) strcpy(settings.almTune, name);
        }
        break;
      }
      case AdkVolume:{
       
        uint8_t dispVol = ((uint32_t)L.getVolume() * 100) >> 8;
        
        L.ledDrawLetter(0, 'V', r, g, b);
        L.ledDrawLetter(1, 'O', r, g, b);
        L.ledDrawLetter(2, 'L', r, g, b);
        L.ledDrawLetter(3, ' ', r, g, b);
        L.ledDrawLetter(4, dispVol / 10 + '0', r, g, b);
        L.ledDrawLetter(5, dispVol % 10 + '0', r, g, b);
        
        dispVol = L.capSenseSlider();
        if(dispVol != volSlider){
          L.setVolume(settings.vol = dispVol);
          volSlider = dispVol;
        }
        
        break;
      }
      case AdkColor:{
        
        uint8_t slider, seg, prog;
        static const char hexch[] = "0123456789ABCDEF";
        
        L.ledDrawLetter(0, hexch[settings.R >> 4], r, g, b);
        L.ledDrawLetter(1, hexch[settings.R & 15], r, g, b);
        L.ledDrawLetter(2, hexch[settings.G >> 4], r, g, b);
        L.ledDrawLetter(3, hexch[settings.G & 15], r, g, b);
        L.ledDrawLetter(4, hexch[settings.B >> 4], r, g, b);
        L.ledDrawLetter(5, hexch[settings.B & 15], r, g, b);
        L.ledWrite(9, 0, 0, 0);
        L.ledWrite(2, 0, 0, 0);
        
        if(button & (BTN_MASK_PRESS | BTN_MASK_HOLD)) switch(button & BTN_MASK_ID){
          
          case BTN_5_DN:
            if(settings.B) settings.B--;
            break;
          case BTN_5_UP:
            if(settings.B < 0xFF) settings.B++;
            break;
          case BTN_4_UP:
            if(settings.B < 0xF0) settings.B += 0x10;
            break;
          case BTN_4_DN:
            if(settings.B > 0x0F) settings.B -= 0x10;
            break;
          case BTN_3_UP:
            if(settings.G < 0xFF) settings.G++;
            break;
          case BTN_3_DN:
            if(settings.G) settings.G--;
            break;
          case BTN_2_UP:
            if(settings.G < 0xF0) settings.G += 0x10;
            break;
          case BTN_2_DN:
            if(settings.G > 0x0F) settings.G -= 0x10;
            break;
          case BTN_1_UP:
            if(settings.R < 0xFF) settings.R++;
            break;
          case BTN_1_DN:
            if(settings.R) settings.R--;
            break;
          case BTN_0_UP:
            if(settings.R < 0xF0) settings.R += 0x10;
            break;
          case BTN_0_DN :
            if(settings.R > 0x0F) settings.R -= 0x10;
            break;
        }
          
        slider = L.capSenseSlider();
        if(slider != colorSlider){
          colorSlider = slider;

          gethue(slider, &settings.R, &settings.G, &settings.B);
        }
        break;
      }
      case AdkBrightness:{
        
        uint8_t slider;
        uint8_t dispBri = ((uint32_t)settings.bri * 100) >> 8;
        
        L.ledDrawLetter(0, 'B', r, g, b);
        L.ledDrawLetter(1, 'R', r, g, b);
        L.ledDrawLetter(2, 'I', r, g, b);
        L.ledDrawLetter(3, ' ', r, g, b);
        L.ledDrawLetter(4, dispBri / 10 + '0', r, g, b);
        L.ledDrawLetter(5, dispBri % 10 + '0', r, g, b);
        L.ledWrite(9, 0, 0, 0);
        L.ledWrite(2, 0, 0, 0);
       
        slider = L.capSenseSlider();
        slider = ((((uint32_t)slider) * 191) >> 8) + 64;
        if(slider != briSlider){
          briSlider = slider;
          settings.bri = slider;
        }
        break;
      }
      case AdkDisplay:{
       
        static uint8_t vals[NUM_LEDS][3] = {{0,},};
        static const uint8_t doNotTouch[] = {49, 33, 17, 1, 48, 32, 16, 0};
        uint8_t slider = L.capSenseSlider();
        
        if(slider != speedSlider){
          speedSlider = slider;
          settings.speed = (255 - slider) >> 3;
        }
        
        if(L.getUptime() > nextTime){
          
          if(!nextTime){

            const char* name = NULL;

            switch(settings.displayMode){

              default: settings.displayMode = AdkShowAnimation; //fallthrough
              case AdkShowAnimation: name = "PRETTY"; break;
              case AdkShowAccel:     name = " ACCEL"; break;
              case AdkShowMag:       name = "MAGNET"; break;
              case AdkShowTemp:	     name = " TEMP "; break;
              case AdkShowHygro:     name = " HYGRO"; break;
              case AdkShowBaro:      name = " BARO "; break;
              case AdkShowProx:      name = " PROX "; break;
              case AdkShowColor:     name = " COLOR"; break;
            }
            for(i = 0; i < 6; i++) L.ledDrawLetter(i, name[i], r, g, b);
            L.ledWrite(9, 0, 0, 0);
            L.ledWrite(2, 0, 0, 0);
            nextTime = L.getUptime() + 1000;
          }
          else{
            nextTime = L.getUptime() + settings.speed;
            volatile int16_t* arrPtr = NULL;
            static const long valUnused = 0x7FFFFFFF;
            int val = valUnused;
            char isSigned = 1, cR = r, cG = g, cB = b;

            switch(settings.displayMode){

              case AdkShowAnimation:{

                /*if(rnd() < 30){  //spawn
          
                    i = rnd() % NUM_LEDS;
                    gethue(rnd(), vals[i] + 0, vals[i] + 1, vals[i] + 2);
                }
                for(i = 0; i < NUM_LEDS; i++){
                  for(j = 0; j < 3; j++) vals[i][j] = ((uint32_t)vals[i][j] * 127) >> 7;
                  for(j = 0; j < sizeof(doNotTouch) && i != doNotTouch[j]; j++);
                  if(j == sizeof(doNotTouch)) ADK_ledWrite(i, vals[i][0], vals[i][1], vals[i][2]);
                }*/
		static const uint16_t coord_x[] =	{
								0,0,406,434,318,460,549,346,259,204,176, 28, 57,116,230,
								57,0,0,549,523,523,549,582,582,549,259,232,232,259,293,
								293,259,0,0,466,435,435,466,494,494,466,149,114,116,149,
								178,178,149,0,0,349,318,318,349,378,378,349, 59, 28, 28,
								57, 89, 89, 59
							};
		static const uint16_t coord_y[] =	{
								0,0, 92,221,219,194,194,253,194, 92,287,282,194,222,283,
								313,0,0, 87, 60,117,149,117, 60, 30, 87, 60,117,149,117,
								60, 30,0,0, 87, 60,117,149,117, 60,30, 87, 60,117,149,117,
								60, 30,0,0, 87, 60,117,149,117, 60, 30, 87, 60,117,149,117,
								60, 30
							};

		static long posX = 0, posY = 0;
		static int speedX = 16, speedY = 8;
		static int accelX = 0, accelY = 0;
		static signed char sizeX = 0, sizeY = 0;

		if(!rnd()) accelX += (rnd() & 2) - 1;
                if(!rnd()) accelY += (rnd() & 2) - 1;
                
		if(!(rnd() & 0x1F) && sizeX <  0x7F) sizeX++;
                if(!(rnd() & 0x1F) && sizeX > -0x7F) sizeX--;
		if(!(rnd() & 0x1F) && sizeY <  0x7F) sizeY++;
                if(!(rnd() & 0x1F) && sizeY > -0x7F) sizeY--;
                
		if(!(rnd() & 0x3F)) speedX += accelX;
		if(speedX > 32 || speedX < -32){

			accelX = -accelX;
			speedX += 2 * accelX;
		}
		if(!(rnd() & 0x3F)) speedY += accelY;
		if(speedY > 32 || speedY < -32){

			accelY = -accelY;
			speedY += 2 * accelY;
		}

		posX += speedX;
		posY += speedY;

                for(i = 0; i < sizeof(coord_x) / sizeof(*coord_x); i++) if(coord_x[i]){

			uint8_t cr, cg, cb, val;

			val = 128 + ((isin((long)coord_y[i] * (8 + sizeX / 42) / 8 + posX / 16) * icos((long)coord_x[i] * (8 + sizeY / 42) / 8 - posY / 16)) >> 12);	//now in 8-bit unsigned range

			gethue(val, &cr, &cg, &cb);

			//dim the neutral areas a little bit
			if(val < 128 + 32 && val > 128 - 32){

				val -= 128;
				if(val & 0x80) val = -val;
				//now val is 0..32  where 0 is closest to 128
				val >>= 3;
				//now val is 0..3   where 0 is closest to 128
				val = 5 - val;
				//now val is 2..5   where 5 is closest to 128
				cr = ((long)cr * 2L + val / 2) / val;
				cg = ((long)cg * 2L + val / 2) / val;
				cb = ((long)cb * 2L + val / 2) / val;
			}

			L.ledWrite(i, cr, cg, cb);
		}
                }break;

              case AdkShowAccel:

                arrPtr = accel;
                //fallthrough

              case AdkShowMag:

                if(!arrPtr) arrPtr = mag;
                //fallthrough

              case AdkShowProx:

                if(!arrPtr){

                  arrPtr = (int16_t*)prox;
                  isSigned = 0;
                }
                //fallthrough

              case AdkShowColor:

                if(!arrPtr){

                  arrPtr = (int16_t*)proxNormalized;
                  isSigned = 0;
                  cR = proxNormalized[0] >> 8;
                  cG = proxNormalized[1] >> 8;
                  cB = proxNormalized[2] >> 8;
                }

                for(i = 0; i < 6; i += 2){

                  val = arrPtr[i >> 1];
                  if(isSigned){
                    if(val < 0){
                      val = -val;
                      j = 0;
                    }
                    else j = 1;
                  }
                  else val = (uint16_t)val;

                  val *= 100;
                  val >>= isSigned ? 15 : 16;
                  L.ledDrawLetter(i + 0, + val / 10 + '0', isSigned ? (j ? settings.bri : 0) : cR, isSigned ? 0 : cG, isSigned ? (j ? 0 : settings.bri) : cB);
                  L.ledDrawLetter(i + 1, + val % 10 + '0', isSigned ? (j ? settings.bri : 0) : cR, isSigned ? 0 : cG, isSigned ? (j ? 0 : settings.bri) : cB);
                }
                break;

              case AdkShowTemp:

                val = bTemp;
                //fall through

              case AdkShowHygro:

                if(val == valUnused) val = hHum;
                //fall through

              case AdkShowBaro:

                if(val == valUnused) val = (bPress + 50) / 100;
                if(val < 0){
                  L.ledDrawLetter(0, '-', r, g, b);
                  val = -val;
                }
		for(j = 1000, i = 1, k = 0; i < 6; i++){

                  if(i == 4) L.ledDrawLetter(i, '.', r, g, b);
                  else{

                    if(val >= j || i >= 4 || k){
                      k = 1;
                      L.ledDrawLetter(i, val / j + '0', r, g, b);
                    }
                    else L.ledDrawLetter(i, ' ', r, g, b);
                    val %= j;
                    j /= 10;
                  }
                }
                break;
            }
          }
        }
        break;
      }
      case AdkPresets:{

        L.ledWrite(9, 0, 0, 0);
        L.ledWrite(2, 0, 0, 0);

        L.ledDrawLetter(0, ' ', r, g, b);
        L.ledDrawLetter(1, 'n', r, g, b);
        L.ledDrawLetter(2, 'o', r, g, b);
        L.ledDrawLetter(3, 'n', r, g, b);
        L.ledDrawLetter(4, 'e', r, g, b);
        L.ledDrawLetter(5, ' ', r, g, b);

        break;
      }
    }

    if(button == (BTN_MASK_RELEASE | BTN_ALARM) && alarmFake && !alarmEnded){

      dbgPrintf("Stopping song\n");
      alarmStop = 1;
      alarmFake = 0;
    }

    if(button & BTN_MASK_PRESS) switch(button & BTN_MASK_ID){
      
       case BTN_CLOCK:      newState = AdkClock;      break;
       case BTN_ALARM: 
         // eat the transition to alarm edit mode if we're in an alarm
         if (alarmState == AdkAlarmIdle || alarmState == AdkAlarmIdleWait)
           newState = AdkAlarm;
         break;
       case BTN_BRIGHTNESS: newState = AdkBrightness; briSlider = L.capSenseSlider(); break;
       case BTN_COLOR:      newState = AdkColor;      colorSlider = L.capSenseSlider(); break;
       case BTN_PRESETS:    newState = AdkPresets;    break;
       case BTN_DISPLAY:    newState = AdkDisplay;    nextTime = 0; speedSlider = L.capSenseSlider(); break;
       case BTN_VOLUME:     newState = AdkVolume;     volSlider = L.capSenseSlider(); break;
    }
    if(newState != AdkInvalid){
      writeSettings();
      if(state == AdkDisplay){
        screenClear();

        //switch to next display mode
        if(newState == AdkDisplay && ++settings.displayMode  == AdkShowLast) settings.displayMode = 0;
      }
    }

    // alarm
    if (settings.almOn) {
      uint8_t month, day, h, m, sec;

      L.rtcGet(0, 0, 0, &h, &m, 0);

      switch (alarmState) {
        case AdkAlarmIdle: // see if we need to trigger an alarm
          if (settings.almH == h && settings.almM == m) {
            // start alarm
            Serial.write("ALARM\n");
            alarmState = AdkAlarmAlarm;
          }
          break;
        case AdkAlarmIdleWait: // we had triggered, need to wait at least a minute before waiting for next alarm
          if (settings.almH != h || settings.almM != m) {
            alarmState = AdkAlarmIdle;
          }
          break;
        case AdkAlarmAlarm: {
          // check for alarm button to cancel playing alarm
          if (((button & BTN_MASK_ID) == BTN_ALARM) &&
              (button & (BTN_MASK_PRESS | BTN_MASK_HOLD))) {
            alarmState = AdkAlarmIdleWait;
            alarmStop = 1;
            L.ledDrawIcon(1, (settings.R + dimIconFactor - 1) / dimIconFactor, (settings.G + dimIconFactor - 1) / dimIconFactor, (settings.B + dimIconFactor - 1) / dimIconFactor);
            break;
          }
          // check for snooze button
          if (((button & BTN_MASK_ID) <= 2) &&
              (button & (BTN_MASK_PRESS | BTN_MASK_HOLD))) {
            Serial.write("ALARM SNOOZE\n");
            alarmState = AdkAlarmSnooze;
            snoozeStart = millis() / 1000;
            alarmStop = 1;
            break;
          }

          // make sure ogg is playing
          if (alarmEnded) {
            alarmEnded = 0;
            alarmStop = 0;
            alarmFake = 0;
            L.playOggBackground(settings.almTune, &alarmEnded, &alarmStop);
          }

          // blink the alarm button red
          if ((millis() / 500) & 1)
            L.ledDrawIcon(1, 0xff, 0, 0);
          else
            L.ledDrawIcon(1, (settings.R + dimIconFactor - 1) / dimIconFactor, (settings.G + dimIconFactor - 1) / dimIconFactor, (settings.B + dimIconFactor - 1) / dimIconFactor);
          break;
        }
        case AdkAlarmSnooze:
          // check for alarm button to cancel playing alarm
          if (alarmState != AdkAlarmIdle && 
              ((button & BTN_MASK_ID) == 4) &&
              (button & (BTN_MASK_PRESS | BTN_MASK_HOLD))) {
            alarmState = AdkAlarmIdleWait;
            alarmStop = 1;
            L.ledDrawIcon(1, (settings.R + dimIconFactor - 1) / dimIconFactor, (settings.G + dimIconFactor - 1) / dimIconFactor, (settings.B + dimIconFactor - 1) / dimIconFactor);
            break;
          }
          // see if we need to transition back to alarm state
          if ((millis() / 1000) - snoozeStart > settings.almSnooze) {
            Serial.write("ALARM FROM SNOOZE\n");
            alarmState = AdkAlarmAlarm; 
            break;
          }

          // blink the alarm button blue
          if ((millis() / 500) & 1)
            L.ledDrawIcon(1, 0, 0, 0xff);
          else
            L.ledDrawIcon(1, (settings.R + dimIconFactor - 1) / dimIconFactor, (settings.G + dimIconFactor - 1) / dimIconFactor, (settings.B + dimIconFactor - 1) / dimIconFactor);
          break;
      }
    } else {
      // make sure there are no dangling alarms
      if(!alarmFake) alarmStop = 1; 
    }

    // usb accessory processing
    processUSBAccessory();
  }
}




//////////// bt interface (fun)

//commands
#define MAX_PACKET_SZ                       260  //256b payload + header

// command header
// u8 cmd opcode
// u8 sequence
// u16 size

// data formats:
//   timespec = (year,month,day,hour,min,sec) (u16,u8,u8,u8,u8,u8)

#define CMD_MASK_REPLY                      0x80
#define BT_CMD_GET_PROTO_VERSION            1    // () -> (u8 protocolVersion)
#define BT_CMD_GET_SENSORS                  2    // () -> (sensors: i32,i32,i32,i32,u16,u16,u16,u16,u16,u16,u16,i16,i16,i16,i16,i16,i16) 
#define BT_CMD_FILE_LIST                    3    // FIRST: (char name[]) -> (fileinfo or single zero byte)   OR   NONLATER: () -> (fileinfo or empty or single zero byte)
#define BT_CMD_FILE_DELETE                  4    // (char name[0-255)) -> (char success)
#define BT_CMD_FILE_OPEN                    5    // (char name[0-255]) -> (char success)
#define BT_CMD_FILE_WRITE                   6    // (u8 data[]) -> (char success)
#define BT_CMD_FILE_CLOSE                   7    // () -> (char success)
#define BT_CMD_GET_UNIQ_ID                  8    // () -> (u8 uniq[16])
#define BT_CMD_BT_NAME                      9    // (char name[]) -> () OR () -> (char name[])
#define BT_CMD_BT_PIN                      10    // (char PIN[]) -> () OR () -> (char PIN[])
#define BT_CMD_TIME                        11    // (timespec) -> (char success)) OR () > (timespec)
#define BT_CMD_SETTINGS                    12    // () -> (alarm:u8,u8,u8,brightness:u8,color:u8,u8,u8:volume:u8) or (alarm:u8,u8,u8,brightness:u8,color:u8,u8,u8:volume:u8) > (char success)
#define BT_CMD_ALARM_FILE                  13    // () -> (char file[0-255]) OR (char file[0-255]) > (char success)
#define BT_CMD_GET_LICENSE                 14    // () -> (u8 licensechunk[]) OR () if last sent
#define BT_CMD_DISPLAY_MODE                15    // () -> (u8) OR (u8) -> ()
#define BT_CMD_LOCK                        16    // () -> (u8) OR (u8) -> ()

#define BT_PROTO_VERSION_1                  1    //this line marks the end of v1.0 API, all things after this are the next version


//constants
#define BT_PROTO_VERSION_CURRENT            BT_PROTO_VERSION_1

static const uint8_t gzippedLicences[] = {
	0x1F, 0x8B, 0x08, 0x00, 0x36, 0xB6, 0xDF, 0x4F, 0x02, 0x03, 0xCD, 0x58, 0x5D, 0x73, 0xDA, 0x38, 0x14, 0x7D, 0xD7, 0xAF, 0xB8, 0x93, 0x97, 0x26, 0x19, 0x07, 0xB2, 0x79, 0xDA, 0x49, 0x9F, 0x0C,
	0x18, 0xD0, 0x2C, 0xB1, 0x59, 0xDB, 0x24, 0xE5, 0xAD, 0xC2, 0x16, 0xE0, 0x1D, 0x63, 0x79, 0x2D, 0x93, 0x34, 0xFF, 0x7E, 0xEF, 0x95, 0x6D, 0x3E, 0x12, 0x92, 0xA6, 0x09, 0xDD, 0x96, 0xE9, 0x34,
	0x06, 0x49, 0x57, 0xE7, 0x9E, 0xFB, 0x71, 0x2C, 0x31, 0x38, 0xC6, 0x87, 0xFD, 0x02, 0x2B, 0x2C, 0x5C, 0x26, 0x1A, 0xF2, 0x42, 0xC5, 0xEB, 0xA8, 0x84, 0x24, 0x8B, 0xD2, 0x75, 0x2C, 0x35, 0x68,
	0x35, 0x2F, 0x1F, 0x44, 0x21, 0x61, 0x5E, 0xA8, 0x15, 0x74, 0x97, 0xC2, 0xBD, 0x66, 0xE7, 0x17, 0xC7, 0xFB, 0xB4, 0x59, 0x1B, 0xA0, 0x2F, 0xCA, 0xBE, 0x86, 0x0B, 0xE8, 0xDB, 0x21, 0xCC, 0x93,
	0x54, 0x82, 0x7E, 0xD4, 0xA5, 0x5C, 0xC1, 0x0A, 0xC1, 0xE0, 0xB7, 0x1A, 0x4C, 0x35, 0x04, 0xFE, 0x65, 0xEB, 0xF2, 0xCF, 0x19, 0x41, 0x3E, 0xED, 0x9E, 0x11, 0x20, 0x0B, 0xAE, 0x2E, 0xFF, 0xF8,
	0x83, 0xB5, 0x2F, 0x8E, 0x0C, 0xAB, 0x42, 0xD5, 0x60, 0xD0, 0x20, 0x60, 0x21, 0x33, 0x59, 0x24, 0xD1, 0x4B, 0x38, 0xE7, 0xAA, 0x00, 0xBD, 0x12, 0x69, 0x0A, 0x72, 0x35, 0x93, 0x71, 0x2C, 0xE3,
	0x7A, 0x82, 0x6E, 0xA1, 0x39, 0xC3, 0xAF, 0x31, 0x33, 0x2F, 0xA4, 0xDC, 0x12, 0x5B, 0x2E, 0x45, 0x09, 0x2A, 0x47, 0xD3, 0xB1, 0xB1, 0x20, 0x31, 0x00, 0xA2, 0x4C, 0x54, 0x66, 0x41, 0x21, 0xB5,
	0x14, 0x45, 0xB4, 0x04, 0x91, 0xC5, 0x10, 0xA9, 0xD5, 0x4A, 0x16, 0x51, 0x22, 0x52, 0x34, 0x16, 0xCB, 0x7B, 0x99, 0xAA, 0x7C, 0x25, 0xB3, 0x52, 0xC3, 0x3A, 0x8B, 0x65, 0x01, 0x69, 0x12, 0xC9,
	0x4C, 0x4B, 0xC8, 0x15, 0x3E, 0x3D, 0x82, 0x9A, 0xA3, 0xB5, 0x34, 0x55, 0x0F, 0x49, 0xB6, 0x80, 0xB2, 0xA8, 0x40, 0x10, 0xD9, 0x5D, 0x95, 0x3F, 0x16, 0xC9, 0x62, 0x59, 0x12, 0x7F, 0x86, 0x3A,
	0x0B, 0x2A, 0x1A, 0x09, 0x79, 0x35, 0x42, 0x1B, 0x17, 0xF7, 0x32, 0xAE, 0x96, 0x9C, 0x23, 0x76, 0x79, 0x80, 0x8E, 0x7D, 0x3F, 0x08, 0x63, 0xB9, 0x94, 0x85, 0x19, 0x74, 0x3D, 0xB8, 0xB3, 0x7D,
	0xDF, 0x76, 0xC3, 0x69, 0xCB, 0x58, 0x70, 0x15, 0x19, 0x2D, 0x91, 0x3D, 0x72, 0x0D, 0xF0, 0xDF, 0x5A, 0xCB, 0x16, 0x4C, 0xD5, 0x1A, 0x22, 0x61, 0xBE, 0x58, 0x64, 0x3B, 0x99, 0x3F, 0x1A, 0x43,
	0x85, 0x8C, 0x13, 0x9A, 0x3D, 0x5B, 0x97, 0x68, 0xAF, 0x24, 0x62, 0x08, 0x3B, 0xE4, 0xB2, 0xD0, 0x2A, 0x13, 0xA9, 0x05, 0x99, 0xCA, 0x2E, 0x30, 0x5B, 0xE7, 0x38, 0x88, 0xA4, 0x6D, 0xC9, 0xD9,
	0xA4, 0xF0, 0xC4, 0xED, 0x39, 0x3E, 0x4C, 0xBD, 0x89, 0x0F, 0xBE, 0x13, 0x8C, 0x3D, 0x37, 0xE0, 0x1D, 0x3E, 0xE2, 0x0D, 0x20, 0x7F, 0xBB, 0x03, 0x02, 0xD2, 0xC4, 0x97, 0x56, 0xEB, 0x22, 0x92,
	0x68, 0x0B, 0x73, 0x6D, 0xB5, 0xD6, 0x44, 0x43, 0x29, 0x92, 0x8C, 0xBC, 0x02, 0x31, 0x53, 0xF7, 0x34, 0xD4, 0x70, 0x97, 0xA9, 0x12, 0xF9, 0x36, 0xFC, 0x1C, 0x33, 0xEF, 0xCE, 0xDB, 0x8C, 0xBD,
	0xA9, 0x12, 0x7B, 0xAB, 0xA4, 0x2C, 0x1E, 0x61, 0x50, 0x24, 0xD9, 0x4C, 0x16, 0x8B, 0x6B, 0xD6, 0x3E, 0x67, 0x3B, 0x91, 0x8D, 0x9A, 0xC8, 0x3E, 0x99, 0x07, 0xA7, 0x02, 0x4D, 0xAF, 0x67, 0x69,
	0xA2, 0x97, 0x75, 0xBE, 0xF5, 0x06, 0x5E, 0x40, 0xF1, 0x58, 0x96, 0x65, 0x7E, 0xDD, 0x6E, 0xC7, 0x0B, 0xA5, 0x67, 0xA9, 0x5A, 0xB4, 0xE8, 0x3F, 0x9D, 0xAB, 0xB2, 0x85, 0xDC, 0x9E, 0x31, 0xBB,
	0xC9, 0x0D, 0xBD, 0x93, 0x1C, 0x6C, 0x9F, 0x44, 0x13, 0x39, 0x8C, 0x24, 0x62, 0x6E, 0xB8, 0xA4, 0x5F, 0x66, 0x49, 0x26, 0x10, 0x02, 0xEE, 0xB5, 0xD2, 0x16, 0x3C, 0x24, 0xE5, 0x92, 0x22, 0x46,
	0x7F, 0xD5, 0xBA, 0x64, 0x26, 0xE8, 0x49, 0x93, 0xF1, 0xE4, 0x1F, 0xC6, 0x18, 0x41, 0x97, 0x08, 0x0F, 0x39, 0xB8, 0x4F, 0xA8, 0x8C, 0x4C, 0x8D, 0x50, 0x18, 0xB6, 0x29, 0x1D, 0xA9, 0x2C, 0x4E,
	0xCC, 0xAE, 0x48, 0xD6, 0x4A, 0x96, 0xD7, 0xC0, 0xD8, 0x91, 0xA2, 0x6A, 0xE1, 0x58, 0xA2, 0xA9, 0x7D, 0x22, 0x4D, 0x25, 0xD9, 0xD8, 0x6C, 0xA6, 0x9B, 0x34, 0xDF, 0x41, 0x82, 0x3B, 0x46, 0xA9,
	0x48, 0x30, 0xFF, 0x5A, 0x88, 0x21, 0x1C, 0xF2, 0x00, 0x02, 0xAF, 0x1F, 0x62, 0xFE, 0x3B, 0x80, 0xCF, 0x63, 0xDF, 0xBB, 0xE5, 0x3D, 0xA7, 0x07, 0x9D, 0x29, 0x84, 0x43, 0x07, 0xBA, 0xDE, 0x78,
	0xEA, 0xF3, 0xC1, 0x30, 0x84, 0xA1, 0x37, 0xC2, 0x14, 0x0D, 0xC0, 0x76, 0x7B, 0xF8, 0xAB, 0x1B, 0xFA, 0xBC, 0x33, 0x09, 0x3D, 0xFC, 0xE1, 0xC4, 0x0E, 0x70, 0xE5, 0x09, 0x0D, 0x30, 0xDB, 0x9D,
	0x82, 0xF3, 0x65, 0x8C, 0x29, 0x1C, 0x80, 0xE7, 0x03, 0xBF, 0x19, 0x8F, 0x38, 0x1A, 0xAB, 0xAB, 0x8B, 0x3B, 0x81, 0x05, 0xDC, 0xED, 0x8E, 0x26, 0x3D, 0xEE, 0x0E, 0x2C, 0x40, 0x03, 0x58, 0x7B,
	0x21, 0x8C, 0xF8, 0x0D, 0x0F, 0x71, 0x5A, 0xE8, 0x59, 0x66, 0xD3, 0x7A, 0x19, 0xDB, 0x2E, 0x03, 0xAF, 0x0F, 0x37, 0x8E, 0xDF, 0x1D, 0xE2, 0x57, 0xBB, 0x2A, 0x0C, 0x03, 0xA4, 0xCF, 0x43, 0x97,
	0xF6, 0xEA, 0xE3, 0x66, 0x36, 0x8C, 0x6D, 0x3F, 0xE4, 0xDD, 0xC9, 0xC8, 0xF6, 0x61, 0x3C, 0xF1, 0xC7, 0x5E, 0xE0, 0x00, 0xBA, 0xC5, 0x7A, 0x3C, 0xE8, 0x8E, 0x6C, 0x7E, 0xE3, 0xF4, 0x5A, 0xB8,
	0x3B, 0x55, 0xBB, 0x73, 0xEB, 0xB8, 0x21, 0x04, 0x43, 0x7B, 0x34, 0x7A, 0xE2, 0xA5, 0x77, 0xE7, 0x62, 0x1D, 0xA2, 0xB5, 0x3D, 0x17, 0x3B, 0x0E, 0x62, 0xB4, 0x3B, 0x23, 0x87, 0x36, 0x32, 0x4E,
	0xF6, 0xB8, 0xEF, 0x74, 0x43, 0xF2, 0x66, 0xFB, 0xD4, 0x45, 0xE2, 0x10, 0xDE, 0xC8, 0x82, 0x60, 0xEC, 0x74, 0x39, 0x3D, 0x38, 0x5F, 0x1C, 0xF4, 0xC5, 0xF6, 0xA7, 0x56, 0x6D, 0x33, 0x70, 0xFE,
	0x9E, 0xE0, 0x24, 0x1C, 0x84, 0x9E, 0x7D, 0x63, 0x0F, 0x9C, 0x80, 0x9D, 0x7E, 0x87, 0x11, 0x0C, 0x49, 0x77, 0xE2, 0x3B, 0x37, 0x04, 0x19, 0x69, 0x08, 0x26, 0x9D, 0x20, 0xE4, 0xE1, 0x24, 0x74,
	0x60, 0xE0, 0x79, 0x3D, 0xC3, 0x73, 0xE0, 0xF8, 0xB7, 0xBC, 0xEB, 0x04, 0x9F, 0xD9, 0xC8, 0x0B, 0x0C, 0x59, 0x93, 0xC0, 0xC1, 0x72, 0xB2, 0x43, 0xDB, 0x6C, 0x8C, 0x26, 0x90, 0xA9, 0xE0, 0x33,
	0x3D, 0x77, 0x26, 0x01, 0x37, 0x9C, 0x71, 0x37, 0x74, 0x7C, 0x7F, 0x32, 0x0E, 0xB9, 0xE7, 0x9E, 0x61, 0x78, 0xEF, 0x90, 0x15, 0xC4, 0x68, 0xE3, 0xD2, 0x9E, 0x09, 0xA6, 0xE7, 0x02, 0xB9, 0x8A,
	0x04, 0x79, 0xFE, 0x94, 0x8C, 0x12, 0x07, 0x86, 0x7B, 0x0B, 0xEE, 0x86, 0x0E, 0xFE, 0xEE, 0x13, 0x9F, 0x86, 0x29, 0x9B, 0x28, 0x08, 0x90, 0xB1, 0x6E, 0xB8, 0x3B, 0x0D, 0xF7, 0x43, 0x02, 0xC3,
	0x1D, 0x1F, 0xC1, 0x75, 0x06, 0x23, 0x3E, 0x70, 0xDC, 0xAE, 0x43, 0xA3, 0x1E, 0x59, 0xB9, 0xE3, 0x81, 0x73, 0x86, 0xA1, 0xE2, 0x01, 0x4D, 0xE0, 0xD5, 0xB6, 0x77, 0x36, 0xEE, 0x39, 0x31, 0x2E,
	0x53, 0x88, 0x10, 0x55, 0xF5, 0xC8, 0x03, 0xD6, 0x24, 0xAC, 0x65, 0x02, 0x09, 0xBC, 0x0F, 0x76, 0xEF, 0x96, 0x13, 0xEC, 0x7A, 0x32, 0x86, 0xBE, 0xE9, 0x9F, 0x15, 0x65, 0xDD, 0x61, 0x4D, 0x77,
	0x8B, 0xBD, 0xB9, 0x69, 0xD9, 0xE5, 0x4A, 0xA6, 0x28, 0x40, 0x45, 0xAE, 0x0A, 0x53, 0xF0, 0xD4, 0xB6, 0xE0, 0x50, 0xDB, 0x7A, 0x36, 0xF3, 0xAD, 0xAF, 0x31, 0x88, 0x05, 0x4D, 0x1E, 0xF5, 0x53,
	0x99, 0x3C, 0xD4, 0x00, 0x3F, 0x68, 0xF2, 0x27, 0xA0, 0xFC, 0x78, 0x43, 0x7E, 0x66, 0xF2, 0x78, 0x0D, 0xFA, 0x67, 0x3A, 0x7E, 0x71, 0xAC, 0xD6, 0xBF, 0x35, 0x49, 0x22, 0xF0, 0x9A, 0x02, 0x6C,
	0xFB, 0x3E, 0xCC, 0xF0, 0x65, 0xEC, 0xA1, 0xF5, 0x6B, 0xF2, 0x92, 0x2A, 0xE5, 0x93, 0x86, 0x4C, 0xAC, 0xD0, 0x49, 0xF1, 0x48, 0x8E, 0x20, 0x1E, 0x0A, 0x3B, 0xC2, 0x54, 0x20, 0xB3, 0x58, 0x15,
	0x98, 0x02, 0x18, 0x61, 0x8C, 0xD6, 0x4A, 0xE1, 0xFB, 0x54, 0x5D, 0xA5, 0x1A, 0xDF, 0x22, 0x8B, 0xE4, 0x9E, 0x5E, 0x03, 0xA8, 0x38, 0x9F, 0x38, 0xBE, 0xA9, 0xDC, 0x26, 0x2F, 0x74, 0x2E, 0x23,
	0x4A, 0x04, 0x5C, 0x9E, 0x50, 0xBA, 0x14, 0x94, 0x02, 0x59, 0x95, 0x0C, 0x5A, 0x23, 0x33, 0xAD, 0xFF, 0xD7, 0xF1, 0x8D, 0x06, 0xF9, 0x98, 0x59, 0xAF, 0xAA, 0xAE, 0x1D, 0xDE, 0x38, 0xA3, 0x5D,
	0x51, 0x85, 0x27, 0xA2, 0xDA, 0x98, 0xFC, 0x98, 0xB8, 0xC2, 0xBE, 0xB8, 0x56, 0x26, 0x9F, 0x48, 0xAC, 0xF5, 0x06, 0x7D, 0x45, 0x7C, 0xAE, 0xE7, 0x5E, 0x70, 0xB7, 0xEF, 0xE3, 0xB6, 0x95, 0x4A,
	0x91, 0x57, 0xCF, 0x1C, 0x3F, 0x28, 0xBE, 0x95, 0xB3, 0x7B, 0xF2, 0x0A, 0x87, 0xE5, 0x75, 0x87, 0xCB, 0xF7, 0x2A, 0x2D, 0x1C, 0x52, 0xDA, 0xCA, 0xE4, 0x7B, 0xF5, 0x16, 0x0E, 0xE8, 0x6D, 0x6D,
	0xF2, 0xBD, 0xB2, 0x0B, 0xCF, 0x64, 0x77, 0xE3, 0xF8, 0xBB, 0xF5, 0x77, 0xC7, 0xF3, 0x27, 0x79, 0xF9, 0x71, 0x35, 0x86, 0xAD, 0x1A, 0x57, 0x26, 0x7F, 0x5C, 0x93, 0x5F, 0xAF, 0x9E, 0x97, 0xD4,
	0x3A, 0xC4, 0x18, 0x21, 0xE6, 0x7D, 0xD1, 0xA6, 0x56, 0xF7, 0x25, 0xC9, 0x97, 0x2D, 0x85, 0xC7, 0x87, 0xBE, 0xC2, 0xC3, 0x66, 0x2D, 0xDD, 0x4F, 0x75, 0xFB, 0xF2, 0xCA, 0x3A, 0x34, 0xF1, 0xF7,
	0x38, 0x25, 0x68, 0x46, 0x8B, 0x48, 0x86, 0x18, 0xFB, 0xA8, 0x56, 0xB0, 0xDD, 0x63, 0xC2, 0x0F, 0x9F, 0x11, 0x0E, 0xED, 0x8F, 0xFB, 0xEC, 0xF8, 0xDF, 0xEC, 0x5F, 0xC5, 0x47, 0x1E, 0x1D, 0x02,
	0x54, 0x6E, 0xB1, 0x58, 0x45, 0x6B, 0xBA, 0x41, 0x10, 0x4D, 0x58, 0xDA, 0xC8, 0xB8, 0xA2, 0x33, 0x3C, 0x0A, 0x49, 0x89, 0xDA, 0x20, 0x52, 0xBD, 0x65, 0xD7, 0x84, 0xA4, 0xD6, 0xBD, 0x0D, 0x74,
	0xE3, 0x8D, 0x2B, 0x13, 0xB3, 0x88, 0x06, 0x8D, 0x0C, 0x21, 0x94, 0x17, 0xB2, 0x06, 0xD5, 0x69, 0x3B, 0xCF, 0xF0, 0x9E, 0x94, 0x9A, 0x21, 0xEE, 0xCA, 0x22, 0x2A, 0x95, 0xD1, 0xB0, 0x77, 0xE8,
	0x17, 0x7B, 0xAF, 0x6A, 0x1D, 0xE5, 0xC8, 0xC6, 0xBE, 0x7E, 0x35, 0xF2, 0xF2, 0xE9, 0xD3, 0x21, 0x7D, 0x79, 0x9B, 0xAE, 0xB0, 0xB7, 0xE9, 0xCA, 0x77, 0x0E, 0x6D, 0xEC, 0xA5, 0x43, 0xDB, 0x9E,
	0x6E, 0xBC, 0x70, 0x6A, 0xEB, 0x7B, 0x13, 0x17, 0x5B, 0x2E, 0xF6, 0x51, 0xF6, 0xEA, 0x81, 0x0D, 0xBE, 0x7B, 0x60, 0x63, 0x1F, 0x95, 0x11, 0x76, 0x14, 0x01, 0x61, 0x1F, 0x3B, 0xB0, 0xD5, 0xCA,
	0xC1, 0x7E, 0x9B, 0x03, 0x1B, 0x7B, 0x2E, 0x11, 0xEF, 0x38, 0xB0, 0xBD, 0xED, 0xB4, 0x46, 0x65, 0x6A, 0x67, 0x71, 0xA1, 0x92, 0x18, 0xBC, 0x5C, 0x66, 0x17, 0x41, 0xD5, 0x22, 0xC7, 0x85, 0xFA,
	0x47, 0x46, 0xA5, 0xB9, 0x72, 0x82, 0xF3, 0xE7, 0xF7, 0x89, 0x57, 0xE6, 0xBA, 0x70, 0x77, 0x25, 0xEC, 0xAF, 0xC4, 0x55, 0xB4, 0x70, 0x54, 0x5D, 0x58, 0xC6, 0xF5, 0xFD, 0xA5, 0xD9, 0x2E, 0x17,
	0x11, 0xFE, 0xA9, 0x47, 0x2C, 0xB8, 0x95, 0x05, 0xD5, 0x28, 0x5C, 0xB5, 0x2E, 0xE1, 0x94, 0x26, 0x9C, 0xD4, 0x43, 0x27, 0x67, 0x9F, 0xC9, 0xC4, 0xA3, 0x5A, 0x6F, 0xDE, 0x79, 0x49, 0x52, 0x4C,
	0x0F, 0x30, 0x37, 0xB3, 0xF2, 0x5B, 0x24, 0x73, 0x72, 0x8D, 0xAE, 0x06, 0xF3, 0x34, 0x11, 0x59, 0x24, 0xB7, 0x2D, 0xAC, 0xB6, 0xD2, 0x22, 0x1B, 0xD3, 0xDA, 0x86, 0x9A, 0x99, 0x7E, 0x2F, 0x4C,
	0x93, 0x6D, 0x5A, 0x58, 0x3D, 0x11, 0x44, 0x03, 0xDA, 0x7C, 0xEA, 0xAB, 0xB2, 0x87, 0x87, 0x87, 0x96, 0x30, 0x88, 0xA9, 0xCB, 0xB5, 0xEB, 0x0B, 0x58, 0xDD, 0x1E, 0x61, 0x1E, 0x62, 0x96, 0x5F,
	0x20, 0xEA, 0x7A, 0xD5, 0x24, 0x4B, 0xA5, 0xA6, 0x83, 0xE3, 0xBF, 0xEB, 0xA4, 0x40, 0x8F, 0x67, 0x8F, 0x20, 0x72, 0x44, 0x15, 0x89, 0x19, 0x62, 0x4D, 0xC5, 0x03, 0xB5, 0x37, 0xB1, 0x28, 0x64,
	0xD5, 0xF3, 0x10, 0x06, 0x35, 0x2A, 0xEC, 0xDA, 0xD6, 0x26, 0x26, 0x64, 0x66, 0x7B, 0x27, 0xBA, 0x4B, 0x5A, 0x83, 0x11, 0x5D, 0xDF, 0x9D, 0x60, 0x5A, 0xFA, 0xE6, 0xAD, 0xB7, 0x63, 0x07, 0x3C,
	0xB0, 0xC8, 0xC8, 0x1D, 0x0F, 0x87, 0x94, 0x54, 0xBB, 0x5D, 0xC5, 0xD4, 0x65, 0x8F, 0x53, 0x19, 0x98, 0xD2, 0xA1, 0xE4, 0xFB, 0x0B, 0xEB, 0xDA, 0x82, 0xBA, 0xB5, 0xCB, 0x6F, 0x79, 0x41, 0x1E,
	0x20, 0xCC, 0x84, 0xE8, 0xA4, 0xDB, 0x3F, 0xB4, 0x15, 0x48, 0xB9, 0x07, 0x61, 0x5E, 0x77, 0xF7, 0x4D, 0xD7, 0x4D, 0x45, 0xB6, 0x58, 0x8B, 0x85, 0x84, 0x05, 0xCA, 0x57, 0x91, 0x91, 0x0E, 0x6D,
	0x5B, 0xAF, 0x51, 0x29, 0x32, 0x93, 0x26, 0x28, 0xE5, 0xA2, 0x12, 0xAE, 0x67, 0x7E, 0xD1, 0x46, 0xF8, 0xCA, 0xF2, 0x1F, 0xF8, 0xB9, 0x5A, 0xB2, 0x5A, 0x19, 0x00, 0x00
};


static void putLE32(uint8_t* buf, uint16_t* idx, uint32_t val){

  buf[(*idx)++] = val;
  buf[(*idx)++] = val >> 8;
  buf[(*idx)++] = val >> 16;
  buf[(*idx)++] = val >> 24;
}

static void putLE16(uint8_t* buf, uint16_t* idx, uint32_t val){

  buf[(*idx)++] = val;
  buf[(*idx)++] = val >> 8;
}

static uint16_t adkProcessCommand(uint8_t cmd, const uint8_t* dataIn, uint16_t sz, char fromBT, uint8_t* reply, uint16_t maxReplySz){  //returns num bytes to reply with (or 0 for no reply)

  uint16_t sendSz = 0;
  static FatFileP btFile = 0, usbFile = 0;
  static FatDirP btDir = 0, usbDir = 0;
  static uint16_t btLicPos = 0, usbLicPos = 0;
  FatFileP* filP = fromBT ? &btFile : &usbFile;
  FatDirP* dirP = fromBT ? &btDir : &usbDir;
  uint16_t* licPos = fromBT ? &btLicPos : &usbLicPos;

  dbgPrintf("ADK: BT: have cmd 0x%x with %db of data\n", cmd, sz);
  
  //NOTE: this code was written in a hurry and features little error checking. yes, I know it's bad. -DG
  
  //process packet
  switch(cmd){
      
    case BT_CMD_GET_PROTO_VERSION:
      {
        reply[sendSz++] = BT_PROTO_VERSION_CURRENT;
      }
      break;
      
    case BT_CMD_GET_SENSORS:
      {   
        putLE32(reply, &sendSz, hTemp);
        putLE32(reply, &sendSz, hHum);
        putLE32(reply, &sendSz, bPress);
        putLE32(reply, &sendSz, bTemp);
        putLE16(reply, &sendSz, prox[0]);
        putLE16(reply, &sendSz, prox[1]);
        putLE16(reply, &sendSz, prox[3]);
        putLE16(reply, &sendSz, prox[4]);
        putLE16(reply, &sendSz, prox[5]);
        putLE16(reply, &sendSz, prox[2]);
        putLE16(reply, &sendSz, prox[6]);
        putLE16(reply, &sendSz, accel[0]);
        putLE16(reply, &sendSz, accel[1]);
        putLE16(reply, &sendSz, accel[2]);
        putLE16(reply, &sendSz, mag[0]);
        putLE16(reply, &sendSz, mag[1]);
        putLE16(reply, &sendSz, mag[2]);
      }
      break;

    case BT_CMD_FILE_LIST:
      {
      
        if(sz){  //reset
        
          if(*dirP) L.fatfsCloseDir(*dirP);
          if(L.fatfsOpenDir(dirP, (char*)dataIn)) *dirP = 0;
        }
        if(*dirP){
         
          FatFileInfo fi;
          fi.longName = (char*)reply + 5;
          fi.nameSz = maxReplySz - 5;
          
          if(!L.fatfsReadDir(*dirP, &fi)){
            
            fi.longName[fi.nameSz - 1] = 0;
            reply[sendSz++] = fi.fsize;
            reply[sendSz++] = fi.fsize >> 8;
            reply[sendSz++] = fi.fsize >> 16;
            reply[sendSz++] = fi.fsize >> 24;
            reply[sendSz++] = fi.attrib;
            sendSz += strlen(fi.longName) + 1;
          }
          else reply[sendSz++] = 0;
        }
        else reply[sendSz++] = 0;
      }
      break;
       
    case BT_CMD_FILE_DELETE:
      {
        reply[sendSz++] = !L.fatfsUnlink((const char*)dataIn);
      }
      break;
       
    case BT_CMD_FILE_OPEN:
      {
        if(*filP) L.fatfsClose(*filP);
        reply[sendSz++] = !L.fatfsOpen(filP, (const char*)dataIn, FATFS_WRITE | FATFS_CREATE | FATFS_TRUNCATE);
        if(!reply[sendSz - 1]) *filP = 0;
      }
      break;
       
    case BT_CMD_FILE_WRITE:
      {
        uint32_t written;
        
        reply[sendSz++] = !L.fatfsWrite(*filP, (void*)dataIn, sz, &written) && written == sz;
      }
      break;
       
    case BT_CMD_FILE_CLOSE:
      {
        if(*filP){
          reply[sendSz++] = 1;
          L.fatfsClose(*filP);
          *filP = 0;
        }
        else reply[sendSz++] = 0;
      }
      break;
       
    case BT_CMD_GET_UNIQ_ID:
      {
         
        uint32_t id[4];
           
        L.getUniqueId(id);
        putLE32(reply, &sendSz, id[0]);
        putLE32(reply, &sendSz, id[1]);
        putLE32(reply, &sendSz, id[2]);
        putLE32(reply, &sendSz, id[3]);
      }
      break;
      
    case BT_CMD_BT_NAME:
      {
        if(sz){  //set
          strcpy(settings.btName, (char*)dataIn);
          reply[sendSz++] = 1;
          writeSettings();
        }
        else{
          strcpy((char*)reply, settings.btName);
          sendSz = strlen((char*)reply) + 1;
        }
      }
      break;
      
    case BT_CMD_BT_PIN:
      {
        if(sz){  //set
          strcpy(settings.btPIN, (char*)dataIn);
          reply[sendSz++] = 1;
          writeSettings();
        }
        else{
          strcpy((char*)reply, settings.btPIN);
          sendSz = strlen((char*)reply) + 1;
        }
      }
      break;
    case BT_CMD_TIME:
      {
        if (sz >= 7) {  //set
          L.rtcSet(dataIn[1] << 8 | dataIn[0], dataIn[2], dataIn[3], dataIn[4], dataIn[5], dataIn[6]);
          reply[sendSz++] = 1;
        } else if (sz == 0) {
          L.rtcGet((uint16_t *)&reply[0], &reply[2], &reply[3], &reply[4], &reply[5], &reply[6]);
          sendSz += 7;
        }
      }
      break;
    case BT_CMD_SETTINGS:
      {
        if (sz >= 8) {  //set
          settings.almH = dataIn[0];
          settings.almM = dataIn[1];
          settings.almOn = dataIn[2];
          settings.bri = dataIn[3];
          settings.R = dataIn[4];
          settings.G = dataIn[5];
          settings.B = dataIn[6];
          settings.vol = dataIn[7];
          L.setVolume(settings.vol);
          writeSettings();
          reply[sendSz++] = 1;
        } else {
          reply[sendSz++] = settings.almH;
          reply[sendSz++] = settings.almM;
          reply[sendSz++] = settings.almOn;
          reply[sendSz++] = settings.bri;
          reply[sendSz++] = settings.R;
          reply[sendSz++] = settings.G;
          reply[sendSz++] = settings.B;
          reply[sendSz++] = settings.vol;
        }
      }
      break;
    case BT_CMD_ALARM_FILE:
      {
        if(sz){  //set
          strcpy(settings.almTune, (char*)dataIn);
          reply[sendSz++] = 1;
          writeSettings();
        } else{
          strcpy((char*)reply, settings.almTune);
          sendSz = strlen((char*)reply) + 1;
        }
      }
      break;
    case BT_CMD_GET_LICENSE:
      {
        static const uint32_t maxPacket = MAX_PACKET_SZ - 10;	//seems reasonable

        if(*licPos >= sizeof(gzippedLicences)){	//send terminator
          reply[sendSz++] = 0;
          *licPos = 0;
        }
        else{

          uint32_t left = sizeof(gzippedLicences) - *licPos;
          if(left > maxPacket) left = maxPacket;
          reply[sendSz++] = 1;
          while(left--) reply[sendSz++] = gzippedLicences[(*licPos)++];
        }
      }
      break;
    case BT_CMD_DISPLAY_MODE:
      {
        if (sz) {  //set
          settings.displayMode = dataIn[0];
          reply[sendSz++] = 1;
        } else if (sz == 0) {
          reply[sendSz++] = settings.displayMode;
        }
      }
      break;
    case BT_CMD_LOCK:
      {
        if (sz) {  //set
          locked = dataIn[0] ? 2 : 0;
          reply[sendSz++] = 1;
        } else if (sz == 0) {
          reply[sendSz++] = locked;
        }
      }
      break;
  }
  return sendSz;
}

static uint8_t cmdBuf[MAX_PACKET_SZ];
static uint32_t bufPos = 0;


static void btAdkPortOpen(void* port, uint8_t dlci){

    bufPos = 0;
}

static void btAdkPortClose(void* port, uint8_t dlci){

    //nothing here [yet?]
}


static void btAdkPortRx(void* port, uint8_t dlci, const uint8_t* data, uint16_t sz){

    uint8_t reply[MAX_PACKET_SZ];
    uint32_t i;
    uint8_t seq, cmd;
    uint16_t cmdSz;
    uint8_t* ptr;
      
    while(sz || bufPos){
      
      uint16_t sendSz = 0;
      
      //copy to buffer as much as we can
      while(bufPos < MAX_PACKET_SZ && sz){
        cmdBuf[bufPos++] = *data++;
        sz--;
      }
      
      //see if a packet exists
      if(bufPos < 4) return; // too small to be a packet -> discard
      cmd = cmdBuf[0];
      seq = cmdBuf[1];
      cmdSz = cmdBuf[3];
      cmdSz <<= 8;
      cmdSz += cmdBuf[2];
            
      if(bufPos - 4 < cmdSz) return; //not entire command received yet
      
      sendSz = adkProcessCommand(cmd, cmdBuf + 4, cmdSz, 1, reply + 4, MAX_PACKET_SZ - 4);
      if(sendSz){
                
        reply[0] = cmd | CMD_MASK_REPLY;
        reply[1] = seq;
        reply[2] = sendSz;
        reply[3] = sendSz >> 8;
        sendSz += 4;
      
        L.btRfcommPortTx(port, dlci, reply, sendSz);
      }
      
      //adjust buffer as needed
      for(i = 0; i < bufPos - cmdSz - 4; i++){
        cmdBuf[i] =  cmdBuf[i + cmdSz + 4];
      }
      bufPos = i;
    }
}






//////////// bt support (boring)


static const uint8_t maxPairedDevices = 4;
static uint8_t numPairedDevices = 0;
static uint8_t savedMac[maxPairedDevices][BLUETOOTH_MAC_SIZE];
static uint8_t savedKey[maxPairedDevices][BLUETOOTH_LINK_KEY_SIZE];

static char adkBtConnectionRequest(const uint8_t* mac, uint32_t devClass, uint8_t linkType){	//return 1 to accept

    Serial.print("Accepting connection from ");
    Serial.print(mac[5], HEX);
    Serial.print(":");
    Serial.print(mac[4], HEX);
    Serial.print(":");
    Serial.print(mac[3], HEX);
    Serial.print(":");
    Serial.print(mac[2], HEX);
    Serial.print(":");
    Serial.print(mac[1], HEX);
    Serial.print(":");
    Serial.println(mac[0], HEX);
    return 1;
}

static char adkBtLinkKeyRequest(const uint8_t* mac, uint8_t* buf){ //link key create

  uint8_t i, j;
  
  Serial.print("Key request from ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.print(mac[0], HEX);
  Serial.print(" -> ");
    
  for(i = 0; i < numPairedDevices; i++){
 
    for(j = 0; j < BLUETOOTH_MAC_SIZE && savedMac[i][j] == mac[j]; j++);
    if(j == BLUETOOTH_MAC_SIZE){ //match
    
        Serial.print("{");
        for(j = 0; j < BLUETOOTH_LINK_KEY_SIZE; j++){
         
          Serial.print(" ");
          Serial.print(savedKey[i][j], HEX);
          buf[j] = savedKey[i][j];
        }
        Serial.println(" }");
        return 1;
    }
  }
  Serial.println("FAIL");
  return 0;
}

static void adkBtLinkKeyCreated(const uint8_t* mac, const uint8_t* buf){ 	//link key was just created, save it if you want it later

   uint8_t j;
   
   Serial.print("Key created for ");
   Serial.print(mac[5], HEX);
   Serial.print(":");
   Serial.print(mac[4], HEX);
   Serial.print(":");
   Serial.print(mac[3], HEX);
   Serial.print(":");
   Serial.print(mac[2], HEX);
   Serial.print(":");
   Serial.print(mac[1], HEX);
   Serial.print(":");
   Serial.print(mac[0], HEX);
   Serial.print(" <- ");
    
   Serial.print("{");
   for(j = 0; j < BLUETOOTH_LINK_KEY_SIZE; j++){
         
     Serial.print(" ");
     Serial.print(buf[j], HEX);
   }
   Serial.print(" }");
    
   if(numPairedDevices < maxPairedDevices){
      
      for(j = 0; j < BLUETOOTH_LINK_KEY_SIZE; j++) savedKey[numPairedDevices][j] = buf[j];
      for(j = 0; j < BLUETOOTH_MAC_SIZE; j++) savedMac[numPairedDevices][j] = mac[j];
      numPairedDevices++;
      Serial.print("saved to slot ");
      Serial.print(numPairedDevices);
      Serial.print("/");
      Serial.println(maxPairedDevices);
   }
   else{
      Serial.println("out of slots...discaring\n");
   }
}

static char adkBtPinRequest(const uint8_t* mac, uint8_t* buf){		//fill buff with PIN code, return num bytes used (16 max) return 0 to decline

   uint8_t v, i = 0;
   
   Serial.print("PIN request from ");
   Serial.print(mac[5], HEX);
   Serial.print(":");
   Serial.print(mac[4], HEX);
   Serial.print(":");
   Serial.print(mac[3], HEX);
   Serial.print(":");
   Serial.print(mac[2], HEX);
   Serial.print(":");
   Serial.print(mac[1], HEX);
   Serial.print(":");
   Serial.print(mac[0], HEX);
   
   if(btPIN){
     Serial.print(" -> using pin '");
     Serial.print((char*)btPIN);
     Serial.println("'");
     for(i = 0; btPIN[i]; i++) buf[i] = btPIN[i];
     return i;
   }
   else Serial.println(" no PIN set. rejecting");
   return 0;
}

#define MAGIX	0xFA

static uint8_t sdpDescrADK[] =
{
        //service class ID list
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x00, 0x01, SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 17,
            SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_16), BT_ADK_UUID,
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
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x01, 0x00, SDP_ITEM_DESC(SDP_TYPE_TEXT, SDP_SZ_u8), 12, 'A', 'D', 'K', ' ', 'B', 'T', ' ', 'C', 'O', 'M', 'M', 'S'
};

void btStart(){
    uint8_t i, dlci;
    int f;
  
    L.btEnable(adkBtConnectionRequest, adkBtLinkKeyRequest, adkBtLinkKeyCreated, adkBtPinRequest, NULL);

    dlci = L.btRfcommReserveDlci(RFCOMM_DLCI_NEED_EVEN);

    if(!dlci) dbgPrintf("BTADK: failed to allocate DLCI\n");
    else{

        //change descriptor to be valid...
        for(i = 0, f = -1; i < sizeof(sdpDescrADK); i++){

            if(sdpDescrADK[i] == MAGIX){
                if(f == -1) f = i;
                else break;
            }
        }

        if(i != sizeof(sdpDescrADK) || f == -1){

            dbgPrintf("BTADK: failed to find a single marker in descriptor\n");
            L.btRfcommReleaseDlci(dlci);
            return;
        }

        sdpDescrADK[f] = dlci >> 1;

        dbgPrintf("BTADK has DLCI %u\n", dlci);

        L.btRfcommRegisterPort(dlci, btAdkPortOpen, btAdkPortClose, btAdkPortRx);
        L.btSdpServiceDescriptorAdd(sdpDescrADK, sizeof(sdpDescrADK));
    }
}

// USB accessory
static void processUSBAccessory()
{
  if (!L.accessoryConnected())
    return;

  uint8_t receiveBuf[MAX_PACKET_SZ];
  uint8_t reply[MAX_PACKET_SZ];

  int res = L.accessoryReceive(receiveBuf, sizeof(receiveBuf));
  if (res >= 4) {
    uint8_t cmd = receiveBuf[0];
    uint8_t seq = receiveBuf[1];
    uint16_t size = receiveBuf[2] | receiveBuf[3] << 8;

    if (size + 4 > res) {
      // short packet
      return;
    }
    
    uint16_t replylen = adkProcessCommand(cmd, receiveBuf + 4, size, 0, reply + 4, MAX_PACKET_SZ - 4);
    if (replylen > 0) {
      reply[0] = cmd | CMD_MASK_REPLY;
      reply[1] = seq;
      reply[2] = replylen;
      reply[3] = replylen >> 8;
      replylen += 4;
      
      dbgPrintf("ADK: USB: sending %d bytes\n", replylen);
      L.accessorySend(reply, replylen);
    }
  }
}

