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
// (part-3) teensy35 : 'client:sampler' (mesh post --> play sounds)
//

//
// 2019 12 29
//
// multiple sound playback -> 4 voices -- TESTING
//

//----------<configuration>----------
//
// 'ANALOG_REF_EXTERNAL_3P3V'
// --> this will output 3.3v-pp from dacs pins of the board.
//     outdoor or big space. you will need this.
//     but, teensy36 board dacs pins once might have burned off cause of this?
//     teensy35 was okay since they are stronger (5V compatible I/O)
//
// #define ANALOG_REF_EXTERNAL_3P3V
//
// 'LED_INDICATOR'
// --> this will enable red LED on/off according to the file playback status.
//
#define LED_INDICATOR
//----------</configuration>----------

//watchdog
#include <Adafruit_SleepyDog.h>

//teensy audio
#include <Audio.h>
#include <SdFat.h>
SdFatSdioEX SD;
#include <SerialFlash.h>

//teensy 3.5 with SD card
#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used

// GUItool: begin automatically generated code
AudioPlaySdWav playSdWav1;               //xy=183,90
AudioPlaySdWav playSdWav2;               //xy=185,253
AudioPlaySdWav playSdWav3;           //xy=187,411
AudioPlaySdWav playSdWav4;           //xy=188,579
AudioAmplifier amp1;                     //xy=374,49
AudioAmplifier amp2;                     //xy=375,132
AudioAmplifier amp3;           //xy=377,219
AudioAmplifier amp4;           //xy=378,302
AudioAmplifier amp5;           //xy=378,370
AudioAmplifier amp6;           //xy=379,453
AudioAmplifier amp7;           //xy=381,540
AudioAmplifier amp8;           //xy=382,623
AudioMixer4 mixer2;                      //xy=620,413
AudioMixer4 mixer1;                      //xy=621,210
AudioOutputAnalogStereo dacs1;           //xy=812,318
AudioConnection patchCord1(playSdWav1, 0, amp1, 0);
AudioConnection patchCord2(playSdWav1, 1, amp2, 0);
AudioConnection patchCord3(playSdWav2, 0, amp3, 0);
AudioConnection patchCord4(playSdWav2, 1, amp4, 0);
AudioConnection patchCord5(playSdWav3, 0, amp5, 0);
AudioConnection patchCord6(playSdWav3, 1, amp6, 0);
AudioConnection patchCord7(playSdWav4, 0, amp7, 0);
AudioConnection patchCord8(playSdWav4, 1, amp8, 0);
AudioConnection patchCord9(amp1, 0, mixer1, 0);
AudioConnection patchCord10(amp2, 0, mixer2, 0);
AudioConnection patchCord11(amp3, 0, mixer1, 1);
AudioConnection patchCord12(amp4, 0, mixer2, 1);
AudioConnection patchCord13(amp5, 0, mixer1, 2);
AudioConnection patchCord14(amp6, 0, mixer2, 2);
AudioConnection patchCord15(amp7, 0, mixer1, 3);
AudioConnection patchCord16(amp8, 0, mixer2, 3);
AudioConnection patchCord17(mixer2, 0, dacs1, 1);
AudioConnection patchCord18(mixer1, 0, dacs1, 0);
// GUItool: end automatically generated code

//
class Voice {
  //private

  //teensy audio
  AudioPlaySdWav& player;
  AudioAmplifier& ampL;
  AudioAmplifier& ampR;

  // a filename buffer
  char filename[13];

public:

  //
  int note_now;
  int velocity_now;

  //
  Voice(AudioPlaySdWav& player_, AudioAmplifier& ampL_, AudioAmplifier& ampR_)
    : player(player_)
    , ampL(ampL_)
    , ampR(ampR_)
  {
    //initializations
    note_now = 0;
    velocity_now = 0;
    strcpy(filename, "NNN.WAV");
  }

  //
  void noteOn(int note) {
    // present my 'note' -> 'occupied'.
    note_now = note;
    // set filename to play...
    int nnn = (note % 1000);  // 0~999
    int nn =  (note % 100);   // 0~99
    filename[0] = '0' + (nnn / 100); // N__.WAV
    filename[1] = '0' + (nn / 10);   // _N_.WAV
    filename[2] = '0' + (nn % 10);   // __N.WAV
    // the filename to play is...
    Serial.println(filename);
    // go! (re-triggering)
    // if (player.isPlaying()) player.stop();
    player.play(filename);
    // --> we just believe that this 'file' is existing & available. NO additional checking.
    delay(10);
    // --> let's wait a bit before exit, to give more room to work for background workers(==filesystem|audio-interrupts)
    // --> if we get too fast 'player.play' twice, then the system might get broken/stalled. ?
  }
  //
  void noteOff() {
    player.stop();
    delay(10); // wait to close file?
    // present my 'note' -> 'free'.
    note_now = 0;
  }
  //
  void setVelocity(int val) {
    if (val < 0) val = 0;
    float vv = (float)val / 127; // allowing +gain for values over 127.
    ampL.gain(vv);
    ampR.gain(vv);
  }
  //
  bool isPlaying() {
    return player.isPlaying();
  }
};

