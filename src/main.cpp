// ArduinoJson - arduinojson.org
// Copyright Benoit Blanchon 2014-2019
// MIT License
//
// This example shows how to generate a JSON document with ArduinoJson.
//
// https://arduinojson.org/v6/example/generator/

#include <Arduino.h>
#include <ArduinoJson.h>

void setup() {
  // Initialize Serial port
  Serial.begin(9600);
  // while (!Serial) continue;

}

void loop() {
  // not used in this example
  StaticJsonDocument<200> doc;
  doc["sensor"] = "gps";
  doc["time"] = 1351824120;
  JsonArray data = doc.createNestedArray("data");
  data.add(48.756080);
  data.add(2.302038);
  serializeJson(doc, Serial);
  Serial.println();
  // serializeJsonPretty(doc, Serial);

  delay(1000);
}

// See also
// --------
//
// https://arduinojson.org/ contains the documentation for all the functions
// used above. It also includes an FAQ that will help you solve any
// serialization problem.
//
// The book "Mastering ArduinoJson" contains a tutorial on serialization.
// It begins with a simple example, like the one above, and then adds more
// features like serializing directly to a file or an HTTP request.
// Learn more at https://arduinojson.org/book/
// Use the coupon code TWENTY for a 20% discount ❤❤❤❤❤

