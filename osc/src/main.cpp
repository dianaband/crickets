//arduino
#include <Arduino.h>

//osc
#include <OSCBundle.h>
#ifdef SLIP_USBSERIAL
#include <SLIPEncodedUSBSerial.h>
SLIPEncodedUSBSerial SLIPSerial(Serial);

//i2c
#include <Wire.h>
const int16_t I2C_ADDR = 3;
#define POST_LENGTH 32
#define POST_BUFF_LEN (POST_LENGTH + 1)
//==========<protocol>==========
// postman's protocol
// letter frame ( '[' + 30 bytes + ']' )
//    : [123456789012345678901234567890]
// 'MIDI' letter frame
//    : [123456789012345678901234567890]
//    : [KKKVVVG.......................]
//    : KKK - Key
//      .substring(1, 4);
//    : VVV - Velocity (volume/amp.)
//      .substring(4, 7);
//    : G - Gate (note on/off)
//      .substring(7, 8);
//==========</protocol>=========
void receiveEvent(int howMany) {
  // nothing to expect.. but.. let's just print out..
  while (Wire.available())
    Serial.print(Wire.read());
  Serial.println();
}
// well. i don't know how fast should i be able to send msg.. to the net..
// first test, and then.. if needed. will come back.
bool new_letter;
char letter_outro[POST_BUFF_LEN] = "................................";
void requestEvent() {
  if (random(1000) == 0) {
    Wire.write("[bcdefghabcdefghabcdefghabcdefg]"); //32 bytes
  } else {
    Wire.write(" "); // no letter to send
  }
}

//
void midinote(OSCMessage &msg, int offset) {
  // msg.match("/oncnt");
  if (msg.match("/onoff")) {
    msg.match("/velocity");
    msg.match("/pitch");
  }
}

//
void setup() {
  //i2c
  Wire.begin(I2C_ADDR);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);

  //osc
  SLIPSerial.begin(57600);
}

void loop() {
  //osc
  OSCBundle bundleIN;
  int size;
  while(!SLIPSerial.endofPacket()) {
    if( (size = SLIPSerial.available()) > 0) {
      while(size--) {
        bundleIN.fill(SLIPSerial.read());
      }
    }
  }
  if(!bundleIN.hasError()) {
    bundleIN.route("/note", midinote);
  }
}
