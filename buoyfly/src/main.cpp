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
// 2019 12 15
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
#define SET_CONTAINSROOT
//==========</preset>==========

//============<buoyfly>============
#define BUOYFLY_KEY 110
//============</buoyfly>===========

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
#elif defined(ARDUINO_NodeMCU_32S) // nodemcu-32s
#define LED_PIN 2
#endif
#define LED_PERIOD (1111)
#define LED_ONTIME (1)

//arduino
#include <Arduino.h>

//i2c
#include <Wire.h>
#include "../../post.h"

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

// servo
#define SERVO_PIN D6
#include <Servo.h>
static Servo myservo;

// my tasks
extern Task fastturn_task;
extern Task slowturn_task;
extern Task rest_task;
void fastturn() {
  int r = random(400, 600);
  analogWrite(12,r);
  Serial.print("fast:");
  Serial.println(r);
  slowturn_task.restartDelayed(20000);
}
Task fastturn_task(0, TASK_ONCE, &fastturn);

void slowturn() {
  int r = random(200, 400);
  Serial.print("slow:");
  Serial.println(r);
  analogWrite(12,r);
  rest_task.restartDelayed(15000);
}
Task slowturn_task(0, TASK_ONCE, &slowturn);

void rest() {
  analogWrite(12,0);
  // fastturn_task.restartDelayed(100);
}
Task rest_task(0, TASK_ONCE, &rest);

// mesh callbacks
void receivedCallback(uint32_t from, String & msg) { // REQUIRED
  Serial.print("got msg.: ");
  Serial.println(msg);
  //parse now.

  //parse letter string.

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

  String str_key = msg.substring(1, 4);
  String str_velocity = msg.substring(4, 7);
  String str_gate = msg.substring(7, 8);

  int key = str_key.toInt();
  int velocity = str_velocity.toInt(); // 0 ~ 127
  int gate = str_gate.toInt();

  //is it for me, the gastank?
  if (key == BUOYFLY_KEY && gate == 1) {
    fastturn_task.restartDelayed(10);
  }
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

  //random seed
  randomSeed(analogRead(0));

  //tasks
  runner.addTask(fastturn_task);
  runner.addTask(slowturn_task);
  runner.addTask(rest_task);

  rest_task.restartDelayed(500);
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
