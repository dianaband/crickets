// i2c
#include <Wire.h>
#include "speakers/i2c_protocol.h"

// tasks
extern Task sing_task;
extern Task indep_random_task;
extern Task direc_propa_task;

// soundset
int soundset = 1; // starting # of the set. (10 files will be selected.)

// playmode
int playmode = SPEAKERS_PLAYMODE_INDEP;

// vspeed
int vspeed = 0;

// room protocol
static char msg_cstr[MSG_LENGTH_MAX] = {0, };
void gotChangedConnectionCallback() { // REQUIRED
}
void gotMessageCallback(uint32_t from, String & msg) { // REQUIRED
  Serial.println(msg);
  int message = msg.substring(1, 6).toInt();
  // this speaker event
  if (playmode == SPEAKERS_PLAYMODE_PROPA) {
    if (message == SPEAK_A_TIC) {
      Serial.println("SPEAK_A_TIC");
      int r = 100 + random(vspeed * 10);
      sing_task.restartDelayed(r);
      direc_propa_task.restartDelayed(r);
    }
  }
  // speakers group : SPEAKERS_PLAYMODE
  if (message == SPEAKERS_PLAYMODE) {
    int para = msg.substring(7, 9).toInt(); // get +XX parameter..
    // only allow valid inputs
    if (para == SPEAKERS_PLAYMODE_INDEP) {
      playmode = para;
      indep_random_task.restartDelayed(100);
    }
    else if (para == SPEAKERS_PLAYMODE_PROPA) {
      playmode = para;
      indep_random_task.disable();
    }
  }
  // speakers group : SPEAKERS_PARA_SNDSET
  if (message == SPEAKERS_PARA_SNDSET) {
    int para = msg.substring(7, 9).toInt(); // get +XX parameter..
    // only allow valid inputs
    if (para >= 1 && para < 100) { // 1 ~ 99
      soundset = para;
    }
  }
  // speakers group : SPEAKERS_PARA_SPEED
  if (message == SPEAKERS_PARA_SPEED) {
    int para = msg.substring(7, 10).toInt(); // get +XXX parameter..
    // only allow valid inputs
    if (para >= 0 && para < 1000) { // 0 ~ 999
      vspeed = para;
    }
  }
}

// saying hello
void greeting() {
  static String msg = "";
  sprintf(msg_cstr, "[%05d]", SPEAK_A_HELLO);
  msg = String(msg_cstr);
  mesh.sendBroadcast(msg);
  Serial.println("TX : " + msg);
}
Task saying_greeting(10000, TASK_FOREVER, &greeting);

//playmode #1 : independent random playmode
void indep_random() {
  int base_speed = 1000 + (vspeed * 10);
  int random_portion = base_speed * 0.1;
  //
  sing_task.restartDelayed(100);
  indep_random_task.restartDelayed(base_speed + random(random_portion));  //re-schedule myself.
}
Task indep_random_task(0, TASK_ONCE, &indep_random);

//playmode #2 : directional propagation playmode
void direc_propa() {
  static String msg = "";
  sprintf(msg_cstr, "[%05d]", SPEAK_B_TIC);
  msg = String(msg_cstr);
  mesh.sendBroadcast(msg);
  Serial.println("TX : SPEAK_B_TIC");
}
Task direc_propa_task(0, TASK_ONCE, &direc_propa);

// sing!
void sing() {
  //
  static int song_select = 1;
  //
  song_select = random(soundset, (soundset + 10)); // every sound set has 10 sounds. x ~ x+9
  // "P#SSS@AAAA" - P: P (play), SSS: song #, A: amp. (x 1000)
  // "SXXXXXXXXX" - S: S (stop)
  sprintf(cmdstr, "P#%03d@%04d", song_select, 800); // play song #1, with amplitude
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(cmdstr, CMD_LENGTH);
  Wire.endTransmission();
}
Task sing_task(0, TASK_ONCE, &sing);

//setup_member
void setup_member() {
  //i2c master
  Wire.begin();

  //tasks
  runner.addTask(saying_greeting);
  saying_greeting.enable();
  //
  runner.addTask(sing_task);
  //
  runner.addTask(indep_random_task);
  runner.addTask(direc_propa_task);
  //
  indep_random_task.restartDelayed(100);
}
