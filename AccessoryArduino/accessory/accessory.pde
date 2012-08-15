#include <Wire.h>
#include <Servo.h>

#include <Max3421e.h>
#include <Usb.h>
#include <AndroidAccessory.h>

#include <CapSense.h>

#define  LED3_RED       2
#define  LED3_GREEN     4
#define  LED3_BLUE      3

#define  LED2_RED       5
#define  LED2_GREEN     7
#define  LED2_BLUE      6

#define  LED1_RED       8
#define  LED1_GREEN     10
#define  LED1_BLUE      9

#define  SERVO1         11
#define  SERVO2         12
#define  SERVO3         13

#define  TOUCH_RECV     14
#define  TOUCH_SEND     15

#define  RELAY1         A0
#define  RELAY2         A1

#define  LIGHT_SENSOR   A2
#define  TEMP_SENSOR    A3

#define  BUTTON1        A6
#define  BUTTON2        A7
#define  BUTTON3        A8

#define  JOY_SWITCH     A9      // pulls line down when pressed
#define  JOY_nINT       A10     // active low interrupt input
#define  JOY_nRESET     A11     // active low reset output

AndroidAccessory acc("Acme, Inc.",
"AccessoryController",
"AnDevCon Accessory Demo",
"1.0",
"http://www.andevcon.com",
"0000000012345678");
Servo servos[3];

// 10M ohm resistor on demo shield
CapSense   touch_robot = CapSense(TOUCH_SEND, TOUCH_RECV);

void setup();
void loop();

void init_buttons()
{
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(BUTTON3, INPUT);
  pinMode(JOY_SWITCH, INPUT);

  // enable the internal pullups
  digitalWrite(BUTTON1, HIGH);
  digitalWrite(BUTTON2, HIGH);
  digitalWrite(BUTTON3, HIGH);
  digitalWrite(JOY_SWITCH, HIGH);
}


void init_relays()
{
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
}


void init_leds()
{
  digitalWrite(LED1_RED, 1);
  digitalWrite(LED1_GREEN, 1);
  digitalWrite(LED1_BLUE, 1);

  pinMode(LED1_RED, OUTPUT);
  pinMode(LED1_GREEN, OUTPUT);
  pinMode(LED1_BLUE, OUTPUT);

  digitalWrite(LED2_RED, 1);
  digitalWrite(LED2_GREEN, 1);
  digitalWrite(LED2_BLUE, 1);

  pinMode(LED2_RED, OUTPUT);
  pinMode(LED2_GREEN, OUTPUT);
  pinMode(LED2_BLUE, OUTPUT);

  digitalWrite(LED3_RED, 1);
  digitalWrite(LED3_GREEN, 1);
  digitalWrite(LED3_BLUE, 1);

  pinMode(LED3_RED, OUTPUT);
  pinMode(LED3_GREEN, OUTPUT);
  pinMode(LED3_BLUE, OUTPUT);
}

void init_joystick(int threshold);

void write_msg(byte *msg, int len)
{
  if(acc.isConnected())
  {
    //Write to USB Accessory
    acc.write(msg, len);
  } 
  else {
    //Write to serial
    Serial.write(msg, len);
  }
}

byte b1, b2, b3, b4, c;
void setup()
{
  //Init the serial for Bluetooth COMM 
  Serial.begin(9600);

  init_leds();
  init_relays();
  init_buttons();
  init_joystick( 5 );

  // autocalibrate OFF
  touch_robot.set_CS_AutocaL_Millis(0xFFFFFFFF);

  servos[0].attach(SERVO1);
  servos[0].write(90);
  servos[1].attach(SERVO2);
  servos[1].write(90);
  servos[2].attach(SERVO3);
  servos[2].write(90);


  b1 = digitalRead(BUTTON1);
  b2 = digitalRead(BUTTON2);
  b3 = digitalRead(BUTTON3);
  b4 = digitalRead(JOY_SWITCH);
  c = 0;

  acc.powerOn();
}

