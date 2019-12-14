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
// 2019 12 10
//
// (part-1) esp8266 : 'postman' (the mesh network nodes)
//
//   this module will build up a mesh cloud.
//
//   for now, ESP-MESH is out there.
//   which is probably more complete impl. of this kind.
//   but that's only for esp32, not for esp8266
//   we want to use esp8266, so, we will use painlessMesh
//   which is also good.
//
//   for painlessMesh, a node is a JSON 'postman'
//   we can broadcast/unicast/recv. msg. w/ meshID and nodelist
//   so, let's just use it.
//
//   but one specific thing is that we will use I2C comm. to feed this postman.
//   and I2C is a FIXED-length msg.
//   so, at least, we need to fix this length of the msg.
//   otherwise, we need to do variable-length comm. like uart. to feed/fetch msg. from postman.
//
//   well, okay. but, let's just do.. I2C. and fix a length.
//   maybe, ... 32 bytes?
//   so, then, this postman will read/write I2C channel. always.. 32 bytes.
//   and then, this 32 bytes will be flying in the clouds.
//

//==========<configurations>===========
//
// 'DISABLE_AP'
// --> disabling AP is for teensy audio samplers.
//     they need this to reduce noise from AP beacon signals.
//     but, then they cannot build-up net. by themselves.
//     we need who can do AP..
//     ==> TODO! just prepare some 'dummy' postmans around. w/ AP activated.
//
// 'DISABLE_I2C_REQ'
// --> a quirk.. due to bi-directional I2C hardship.
//     ideally, we want to make this sampler node also speak.
//     but, I2C doesn't work. maybe middleware bug.. we later want to change to diff. proto.
//     for example, UART or so.
//     ==> BEWARE! yet, still we need to take off this.. for 'osc' node.
//
// 'SET_ROOT'
// 'SET_CONTAINSROOT'
// --> for the network stability
//     declare 1 root node and branches(constricted to 'contains the root')
//     to improve the stability of the net
//
//==========</configurations>==========

//==========<preset>===========
// (1) the backbone AP
#if 1
#define DISABLE_I2C_REQ
#define SET_CONTAINSROOT
// (2) osc client (the ROOT)
#elif 0
#define SET_ROOT
#define SET_CONTAINSROOT
// (3) sampler client
#elif 0
#define DISABLE_AP
#define DISABLE_I2C_REQ
// (4) network monitor
#elif 0
#define SET_CONTAINSROOT
//
#endif
//==========</preset>==========

//============<parameters>============
#define MESH_SSID "cricket-crackers"
#define MESH_PASSWORD "cc*vvvv/kkk"
#define MESH_PORT 5555
#define MESH_CHANNEL 5
#define LONELY_TO_DIE    (1000)
//============</parameters>===========

//
// LED status indication
// phase 0
//    - LED => steady on
//    - booted. and running. no connection. scanning.
// phase 1
//    - LED => slow blinking (syncronized)
//    - + connected.
//
#if defined(ARDUINO_ESP8266_NODEMCU) // nodemcuv2
#define LED_PIN 2
#elif defined(ARDUINO_ESP8266_ESP12) // huzzah
#define LED_PIN 2
#elif defined(ARDUINO_FEATHER_ESP32) // featheresp32
#define LED_PIN 13
#endif
#define LED_PERIOD (1111)
#define LED_ONTIME (1)

//arduino
#include <Arduino.h>

//i2c
#include <Wire.h>
#include "../post.h"

//painlessmesh
#include <painlessMesh.h>
painlessMesh mesh;

//scheduler
Scheduler runner;

//task #0 : connection indicator
bool onFlag = false;
bool isConnected = false;
//prototypes
void taskStatusBlink_steadyOn();
void taskStatusBlink_slowblink_insync();
void taskStatusBlink_steadyOff();
//the task
Task statusblinks(0, 1, &taskStatusBlink_steadyOn); // at start, steady on. default == disabled. ==> setup() will enable.
// when disconnected, and trying, steadyon.
void taskStatusBlink_steadyOn() {
  onFlag = true;
}
// when connected, blink per 1s. sync-ed. (== default configuration)
void taskStatusBlink_slowblink_insync() {
  // toggler
  onFlag = !onFlag;
  // on-time
  statusblinks.delay(LED_ONTIME);
  // re-enable & sync.
  if (statusblinks.isLastIteration()) {
    statusblinks.setIterations(2); //refill iteration counts
    statusblinks.enableDelayed(LED_PERIOD - (mesh.getNodeTime() % (LED_PERIOD*1000))/1000); //re-enable with sync-ed delay
  }
}
// when connected, steadyoff. (== alternative configuration)
void taskStatusBlink_steadyOff() {
  onFlag = false;
}

