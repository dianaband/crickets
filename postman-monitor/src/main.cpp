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
// 2019 12 13
//

//==========<configuration>===========
// #define DISABLE_AP
// --> disabling AP is for teensy audio samplers.
//     they need this to reduce noise from AP beacon signals.
//     but, then they cannot build-up net. by themselves.
//     we need who can do AP..
//     ==> TODO! just prepare some 'dummy' postmans around. w/ AP activated.
// #define SET_ROOT
#define SET_CONTAINSROOT
// --> for the network stability
//     declare 1 root node and branches(constricted to 'contains the root')
//     to improve the stability of the net
//==========</configuration>==========

//============<monitor-parameters>============
#define NODE_DISP_COL 10
#define NODE_DISP_ROW 6
// NODECOUNT_MAX = NODE_DISP_COL * NODE_DISP_ROW
#define NODECOUNT_MAX 60
//============</monitor-parameters>===========

//============<registered-nodelist>============
#include <map>
#define NTYPE_OSC_ROOT   0xEEEEEEE1
#define NTYPE_POSTMAN    0xEEEEEEE2
#define NTYPE_MONITOR    0xEEEEEEE3
#define NTYPE_SAMPLER    0xEEEEEEE4
#define NTYPE_GASTANK    0xEEEEEEE5
#define NTYPE_BUOY_FLY   0xEEEEEEE6
std::map<uint32_t, uint32_t> registered;
void register_nodes(){
  registered[0x2D9EC96E] = NTYPE_MONITOR; // this one.
  registered[0x2D37150B] = NTYPE_MONITOR; // sub-monitor (old one)

  registered[0x2D373B90] = NTYPE_OSC_ROOT;

  registered[0xABB3B758] = NTYPE_POSTMAN;
  registered[0xC21474D2] = NTYPE_POSTMAN;
  registered[0xABB3B4B8] = NTYPE_POSTMAN;
  registered[0x2D370A07] = NTYPE_POSTMAN;
  registered[0xBFFBDD7D] = NTYPE_POSTMAN; //esp32 postman
  registered[0xABB3B9B4] = NTYPE_POSTMAN;
  registered[0x2D3718AE] = NTYPE_POSTMAN;
  registered[0xABB3BA44] = NTYPE_POSTMAN;
  registered[0xBFFD7FF1] = NTYPE_POSTMAN;
  registered[0x3A58872D] = NTYPE_POSTMAN;

  registered[0x2D3745F5] = NTYPE_SAMPLER; //1
  registered[0x2218D91E] = NTYPE_SAMPLER; //2
  registered[0xBC631984] = NTYPE_SAMPLER; //3
  registered[0x22199605] = NTYPE_SAMPLER; //4
  registered[0x3AD53EF3] = NTYPE_SAMPLER; //5
  registered[0x3AD74D97] = NTYPE_SAMPLER; //6
  registered[0xABB3B419] = NTYPE_SAMPLER; //7
  registered[0xABB3B4DD] = NTYPE_SAMPLER; //8
  registered[0x2219A51C] = NTYPE_SAMPLER; //9
  registered[0xC2B2AFD4] = NTYPE_SAMPLER; //a
  registered[0x3AAB562D] = NTYPE_SAMPLER; //b
  registered[0xABB3B5C2] = NTYPE_SAMPLER; //c

  registered[0x2219ACD1] = NTYPE_SAMPLER; //A round (old)
  registered[0x2219A4A7] = NTYPE_SAMPLER; //B round (old)
  registered[0x22199CAD] = NTYPE_SAMPLER; //C round (old)
  registered[0x2218DEE3] = NTYPE_SAMPLER; //D round (old)
  registered[0x2218DFD5] = NTYPE_SAMPLER; //E round (old)

  registered[0x2D371730] = NTYPE_GASTANK;
  registered[0x3A588030] = NTYPE_BUOY_FLY;
}
//============</registered-nodelist>===========

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

//painlessmesh
#include <painlessMesh.h>
painlessMesh mesh;

// using Adafruit 3.5" TFT (HX8357) FeatherWing
//     ----> http://www.adafruit.com/products/3651
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_HX8357.h"
#include "Adafruit_STMPE610.h"

