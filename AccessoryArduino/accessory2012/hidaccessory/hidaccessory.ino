#include "Arduino.h"
#include <ADK.h>

#define  LED3_RED       2
#define  LED3_GREEN     4
#define  LED3_BLUE      3

#define  BUTTON1        A6
#define  BUTTON2        A7
#define  BUTTON3        A8

// Report Descriptor Used in Library
//			uint8_t reportDesc[] =
//			{
//			  0x05, 0x0C,	// Usage Page (Consumer Devices)
//			  0x09, 0x01,	// Usage (Consumer Control)
//			  0xA1, 0x01,	// Collection (Application)
//			  0x15, 0x00,	//   Logical Minimum (0)
//			  0x25, 0x01,	//   Logical Maximum (1)
//			  0x09, 0xB5,	//   Usage (Scan Next)
//			  0x09, 0xB6,	//   Usage (Scan Prev)
//			  0x09, 0xB7,	//   Usage (Stop)
//			  0x09, 0xCD,	//   Usage (Play/Pause)
//			  0x09, 0xE2,	//   Usage (Mute)
//			  0x09, 0xE9,	//   Usage (Volume Up)
//			  0x09, 0xEA,	//   Usage (Volume Down)
//			  0x75, 0x01,	//   Report Size (1)
//			  0x95, 0x07,	//   Report Count (7)
//			  0x81, 0x02,	//   Input (Data, Variable, Absolute)
//			  0x95, 0x01,   //   Report Count (1)
//			  0x81, 0x01,	//   Input (Constant)
//			  0xC0			// End Collection
//			};


ADK acc;

#define ACCESSORY_STRING_VENDOR "Xcellent Creations"
#define ACCESSORY_STRING_NAME "HIDDemo"
#define ACCESSORY_STRING_LONGNAME "AnDevCon HID Demo"
#define ACCESSORY_STRING_VERSION  "1.0"
#define ACCESSORY_STRING_URL    "http://www.andevcon.com"
#define ACCESSORY_STRING_SERIAL "0000000012345678"

void adkPutchar(char c){Serial.write(c);}
extern "C" void dbgPrintf(const char *, ... );

void init_buttons()
{
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(BUTTON3, INPUT);

  // enable the internal pullups
  digitalWrite(BUTTON1, HIGH);
  digitalWrite(BUTTON2, HIGH);
  digitalWrite(BUTTON3, HIGH);
}

void init_leds()
{
  digitalWrite(LED3_RED, 1);
  digitalWrite(LED3_GREEN, 1);
  digitalWrite(LED3_BLUE, 1);

  pinMode(LED3_RED, OUTPUT);
  pinMode(LED3_GREEN, OUTPUT);
  pinMode(LED3_BLUE, OUTPUT);
}

byte b1, b2, b3, conn;
void setup()
{
  //Set up serial debugging
  Serial.begin(115200);
  acc.adkSetPutchar(adkPutchar);
  
  dbgPrintf("AnDevCon Accessory\n");
  
  init_buttons();

  acc.adkInit();

  acc.usbSetAccessoryStringVendor(ACCESSORY_STRING_VENDOR);
  acc.usbSetAccessoryStringName(ACCESSORY_STRING_NAME);
  acc.usbSetAccessoryStringLongname(ACCESSORY_STRING_LONGNAME);
  acc.usbSetAccessoryStringVersion(ACCESSORY_STRING_VERSION);
  acc.usbSetAccessoryStringUrl(ACCESSORY_STRING_URL);
  acc.usbSetAccessoryStringSerial(ACCESSORY_STRING_SERIAL);

  acc.usbStart();
  
  //Get initial button readings
  b1 = digitalRead(BUTTON1);
  b2 = digitalRead(BUTTON2);
  b3 = digitalRead(BUTTON3);
  
  //Set initial LED color
  analogWrite(LED3_RED, 1);
  analogWrite(LED3_GREEN, 255);
  analogWrite(LED3_BLUE, 255);
}

void write_report(byte *msg, int len)
{
  if (acc.accessoryConnected()) {
    dbgPrintf("Write %d :: ", msg[0]);
    acc.accessorySendHidEvent(msg, len);
  }
}

void loop()
{
  byte temp;
  byte msg[1];
  
  //Vol Up
  temp = digitalRead(BUTTON1);
  if (b1 != temp) {
    msg[0] = temp ? 0 : 32;
    write_report(msg, 1);
    b1 = temp;
  }

  //Vol Dn
  temp = digitalRead(BUTTON2);
  if (b2 != temp) {
    msg[0] = temp ? 0 : 64;
    write_report(msg, 1);
    b2 = temp;
  }

  //Play/Pause Button
  temp = digitalRead(BUTTON3);
  if (b3 != temp) {
    msg[0] = temp ? 0 : 8;
    write_report(msg, 1);
    b3 = temp;
  }
  
  temp = acc.accessoryConnected();
  if (temp != conn) {
    if (temp)
    {
      analogWrite(LED3_RED, 255);
      analogWrite(LED3_GREEN, 1);
      analogWrite(LED3_BLUE, 255);
    }
    else
    {
      analogWrite(LED3_RED, 1);
      analogWrite(LED3_GREEN, 255);
      analogWrite(LED3_BLUE, 255);
    }
    conn = temp;
  }
  
  acc.adkEventProcess(); //Let library process messages
  delay(10);
}