// voice banks
#include <vector>
#include <deque>
static Voice __voice_1(playSdWav1, amp1, amp2);
static Voice __voice_2(playSdWav2, amp3, amp4);
static Voice __voice_3(playSdWav3, amp5, amp6);
static Voice __voice_4(playSdWav4, amp7, amp8);
static std::vector<Voice> poly_bank;
static std::deque< std::pair<int, int> > poly_queue;

//task
#include <TaskScheduler.h>
Scheduler runner;
// polyphonics
static int note_sched = 0;
static int velocity_sched = 0;
void scheduleNoteOn()
{
  //filename buffer - 8.3 naming convension! 8+1+3+1 = 13
  char fname[13] = "NNN.WAV";
  //search for the sound file
  int note = note_sched;
  int nnn = (note % 1000);  // 0~999
  int nn =  (note % 100);   // 0~99
  fname[0] = '0' + (nnn / 100); // N__.WAV
  fname[1] = '0' + (nn / 10);   // _N_.WAV
  fname[2] = '0' + (nn % 10);   // __N.WAV
  //TEST
  Serial.println(fname);
  AudioNoInterrupts();
  bool test = SD.exists(fname);
  AudioInterrupts();
  if (!test) {
    Serial.println("... does not exist.");
    return;
  }
  //ok, let's schedule a voice
  //btw, is it already playing?
  // --> entry : (# of voice bank, playing note #)
  bool is_already = false;
  for (uint32_t idx = 0; idx < poly_queue.size(); idx++) {
    if (poly_queue[idx].second == note) {
      //oh, it is alreay playing
      // --> what to do?
      // (1) re-trigger (stop-and-restart)
      // is_already = true;
      // Voice& v = poly_bank[poly_queue[idx].first];
      // v.noteOff();
      // v.noteOn(note);
      // v.setVelocity(velocity_sched);
      // break;
      // (2) do nothing (just let it play till end)
      // is_already = true;
      // break;
      // (3) trigger a new one? (schedule a new one overlapping)
      // --> then you just ignore the fact.
      //     i.e. do not tick 'is_already = true',
      //     then a new one will be automatically assigned.
      break;
    }
  }
  //it's sth. new..
  if (is_already == false) {
    //fine, is there idle voice?
    bool is_found_idle = false;
    for (uint32_t idx = 0; idx < poly_bank.size(); idx++) {
      if (poly_bank[idx].note_now == 0) {
        //cool, got one.
        is_found_idle = true;
        //play start-up
        Voice& v = poly_bank[idx];
        v.noteOn(note);
        v.setVelocity(velocity_sched);
        //leave a record : (# of voice bank, playing note #)
        poly_queue.push_back(std::pair<int, int>(idx, note));
        break;
      }
    }
    //oh, no idle one!
    if (is_found_idle == false) {
      //then, who's the oldest?
      int oldest = poly_queue.front().first;
      poly_bank[oldest].noteOff();
      poly_queue.pop_front();
      //
      int newentry = oldest;
      //
      Voice& v = poly_bank[newentry];
      v.noteOn(note);
      v.setVelocity(velocity_sched);
      //leave a record : (# of voice bank, playing note #)
      poly_queue.push_back(std::pair<int, int>(newentry, note));
    }
  }
  //small waiting time for 'isPlaying' update?
  // delay(10);
  //monitoring the queue
  Serial.println("--notoOn:poly_queue---");
  Serial.println("(voice#, note#)");
  for (uint32_t idx = 0; idx < poly_queue.size(); idx++) {
    Serial.print("(");
    Serial.print(poly_queue[idx].first);
    Serial.print(", ");
    Serial.print(poly_queue[idx].second);
    Serial.println(")");
  }
  Serial.println();
}
//
Task scheduleNoteOn_task(0, TASK_ONCE, scheduleNoteOn);
//
void scheduleNoteOff() {
  for (auto it = poly_queue.begin(); it != poly_queue.end(); ++it) {
    //is this meaningful, btw?
    if ((*it).second == note_sched) {
      //okay. we've got that.
      Serial.println("okay. we've got that.");
      //a record : (# of voice bank, playing note #)
      poly_bank[(*it).first].noteOff(); // stop the bank
      poly_queue.erase(it); // remove the record
      break;
    }
  }
  //monitoring the queue
  Serial.println("--notoOff:poly_queue---");
  Serial.println("(voice#, note#)");
  for (uint32_t idx = 0; idx < poly_queue.size(); idx++) {
    Serial.print("(");
    Serial.print(poly_queue[idx].first);
    Serial.print(", ");
    Serial.print(poly_queue[idx].second);
    Serial.println(")");
  }
  Serial.println();
}
//
Task scheduleNoteOff_task(0, TASK_ONCE, scheduleNoteOff);