// //************************************************************
// // this is a simple example that uses the easyMesh library
// //
// // 1. blinks led once for every node on the mesh
// // 2. blink cycle repeats every BLINK_PERIOD
// // 3. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// // 4. prints anything it receives to Serial.print
// //
// //
// //************************************************************
// #include <Arduino.h>
// #include <painlessMesh.h>
//
// #include <OSCBundle.h>
// #include <OSCBoards.h>
// #include "StringStream.h"
//
// // some gpio pin that is connected to an LED...
// // on my rig, this is 5, change to the right number of your LED.
// #define   LED             2       // GPIO number of connected LED, ON ESP-12 IS GPIO2
//
// #define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
// #define   BLINK_DURATION  100  // milliseconds LED is on for
//
// #define   MESH_SSID       "whateverYouLike"
// #define   MESH_PASSWORD   "somethingSneaky"
// #define   MESH_PORT       5555
//
// // Prototypes
// void sendMessage();
// void receivedCallback(uint32_t from, String & msg);
// void newConnectionCallback(uint32_t nodeId);
// void changedConnectionCallback();
// void nodeTimeAdjustedCallback(int32_t offset);
// void delayReceivedCallback(uint32_t from, int32_t delay);
//
// Scheduler userScheduler;     // to control your personal task
// painlessMesh mesh;
//
// bool calc_delay = false;
// SimpleList<uint32_t> nodes;
//
// void sendMessage();  // Prototype
// Task taskSendMessage( TASK_SECOND * 1, TASK_FOREVER, &sendMessage ); // start with a one second interval
//
// // Task to blink the number of nodes
// Task blinkNoNodes;
// bool onFlag = false;
//
// void setup() {
//   Serial.begin(115200);
//
//   pinMode(LED, OUTPUT);
//
//   mesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages
//   // mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE);  // set before init() so that you can see error messages
//
//   mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
//   mesh.onReceive(&receivedCallback);
//   mesh.onNewConnection(&newConnectionCallback);
//   mesh.onChangedConnections(&changedConnectionCallback);
//   mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
//   mesh.onNodeDelayReceived(&delayReceivedCallback);
//
//   userScheduler.addTask( taskSendMessage );
//   taskSendMessage.enable();
//
//   blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []() {
//     // If on, switch off, else switch on
//     if (onFlag)
//       onFlag = false;
//     else
//       onFlag = true;
//     blinkNoNodes.delay(BLINK_DURATION);
//
//     if (blinkNoNodes.isLastIteration()) {
//       // Finished blinking. Reset task for next run
//       // blink number of nodes (including this node) times
//       blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
//       // Calculate delay based on current mesh time and BLINK_PERIOD
//       // This results in blinks between nodes being synced
//       blinkNoNodes.enableDelayed(BLINK_PERIOD -
//                                  (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
//     }
//   });
//   userScheduler.addTask(blinkNoNodes);
//   blinkNoNodes.enable();
//
//   randomSeed(analogRead(A0));
// }
//
// void loop() {
//   mesh.update();
//   digitalWrite(LED, !onFlag);
// }
//
// OSCErrorCode error;
// void oscRecvNodeId(OSCMessage &msg) {
//   Serial.print("/nodeid --> ");
//   Serial.println(msg.getInt(0));
// }
//
// void oscRecvMyFreeMemory(OSCMessage &msg) {
//   Serial.print("/myFreeMemory --> ");
//   Serial.println(msg.getInt(0));
// }
//
//
// void sendMessage() {
//   //
//   OSCBundle bndl;
//   String oscmsg;
//   StringStream sstream(oscmsg);
//   bndl.add("/nodeid").add(mesh.getNodeId());
//   bndl.add("/myFreeMemory").add(ESP.getFreeHeap());
//   bndl.send(sstream);
//   //
//   mesh.sendBroadcast(oscmsg);
//
//   //
//   int size = sstream.available();
//   //
//   OSCBundle bndl2;
//   if (size > 0) {
//     while (size--) {
//       bndl2.fill(sstream.read());
//     }
//     if (!bndl2.hasError()) {
//       Serial.println("bndl2.dispatch...");
//       bndl2.dispatch("/nodeid", oscRecvNodeId);
//       bndl2.dispatch("/myFreeMemory", oscRecvMyFreeMemory);
//     } else {
//       error = bndl2.getError();
//       Serial.print("error: ");
//       Serial.println(error);
//     }
//   }
//
//
//   if (calc_delay) {
//     SimpleList<uint32_t>::iterator node = nodes.begin();
//     while (node != nodes.end()) {
//       mesh.startDelayMeas(*node);
//       node++;
//     }
//     calc_delay = false;
//   }
//
//   Serial.print("Sending message: ");
//   Serial.println(oscmsg);
//
//   taskSendMessage.setInterval( random(TASK_SECOND * 1, TASK_SECOND * 5));  // between 1 and 5 seconds
// }
//
// void receivedCallback(uint32_t from, String & msg) {
//   Serial.printf("startHere: Received from %u msg=", from);
//   Serial.println(msg);
//   //
//   OSCBundle bndl;
//   String oscmsg;
//   StringStream sstream(oscmsg);
//   oscmsg = msg;
//   int size = sstream.available();
//   //
//   if (size > 0) {
//     while (size--) {
//       bndl.fill(sstream.read());
//     }
//     if (!bndl.hasError()) {
//       Serial.println("bndl.dispatch...");
//       bndl.dispatch("/nodeid", oscRecvNodeId);
//       bndl.dispatch("/myFreeMemory", oscRecvMyFreeMemory);
//     } else {
//       error = bndl.getError();
//       Serial.print("error: ");
//       Serial.println(error);
//     }
//   }
// }
//
// void newConnectionCallback(uint32_t nodeId) {
//   // Reset blink task
//   onFlag = false;
//   blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
//   blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
//
//   Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
//   Serial.printf("--> startHere: New Connection, %s\n", mesh.subConnectionJson(true).c_str());
// }
//
// void changedConnectionCallback() {
//   Serial.printf("Changed connections\n");
//   // Reset blink task
//   onFlag = false;
//   blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
//   blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
//
//   nodes = mesh.getNodeList();
//
//   Serial.printf("Num nodes: %d\n", nodes.size());
//   Serial.printf("Connection list:");
//
//   SimpleList<uint32_t>::iterator node = nodes.begin();
//   while (node != nodes.end()) {
//     Serial.printf(" %u", *node);
//     node++;
//   }
//   Serial.println();
//   calc_delay = true;
// }
//
// void nodeTimeAdjustedCallback(int32_t offset) {
//   Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
// }
//
// void delayReceivedCallback(uint32_t from, int32_t delay) {
//   Serial.printf("Delay to node %u is %d us\n", from, delay);
// }
//
// // #include <Arduino.h>
// //
// // #include <OSCBundle.h>
// // #include <OSCBoards.h>
// //
// // #include "StringStream.h"
// //
// // void setup() {
// //   Serial.begin(9600);
// // }
// //
// // void loop(){
// //   OSCBundle bndl;
// //   int size;
// //   //receive a bundle
// //
// //   while(!SLIPSerial.endofPacket())
// //     if( (size = SLIPSerial.available()) > 0)
// //     {
// //       while(size--)
// //         bndl.fill(SLIPSerial.read());
// //     }
// //
// //   // if(!bndl.hasError())
// //   // {
// //   static int32_t sequencenumber=0;
// //   // we can sneak an addition onto the end of the bundle
// //   // bndl.add("/micros").add((int32_t)micros());       // (int32_t) is the type of OSC Integers
// //   bndl.add("/sequencenumber").add(sequencenumber++);
// //   bndl.add("/digital/5").add(digitalRead(5)==HIGH);
// //   // bndl.add("/lsb").add((sequencenumber &1)==1);
// //   String sstr;
// //   StringStream ss(sstr);
// //   bndl.send(ss);
// //   Serial.println(sstr);
// //   // }
// //
// //   delay(1000);
// // }