//task #1 : happy or lonely
//   --> automatic reset after some time of 'loneliness (disconnected from any node)'
void nothappyalone() {
  static bool isConnected_prev = false;
  static unsigned long lonely_time_start = 0;
  // oh.. i m lost the signal(==connection)
  if (isConnected_prev != isConnected && isConnected == false) {
    lonely_time_start = millis();
    Serial.println("oh.. i m lost!");
  }
  // .... how long we've been lonely?
  if (isConnected == false) {
    if (millis() - lonely_time_start > LONELY_TO_DIE) {
      // okay. i m fed up. bye the world.
      Serial.println("okay. i m fed up. bye the world.");
      Serial.println();
#if defined(ESP8266)
      ESP.reset();
#elif defined(ESP32)
      ESP.restart();
      // esp32 doesn't support 'reset()' yet...
      // (restart() is framework-supported, reset() is more forced hardware-reset-action)
#else
#error unknown esp.
#endif
    }
  }
  //
  isConnected_prev = isConnected;
}
// Task nothappyalone_task(1000, TASK_FOREVER, &nothappyalone, &runner, true); // by default, ENABLED.
Task nothappyalone_task(100, TASK_FOREVER, &nothappyalone); // by default, ENABLED.

//task #2 : regular post collection
#if !defined(DISABLE_I2C_REQ)
void collect_post() {
  //
  static char letter_outro[POST_BUFF_LEN] = "................................";
  // ask a letter and collect the feedback.
  Wire.requestFrom(I2C_ADDR, POST_LENGTH);
  // error flag
  bool letter_is_good = false;
  // check the first byte
  char first = '.';
  // automatically match start byte.
  while (Wire.available()) {
    first = Wire.read();
    if (first == '[') {
      // client want to give me a letter.
      letter_outro[0] = first;
      // matched!
      letter_is_good = true;
      break;
    } else if (first == ' ') {
      // client says nothing to send.
      Serial.print("."); // nothing to send.
      return;
    }
  }
  //
  if (letter_is_good == false) {
    // no more letters, but no valid char.
    Serial.print("?"); // wrong client.
    return;
  } else if (letter_is_good == true) {
    // get more contents
    for (int i = 1; i < (POST_LENGTH-1); i++) {
      if (Wire.available()) {
        letter_outro[i] = Wire.read();
      } else {
        // hmm.. letter is too short.
        letter_outro[i] = '.'; // fill-out with dots.
        Serial.print("$"); // too $hort msg.
        letter_is_good = false;
      }
    }
    // the last byte
    char last = '.';
    if (Wire.available()) {
      letter_outro[POST_LENGTH-1] = last = Wire.read();
      if (last != ']') {
        // hmm.. last byte is strange
        Serial.print("#"); // last byte error.
        letter_is_good = false;
      }
    } else {
      // hmm.. letter is too short.
      letter_outro[POST_LENGTH-1] = '.'; // fill-out with dots.
      Serial.print("$"); // too $hort msg.
      letter_is_good = false;
    }
    // terminal char.
    letter_outro[POST_LENGTH] = '\0';
  }
  // no good letter, we discard.
  if (letter_is_good == false) {
    return;
  }
  // or, post it.
  if (isConnected == true) {
    mesh.sendBroadcast(String(letter_outro));
    Serial.print("sendBroadcast: ");
    Serial.println(letter_outro);
  } else {
    Serial.print("_"); // disconnected.
  }
}
Task collect_post_task(10, TASK_FOREVER, &collect_post, &runner, true); // by default, ENABLED
//MAYBE... 10ms is too fast? move this to the loop() then?
#endif

