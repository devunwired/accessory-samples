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
#include "Arduino.h"
#include <ADK.h>

ADK L;

// ADK1 usb accessory strings
#define ACCESSORY_STRING_VENDOR "Google, Inc."
#define ACCESSORY_STRING_NAME   "DemoKit"
#define ACCESSORY_STRING_LONGNAME "DemoKit Arduino Board"
#define ACCESSORY_STRING_VERSION  "1.0"
#define ACCESSORY_STRING_URL    "http://www.android.com"
#define ACCESSORY_STRING_SERIAL "0000000012345678"

void adkPutchar(char c){Serial.write(c);}
extern "C" void dbgPrintf(const char *, ... );

void setup(void)
{
  Serial.begin(115200);

  L.adkSetPutchar(adkPutchar);
  L.adkInit();
  
  // set the old accessory strings
  L.usbSetAccessoryStringVendor(ACCESSORY_STRING_VENDOR);
  L.usbSetAccessoryStringName(ACCESSORY_STRING_NAME);
  L.usbSetAccessoryStringLongname(ACCESSORY_STRING_LONGNAME);
  L.usbSetAccessoryStringVersion(ACCESSORY_STRING_VERSION);
  L.usbSetAccessoryStringUrl(ACCESSORY_STRING_URL);
  L.usbSetAccessoryStringSerial(ACCESSORY_STRING_SERIAL);
  
  L.usbStart();
}

struct SendBuf {
  void Reset() { pos = 0; memset(buf, 0, sizeof(buf)); }
  void Append(int val) { buf[pos++] = val; }
  void Append(uint8_t val) { buf[pos++] = val; }
  void Append(uint16_t val) { buf[pos++] = val >> 8; buf[pos++] = val; }
  void Append(uint32_t val) { buf[pos++] = val >> 24; buf[pos++] = val >> 16; buf[pos++] = val >> 8; buf[pos++] = val; }

  int Send() { return L.accessorySend(buf, pos); }

  uint8_t buf[128];
  int pos;
};

void loop()
{
  static int last_report = 0;
  static uint16_t temp = 0;
  static uint16_t light = 0;
  static uint8_t buttons = 0;
  static uint8_t joyx = 0;
  static uint8_t joyy = 0;
  static uint8_t ledrgb[6][3];

  int now = millis() / 100;
  
  // see if we need to report our current status
  if (now != last_report) {
    SendBuf buf;
    buf.Reset();

    // temperature
    buf.Append(0x04);
    buf.Append(temp);
    temp++;

    // light<
    buf.Append(0x05);
    buf.Append(light);
    light++;

    // buttons
    for (int i = 0; i < 5; i++) {
      buf.Append(0x01);
      buf.Append(i);
      buf.Append((uint8_t)(buttons & (1 << i)));
    }
    buttons++;

    // joystick
    buf.Append(0x06);
    buf.Append((uint8_t)(L.capSenseSlider() - 128));
    buf.Append(0);
    joyx += 2;
    joyy += 3;

    buf.Send();
  }

  // read from phone
  {
    uint8_t buf[64];

    int res = L.accessoryReceive(buf, sizeof(buf));
 
    int pos = 0;
    while (pos < res) {
      uint8_t op = buf[pos++];
  
      switch (op) {
        case 0x2: {
          // color sliders
          unsigned int led = buf[pos];
          
          if (led < 12) {
            ledrgb[led/3][led%3] = buf[pos+1]; 
          } else if (led >= 0x10 || led <= 0x12) {
            ledrgb[3][led%3] = buf[pos+1];            
          }

          pos += 2;
          break;
        }
        case 0x3: {
          // relay
          int val = buf[pos+1] ? 255 : 0;
          int led = (buf[pos] == 0) ? 4 : 5;
          ledrgb[led][0] = val;              
          ledrgb[led][1] = val;              
          ledrgb[led][2] = val;              
          pos += 2;
          break;
        }
        default: // assume 3 byte packet
          pos += 2;
          break;
      }    
    }
  }
  
  for (int i = 0; i < 6; i++) {
    L.ledDrawLetter(i, '8', ledrgb[i][0], ledrgb[i][1], ledrgb[i][2]);
  }

  last_report = now;
  
  L.adkEventProcess(); //let the adk framework do its thing
}
