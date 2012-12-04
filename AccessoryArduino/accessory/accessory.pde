#include <Max3421e.h>
#include <Usb.h>
#include <AndroidAccessory.h>

#define  JOY_LAT        A4
#define  JOY_VERT       A5
#define  JOY_SW         6

#define  MOTOR          7
#define  BUTTON_B       4
#define  BUTTON_A       5

AndroidAccessory acc("Xcellent Creations",
"AccessoryController",
"AnDevCon Accessory Demo",
"1.0",
"http://www.andevcon.com",
"0000000012345678");

void setup();
void loop();

void init_buttons()
{
  pinMode(BUTTON_A, INPUT);
  pinMode(BUTTON_B, INPUT);
  pinMode(JOY_SW, INPUT);

  // enable the internal pullups
  digitalWrite(BUTTON_A, HIGH);
  digitalWrite(BUTTON_B, HIGH);
  digitalWrite(JOY_SW, HIGH);
}


void init_motor()
{
  pinMode(MOTOR, OUTPUT);
  digitalWrite(MOTOR, HIGH);
}

unsigned int initLat, initVert;
void init_joystick() {
  initLat = analogRead(JOY_LAT);
  initVert = analogRead(JOY_VERT);
}

void read_joystick(int *x, int *y)
{
  *x = analogRead(JOY_LAT) - initLat;
  *y = analogRead(JOY_VERT) - initVert;
}

void write_msg(byte *msg, int len)
{
  if(acc.isConnected())
  {
    //Write to USB Accessory
    acc.write(msg, len);
  } 
  else
  {
    //Write to serial
    Serial.write(msg, len);
  }
}

int read_msg(byte *msg)
{
  int len = 0;
  if (acc.isConnected())
  {
    len = acc.read(msg, sizeof(msg), 1);
  }
  else if (Serial.available())
  {
    while (Serial.available())
    {
      msg[len++] = Serial.read(); 
    }
  }
  
  return len;
}

byte b1, b2, b3;
int x1, y1;
byte pulse_count;
void setup()
{
  //Init the serial for Bluetooth COMM 
  Serial.begin(115200);

  init_motor();
  init_buttons();
  init_joystick();

  //Get initial readings
  b1 = digitalRead(BUTTON_A);
  b2 = digitalRead(BUTTON_B);
  b3 = digitalRead(JOY_SW);
  read_joystick(&x1, &y1);

  acc.powerOn();
}

void loop()
{
  int len;
  byte msg[3];

  byte b;
  uint16_t val;
  int x, y;

  /* Read data from device and update outputs */
  len = read_msg(msg);
  if (len > 0)
  {
    if (msg[0] == 0x2)
    {
      pulse_count = msg[1];
    }
  }

  if (pulse_count > 0)
  {
    digitalWrite(MOTOR, LOW);
    pulse_count--;
  }
  else
  {
    digitalWrite(MOTOR, HIGH);
  }

  /* Transmit data from all inputs to device */

  msg[0] = 0x1;

  b = digitalRead(BUTTON_A);
  if (b != b1)
  {
    msg[1] = 0;
    msg[2] = b ? 0 : 1;
    write_msg(msg, 3);
    b1 = b;
  }

  b = digitalRead(BUTTON_B);
  if (b != b2)
  {
    msg[1] = 1;
    msg[2] = b ? 0 : 1;
    write_msg(msg, 3);
    b2 = b;
  }

  b = digitalRead(JOY_SW);
  if (b != b3)
  {
    msg[1] = 2;
    msg[2] = b ? 0 : 1;
    write_msg(msg, 3);
    b3 = b;
  }

  read_joystick(&x, &y);
  if (abs(x-x1) > 5 || abs(y-y1) > 5)
  {
    //Scale values down to fit into one byte
    x = x / 4;
    y = y / 4;
    
    msg[0] = 0x6;
    msg[1] = constrain(x, -128, 127);
    msg[2] = constrain(y, -128, 127);
    write_msg(msg, 3);
    x1 = x;
    y1 = y;
  }

  delay(10);
}

