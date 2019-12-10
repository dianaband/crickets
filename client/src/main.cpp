#include <Wire.h>
const int16_t I2C_ADDR = 3;

void receiveEvent(int howMany) {
  (void) howMany;
  while (1 < Wire.available()) {
    char c = Wire.read();
    Serial.print(c);
  }
  char x = Wire.read();
  Serial.println(x);
}

void requestEvent() {
  if (random(1000) == 0) {
    Wire.write("[bcdefghabcdefghabcdefghabcdefg]"); //32 bytes
  } else {
    Wire.write(" "); // no letter to send
  }
}

void setup() {
  Wire.begin(I2C_ADDR);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
}

void loop() {
}
