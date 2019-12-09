//
// Exhibition @ exhibition-space
//   <one and twelve one-hundred-eighth seconds at the prince's room>
//
// Feb. 11 @ 2019
//

// the common sense
#include "common.h"

#if (IDENTITY == ID_SPEAK_A)
#include "../members/speaker_a.cpp"
//
#endif

// painless mesh
painlessMesh mesh;

// firmata
#if (FIRMATA_USE == FIRMATA_ON)
#include <Firmata.h>
#endif

//scheduler
Scheduler runner;

//task #0 : connection indicator
bool onFlag = false;
bool isConnected = false;
//prototypes
void taskStatusBlink_steadyOn();
void taskStatusBlink_slowblink_insync();
void taskStatusBlink_fastblink();
void taskStatusBlink_steadyOff();
//the task
Task statusblinks(0, 1, &taskStatusBlink_steadyOn); // at start, steady on. default == disabled. ==> setup() will enable.
// when disconnected, steadyon.
void taskStatusBlink_steadyOn() {
  onFlag = true;
}
// blink per 1s. sync-ed.
void taskStatusBlink_slowblink_insync() {
  // toggler
  if (onFlag) {
    onFlag = false;
  }
  else {
    onFlag = true;
  }
  // on-time
  statusblinks.delay(LED_ONTIME);
  // re-enable & sync.
  if (statusblinks.isLastIteration()) {
    statusblinks.setIterations(2); //refill iteration counts
    statusblinks.enableDelayed(LED_PERIOD - (mesh.getNodeTime() % (LED_PERIOD*1000))/1000); //re-enable with sync-ed delay
  }
}
void taskStatusBlink_fastblink() {
}
// when connected, steadyoff. (?)
void taskStatusBlink_steadyOff() {
  onFlag = false;
}

// happy or lonely
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
#if (BOARD_SELECT == BOARD_NODEMCU_ESP12E)
      ESP.reset();
#elif (BOARD_SELECT == BOARD_NODEMCU_ESP32)
      ESP.restart(); // esp32 doesn't support 'reset()' yet... (restart() is framework-supported, reset() is more forced hardware-reset-action)
#endif
    }
  }
  //
  isConnected_prev = isConnected;
}
Task nothappyalone_task(1000, TASK_FOREVER, &nothappyalone, &runner, true); // by default, ENABLED.

// mesh callbacks
void receivedCallback(uint32_t from, String & msg) { // REQUIRED
  // let the member device know.
  gotMessageCallback(from, msg);
}
void changedConnectionCallback() {
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
  }
  else {
    // disconnected!!
    statusblinks.set(0, 1, &taskStatusBlink_steadyOn);
    statusblinks.enable();
    //
    isConnected = false;
  }
  // let the member device know.
  gotChangedConnectionCallback();
}
void newConnectionCallback(uint32_t nodeId) {
  changedConnectionCallback();
}

void setup_member();
void setup() {
  //led
  pinMode(LED_PIN, OUTPUT);

  //mesh
  WiFiMode_t node_type = WIFI_AP_STA;
#if (NODE_TYPE == NODE_TYPE_STA_ONLY)
  system_phy_set_max_tpw(0);
  node_type = WIFI_STA;
#endif
#if (FIRMATA_USE == FIRMATA_ON)
  mesh.setDebugMsgTypes( ERROR );
#elif (FIRMATA_USE == FIRMATA_OFF)
  // mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION);
  mesh.setDebugMsgTypes( ERROR | STARTUP );
#endif
  mesh.init(MESH_SSID, MESH_PASSWORD, &runner, MESH_PORT, node_type, MESH_CHANNEL);

  //
  // void init(String ssid, String password, Scheduler *baseScheduler, uint16_t port = 5555, WiFiMode_t connectMode = WIFI_AP_STA, uint8_t channel = 1, uint8_t hidden = 0, uint8_t maxconn = MAX_CONN);
  // void init(String ssid, String password, uint16_t port = 5555, WiFiMode_t connectMode = WIFI_AP_STA, uint8_t channel = 1, uint8_t hidden = 0, uint8_t maxconn = MAX_CONN);
  //

#ifdef MESH_ANCHOR
  mesh.setContainsRoot(true);
#endif
  //callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

  //tasks
  runner.addTask(statusblinks);
  statusblinks.enable();

#if (FIRMATA_USE == FIRMATA_ON)
  Firmata.setFirmwareVersion(0,1);
  Firmata.attach(ANALOG_MESSAGE, analogWriteCallback);
  Firmata.begin(57600);
#elif (FIRMATA_USE == FIRMATA_OFF)
  //serial
  Serial.begin(9600);
  delay(100);
  Serial.println("setup done.");
  Serial.print("IDENTITY: ");
  Serial.println(IDENTITY);
#if (NODE_TYPE == NODE_TYPE_STA_ONLY)
  Serial.println("INFO: we are in the WIFI_STA mode!");
#endif
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

  //setup_member
  setup_member();
}

void loop() {
  runner.execute();
  mesh.update();
#if (BOARD_SELECT == BOARD_NODEMCU_ESP32)
  digitalWrite(LED_PIN, onFlag); // value == true is ON.
#else
  digitalWrite(LED_PIN, !onFlag); // value == false is ON. so onFlag == true is ON. (pull-up)
#endif

#if (FIRMATA_USE == FIRMATA_ON)
  while (Firmata.available()) {
    Firmata.processInput();
  }
#endif
}