void loop()
{
  byte err;
  byte idle;
  static byte count = 0;
  byte msg[3];
  long touchcount;

  int i;
  byte b;
  uint16_t val;
  int x, y;
  char c0;

  msg[0] = 0x1;

  b = digitalRead(BUTTON1);
  if (b != b1) {
    msg[1] = 0;
    msg[2] = b ? 0 : 1;
    write_msg(msg, 3);
    b1 = b;
  }

  b = digitalRead(BUTTON2);
  if (b != b2) {
    msg[1] = 1;
    msg[2] = b ? 0 : 1;
    write_msg(msg, 3);
    b2 = b;
  }

  b = digitalRead(BUTTON3);
  if (b != b3) {
    msg[1] = 2;
    msg[2] = b ? 0 : 1;
    write_msg(msg, 3);
    b3 = b;
  }

  b = digitalRead(JOY_SWITCH);
  if (b != b4) {
    msg[1] = 4;
    msg[2] = b ? 0 : 1;
    write_msg(msg, 3);
    b4 = b;
  }

  switch (count++ % 0x10) {
  case 0:
    val = analogRead(TEMP_SENSOR);
    msg[0] = 0x4;
    msg[1] = val >> 8;
    msg[2] = val & 0xff;
    write_msg(msg, 3);
    break;

  case 0x4:
    val = analogRead(LIGHT_SENSOR);
    msg[0] = 0x5;
    msg[1] = val >> 8;
    msg[2] = val & 0xff;
    write_msg(msg, 3);
    break;

  case 0x8:
    read_joystick(&x, &y);
    msg[0] = 0x6;
    msg[1] = constrain(x, -128, 127);
    msg[2] = constrain(y, -128, 127);
    write_msg(msg, 3);
    break;

  case 0xc:
    touchcount = touch_robot.capSense(5);

    c0 = touchcount > 750;

    if (c0 != c) {
      msg[0] = 0x1;
      msg[1] = 3;
      msg[2] = c0;
      write_msg(msg, 3);
      c = c0;
    }

    break;
  }

  delay(10);
}

// ==============================================================================
// Austria Microsystems i2c Joystick
void init_joystick(int threshold)
{
  byte status = 0;

  pinMode(JOY_SWITCH, INPUT);
  digitalWrite(JOY_SWITCH, HIGH);

  pinMode(JOY_nINT, INPUT);
  digitalWrite(JOY_nINT, HIGH);

  pinMode(JOY_nRESET, OUTPUT);

  digitalWrite(JOY_nRESET, 1);
  delay(1);
  digitalWrite(JOY_nRESET, 0);
  delay(1);
  digitalWrite(JOY_nRESET, 1);

  Wire.begin();

  do {
    status = read_joy_reg(0x0f);
  } 
  while ((status & 0xf0) != 0xf0);

  // invert magnet polarity setting, per datasheet
  write_joy_reg(0x2e, 0x86);

  calibrate_joystick(threshold);
}


int offset_X, offset_Y;

void calibrate_joystick(int dz)
{
  char iii;
  int x_cal = 0;
  int y_cal = 0;

  // Low Power Mode, 20ms auto wakeup
  // INTn output enabled
  // INTn active after each measurement
  // Normal (non-Reset) mode
  write_joy_reg(0x0f, 0x00);
  delay(1);

  // dummy read of Y_reg to reset interrupt
  read_joy_reg(0x11);

  for(iii = 0; iii != 16; iii++) {
    while(!joystick_interrupt()) {
    }

    x_cal += read_joy_reg(0x10);
    y_cal += read_joy_reg(0x11);
  }

  // divide by 16 to get average
  offset_X = -(x_cal>>4);
  offset_Y = -(y_cal>>4);

  write_joy_reg(0x12, dz - offset_X);  // Xp, LEFT threshold for INTn
  write_joy_reg(0x13, -dz - offset_X);  // Xn, RIGHT threshold for INTn
  write_joy_reg(0x14, dz - offset_Y);  // Yp, UP threshold for INTn
  write_joy_reg(0x15, -dz - offset_Y);  // Yn, DOWN threshold for INTn

  // dead zone threshold detect requested?
  if (dz)
    write_joy_reg(0x0f, 0x04);
}


void read_joystick(int *x, int *y)
{
  *x = read_joy_reg(0x10) + offset_X;
  *y = read_joy_reg(0x11) + offset_Y;  // reading Y clears the interrupt
}

char joystick_interrupt()
{
  return digitalRead(JOY_nINT) == 0;
}


#define  JOY_I2C_ADDR    0x40

char read_joy_reg(char reg_addr)
{
  char c;

  Wire.beginTransmission(JOY_I2C_ADDR);
  Wire.send(reg_addr);
  Wire.endTransmission();

  Wire.requestFrom(JOY_I2C_ADDR, 1);

  while(Wire.available())
    c = Wire.receive();

  return c;
}

void write_joy_reg(char reg_addr, char val)
{
  Wire.beginTransmission(JOY_I2C_ADDR);
  Wire.send(reg_addr);
  Wire.send(val);
  Wire.endTransmission();
}