#if defined(ESP32)
#define STMPE_CS 32
#define TFT_CS   15
#define TFT_DC   33
#define SD_CS    14
#endif

#define TFT_RST -1

// Init screen on hardware SPI, HX8357D type:
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 3800
#define TS_MAXX 100
#define TS_MINY 100
#define TS_MAXY 3750

//scheduler
Scheduler runner;

//screen task
const int ww = 30;
const int hh = 20;
void loop_screen() {
  static bool first = true;
  int idx = 0;
  int cnt_postman = 0;
  int cnt_samplers = 0;
  //the nodelist
  std::list<uint32_t> nodelist = mesh.getNodeList();
  auto it_nodelist = nodelist.begin();
  //
  for (int row = 0; row < NODE_DISP_ROW; row++) {
    for (int col = 0; col < NODE_DISP_COL; col++) {
      // place markings
      int posx = col*(ww + 4) + 50;
      int posy = row*(hh + 4) + 50;
      // static drawings
      if (first) {
        // tft.drawRect(posx, posy, ww, hh, HX8357_BLUE);
        // label
        tft.setCursor(posx + 16, posy + 6);
        tft.setTextColor(HX8357_BLUE);
        tft.setTextSize(1);
        tft.print(idx + 1);
      }
      // dynamic drawings
      int box = 3;
      int boxx = posx + 8;
      int boxy = posy + 5;
      // at first, clear the region.
      tft.fillCircle(boxx, boxy, box, HX8357_BLACK);
      // indicator color for 'unknown'
      int indicator_color = HX8357_BLUE;
      if (idx == 0) {
        // since, me-myself is not in the list. i m the first.
        // me-myself as a 'monitoring node'
        tft.fillCircle(boxx, boxy, box, HX8357_MAGENTA);
      } else {
        // search over
        if (it_nodelist != nodelist.end()) {
          auto found = registered.find(*it_nodelist);
          if (found != registered.end()) {
            //found
            switch (found->second) {
            case NTYPE_MONITOR:
              indicator_color = HX8357_MAGENTA;
              break;
            case NTYPE_OSC_ROOT:
              indicator_color = HX8357_RED;
              break;
            case NTYPE_POSTMAN:
              indicator_color = HX8357_BLUE;
              cnt_postman++;
              break;
            case NTYPE_SAMPLER:
              indicator_color = HX8357_CYAN;
              cnt_samplers++;
              break;
            case NTYPE_GASTANK:
              indicator_color = HX8357_GREEN;
              break;
            case NTYPE_BUOY_FLY:
              indicator_color = HX8357_WHITE;
              break;
            default:
              // std::map error.. this must not happen.
              Serial.println("std::map error.. this must not happen.");
            }
            //
            tft.fillCircle(boxx, boxy, box, indicator_color);
          } else {
            // sth. not registered.
            tft.drawCircle(boxx, boxy, box, HX8357_BLUE);
            Serial.print("ufo. - 0x");
            Serial.println(*it_nodelist, HEX);
          }
          //
          it_nodelist++;
        }
      }
      //
      idx++;
    }
  }
  //
  tft.fillRect(50, 210, 60, 30, HX8357_BLACK);
  tft.setCursor(50, 210);
  tft.setTextColor(HX8357_BLUE);
  tft.setTextSize(3);
  tft.print(cnt_postman);
  //
  tft.fillRect(250, 210, 60, 30, HX8357_BLACK);
  tft.setCursor(250, 210);
  tft.setTextColor(HX8357_CYAN);
  tft.setTextSize(3);
  tft.print(cnt_samplers);
  //
  first = false;
  //
  Serial.println();
}
Task loop_screen_task(1000, TASK_FOREVER, &loop_screen, &runner, false); // fps : 1hz

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

// mesh callbacks
void receivedCallback(uint32_t from, String & msg) { // REQUIRED
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

  //tft screen
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(HX8357_BLACK);

  // registering nodes
  register_nodes();

  //
  runner.addTask(loop_screen_task);
  loop_screen_task.enable();
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