//
extern Task playcheck_task;
void playcheck() {
  //
  if (playcheck_task.isFirstIteration()) {
    //watchdog
    Watchdog.enable(1000);
  }
  //
#if defined(LED_INDICATOR)
  //
  bool is_nosound = true;
  for (uint32_t idx = 0; idx < poly_bank.size(); idx++) {
    if (poly_bank[idx].isPlaying()) {
      is_nosound = false;
    }
  }
  if (is_nosound) {
    //mark the indicator : LOW: OFF
    digitalWrite(13, LOW);
  } else {
    //mark the indicator : HIGH: ON
    digitalWrite(13, HIGH);
  }
#endif

  // //
  // Serial.print("AM_max:");
  // Serial.println(AudioMemoryUsageMax());

  //watchdog
  Watchdog.reset();
  // Serial.println("Watchdog.reset");
}
//
Task playcheck_task(200, TASK_FOREVER, playcheck);

//i2c
#include <Wire.h>
#include "../post_sampler.h"
// DISABLED.. due to bi-directional I2C hardship. ==> use UART.
// void requestEvent() {
//   Wire.write(" "); // no letter to send
// }
void receiveEvent(int numBytes) {
  //numBytes : how many bytes received(==available)
  static char letter_intro[POST_BUFF_LEN] = "................................";

  // Serial.println("[i2c] on receive!");
  int nb = Wire.readBytes(letter_intro, POST_LENGTH);

  if (POST_LENGTH == nb) {

    //convert to String
    String msg = String(letter_intro);
    Serial.println(msg);

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
    // Serial.println(str_key);
    // Serial.println(str_velocity);
    // Serial.println(str_gate);

    //
    int key = str_key.toInt();
    int velocity = str_velocity.toInt(); // 0 ~ 127
    int gate = str_gate.toInt();

    //
    if (gate == 0) {
      note_sched = key;
      scheduleNoteOff_task.restart();
    } else {
      note_sched = key;
      velocity_sched = velocity;
      scheduleNoteOn_task.restart();
    }
  }
}

// SD TEST
void printDirectory(File dir, int numTabs) {
  char filename[256] = "";
  while(true) {
    File entry =  dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    for (uint8_t i=0; i<numTabs; i++) {
      Serial.print('\t');
    }
    entry.getName(filename, 256);
    Serial.print(filename);
    // Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs+1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

//
File root;
void setup() {

  //serial monitor
  Serial.begin(115200);
  //
  delay(50);
  // <-- strange? but, this was needed !!
  // w/o ==> get killed by watchdog.. :(
  //
  // while (!Serial) {}
  //  --> use this.. to capture start-up messages, properly. very handy.

  //i2c
  Wire.begin(I2C_ADDR);
  Wire.onReceive(receiveEvent);
  // DISABLED.. due to bi-directional I2C hardship. ==> use UART.
  // Wire.onRequest(requestEvent);

  //SD
  if (!SD.begin()) {
    Serial.println("[sd] initialization failed!");
    return;
  }
  Serial.println("[sd] initialization done.");
  root = SD.open("/");
  printDirectory(root, 0);

  //polyphonics - 4 voices
  poly_bank.push_back(__voice_1);
  poly_bank.push_back(__voice_2);
  poly_bank.push_back(__voice_3);
  poly_bank.push_back(__voice_4);

  //audio
  AudioMemory(20); // <-- used AudioMemoryUsageMax() to check out!
#if !defined(TEENSY36)
  //NOTE!! teensy36 board..
  //       output broken? ..
  //       so disable this for teensy36..
  //       this is the cause??
#if defined(ANALOG_REF_EXTERNAL_3P3V)
  dacs1.analogReference(EXTERNAL);
#endif
#endif
  mixer1.gain(0,1.0);
  mixer1.gain(1,1.0);
  mixer1.gain(2,1.0);
  mixer1.gain(3,1.0);
  mixer2.gain(0,1.0);
  mixer2.gain(1,1.0);
  mixer2.gain(2,1.0);
  mixer2.gain(3,1.0);
  amp1.gain(1.0);
  amp2.gain(1.0);
  amp3.gain(1.0);
  amp4.gain(1.0);
  amp5.gain(1.0);
  amp6.gain(1.0);
  amp7.gain(1.0);
  amp8.gain(1.0);

  //tasks
  runner.addTask(playcheck_task);
  playcheck_task.restartDelayed(60000); // watchdog start after 60 sec. waiting task scheduling system stabilizing takes that so much time!
  //
  runner.addTask(scheduleNoteOn_task);
  runner.addTask(scheduleNoteOff_task);

  //led
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW); // LOW: OFF

  //
  Serial.println("[setup] done.");
}

void loop() {
  runner.execute();
}
