//
// wirelessly connected cloud (Wireless Mesh Networking)
// MIDI-like
// spacial
// sampler keyboard
//

//
// COSMO40 @ Incheon w/ Factory2
// RTA @ Seoul w/ Post Territory Ujeongguk
//

//
// 2019 12 11
//
// (part-2) teensy35 : 'client:osc' (osc over slip --> mesh post)
//
// especially, MIDI-like.
// collect following OSC msg.
//
// "/note/onoff"
// "/note/velocity"
// "/note/key"
//
// and build a MIDI-like letter post.
// and give it to the postman
//

//
// --[post]--> "sampler"
//
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
//

//arduino
#include <Arduino.h>

//osc
#include <OSCBundle.h>
#include <SLIPEncodedUSBSerial.h>
SLIPEncodedUSBSerial SLIPSerial(Serial);

//i2c
#include <Wire.h>
// --[post]--> "sampler"
#include "../../sampler/post_sampler.h"
void receiveEvent(int howMany) {
  // nothing to expect.. but.. let's just print out..
  // while (Wire.available())
  //   Serial.print(Wire.read());
  // Serial.println();
}
// well. i don't know how fast should i be able to send msg.. to the net..
// first test, and then.. if needed. will come back.
bool new_letter = false;
char letter_outro[POST_BUFF_LEN] = "................................";
void letter_outro_clear() {
  // clear the letter buff
  letter_outro[0] = '[';
  for (int i = 1; i < POST_LENGTH-1; i++) letter_outro[i] = '.';
  letter_outro[POST_LENGTH-1] = ']';
}
void requestEvent() {
  if (new_letter) {
    Wire.write(letter_outro, POST_LENGTH);
    new_letter = false;
  } else {
    Wire.write(" "); // no letter to send
  }
}
void I2CPrint(const String & str); // for DEBUG w/ I2C

//
void setup() {
  //i2c
  Wire.begin(I2C_ADDR);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);

  //osc
  SLIPSerial.begin(57600);
}

//
void midinote(OSCMessage& msg, int offset) {
  // matches will happen in the order. that the bundle is packed.
  static int pitch = 0;
  static int velocity = 0;
  static int onoff = 0;
  // (1) --> /onoff
  if (msg.fullMatch("/onoff", offset)) {
    //
    pitch = 0;
    velocity = 0;
    onoff = 0;
    //
    onoff = msg.getInt(0);
    if (onoff != 0) onoff = 1;
  }
  // (2) --> /velocity
  if (msg.fullMatch("/velocity", offset)) {
    velocity = msg.getInt(0);
    if (velocity < 0) velocity = 0;
    if (velocity > 127) velocity = 127;
  }
  // (3) --> /pitch
  if (msg.fullMatch("/pitch", offset)) {
    pitch = msg.getInt(0);
    if (pitch < 0) pitch = 0;
    if (pitch > 127) pitch = 127;
    //
    // while (new_letter != false) {}; // <-- sort of semaphore.. but it doesn't work yet.. buggy.
    sprintf(letter_outro, "[%03d%03d%01d.......................]", pitch, velocity, onoff);
    new_letter = true;
  }
}

//
void loop() {
  static String str = "";
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
  // else {
  //   str = "error! : " + String(bundleIN.getError());
  //   I2CPrint(str);
  // }
}

//==========<I2CPrint>==========
// utility func. -> for DEBUG w/ I2C
char buff[POST_LENGTH] = "";
void I2CPrint(const String & str) {
  const char * buff = str.c_str();
  int len = str.length();
  // while (new_letter != false) {};
  letter_outro[0] = '[';
  if (len > (POST_LENGTH - 2)) len = (POST_LENGTH - 2);
  for (int i = 0; i < len; i++) {
    letter_outro[i+1] = buff[i];
  }
  for (int j = len; j < (POST_LENGTH - 2); j++) {
    letter_outro[j+1] = '_';
  }
  letter_outro[POST_LENGTH-1] = ']';
  new_letter = true;
}
//==========</I2CPrint>=========