// mesh callbacks
void receivedCallback(uint32_t from, String & msg) { // REQUIRED
  Serial.print("got msg.: ");
  Serial.println(msg);
  // truncate any extra. letters.
  msg = msg.substring(0, POST_LENGTH); // (0) ~ (POST_LENGTH-1)
  // send whatever letter we postmans trust other postman.
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(msg.c_str(), POST_LENGTH);
  Wire.endTransmission();
}
void changedConnectionCallback() {
  Serial.println(mesh.getNodeList().size());
  // check status -> modify status LED
  if (mesh.getNodeList().size() > 0) {
    // (still) connected.
    onFlag = false; //reset flag stat.
    statusblinks.set(LED_PERIOD, 2, &taskStatusBlink_slowblink_insync);
    // statusblinks.set(0, 1, &taskStatusBlink_steadyOff);
    statusblinks.enable();
    Serial.println("connected!");
    //
    isConnected = true;
    runner.addTask(nothappyalone_task);
    nothappyalone_task.enable();
  }
  else {
    // disconnected!!
    statusblinks.set(0, 1, &taskStatusBlink_steadyOn);
    statusblinks.enable();
    //
    isConnected = false;
  }
  // let I2C device know
  /////
  Serial.println("hi. client, we ve got a change in the net.");
}
void newConnectionCallback(uint32_t nodeId) {
  Serial.println(mesh.getNodeList().size());
  Serial.println("newConnectionCallback.");
  changedConnectionCallback();
}

void setup() {
  //led
  pinMode(LED_PIN, OUTPUT);

  //mesh
  WiFiMode_t node_type = WIFI_AP_STA;
#if defined(DISABLE_AP)
  system_phy_set_max_tpw(0);
  node_type = WIFI_STA;
#endif
  // mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION);
  mesh.setDebugMsgTypes( ERROR | STARTUP );
  mesh.init(MESH_SSID, MESH_PASSWORD, &runner, MESH_PORT, node_type, MESH_CHANNEL);

  //
  // void init(String ssid, String password, Scheduler *baseScheduler, uint16_t port = 5555, WiFiMode_t connectMode = WIFI_AP_STA, uint8_t channel = 1, uint8_t hidden = 0, uint8_t maxconn = MAX_CONN);
  // void init(String ssid, String password, uint16_t port = 5555, WiFiMode_t connectMode = WIFI_AP_STA, uint8_t channel = 1, uint8_t hidden = 0, uint8_t maxconn = MAX_CONN);
  //

#if defined(SET_ROOT)
  mesh.setRoot(true);
#endif
#if defined(SET_CONTAINSROOT)
  mesh.setContainsRoot(true);
#endif
  //callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  Serial.println(mesh.getNodeList().size());

  //tasks
  runner.addTask(statusblinks);
  statusblinks.enable();

  //serial
  Serial.begin(115200);
  delay(100);
  Serial.println("hi, postman ready.");
#if defined(DISABLE_AP)
  Serial.println("!NOTE!: we are in the WIFI_STA mode!");
#endif

  //understanding what is 'the nodeId' ==> last 4 bytes of 'softAPmacAddress'
  // uint32_t nodeId = tcp::encodeNodeId(MAC);
  Serial.print("nodeId (dec) : ");
  Serial.println(mesh.getNodeId(), DEC);
  Serial.print("nodeId (hex) : ");
  Serial.println(mesh.getNodeId(), HEX);
  uint8_t MAC[] = {0, 0, 0, 0, 0, 0};
  if (WiFi.softAPmacAddress(MAC) == 0) {
    Serial.println("init(): WiFi.softAPmacAddress(MAC) failed.");
  }
  Serial.print("MAC : ");
  Serial.print(MAC[0], HEX); Serial.print(", ");
  Serial.print(MAC[1], HEX); Serial.print(", ");
  Serial.print(MAC[2], HEX); Serial.print(", ");
  Serial.print(MAC[3], HEX); Serial.print(", ");
  Serial.print(MAC[4], HEX); Serial.print(", ");
  Serial.println(MAC[5], HEX);

  // for instance,

  // a huzzah board
  // nodeId (dec) : 3256120530
  // nodeId (hex) : C21474D2
  // MAC : BE, DD, C2, 14, 74, D2

  // a esp8266 board (node mcu)
  // nodeId (dec) : 758581767
  // nodeId (hex) : 2D370A07
  // MAC : B6, E6, 2D, 37, A, 7

  //i2c master
  Wire.begin();
}

void loop() {
  runner.execute();
  mesh.update();
#if defined(ESP32)
  digitalWrite(LED_PIN, onFlag); // value == true is ON.
#else
  digitalWrite(LED_PIN, !onFlag); // value == false is ON. so onFlag == true is ON. (pull-up)
#endif
}
