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

//HACK: let auto-poweroff speakers stay turned ON! - (creative muvo mini)
#define IDLE_FREQ 22000
#define IDLE_AMP 0 // --> creative muvo 2 doesn't need this. they just stay on!

//teensy audio
#include <Audio.h>
// #include <SPI.h>
// #include <SD.h>
#include <SdFat.h>
SdFatSdioEX SD;
#include <SerialFlash.h>

//teensy 3.5 with SD card
#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used

// GUItool: begin automatically generated code
AudioPlaySdWav playSdWav1;               //xy=224,265
AudioSynthWaveformSine sine1;            //xy=236,361
AudioMixer4 mixer2;                      //xy=497,328
AudioMixer4 mixer1;                      //xy=499,245
AudioAmplifier amp1;                     //xy=633,245
AudioAmplifier amp2;                     //xy=634,328
AudioOutputAnalogStereo dacs1;           //xy=788,284
AudioConnection patchCord1(playSdWav1, 0, mixer1, 0);
AudioConnection patchCord2(playSdWav1, 1, mixer2, 0);
AudioConnection patchCord3(sine1, 0, mixer1, 1);
AudioConnection patchCord4(sine1, 0, mixer2, 1);
AudioConnection patchCord5(mixer2, amp2);
AudioConnection patchCord6(mixer1, amp1);
AudioConnection patchCord7(amp1, 0, dacs1, 0);
AudioConnection patchCord8(amp2, 0, dacs1, 1);
// GUItool: end automatically generated code

//task
#include <TaskScheduler.h>
Scheduler runner;
//sample #
int sample_now = 0; //0~99
void sample_player_start()
{
  //filename buffer - 8.3 naming convension! 8+1+3+1 = 13
  char filename[13] = "NNN.WAV";
  //search for the sound file
  int note = sample_now;
  int nnn = (note % 1000);  // 0~999
  int nn =  (note % 100);   // 0~99
  filename[0] = '0' + (nnn / 100); // N__.WAV
  filename[1] = '0' + (nn / 10);   // _N_.WAV
  filename[2] = '0' + (nn % 10);   // __N.WAV
  //TEST
  Serial.println(filename);
  AudioNoInterrupts();
  bool test = SD.exists(filename);
  AudioInterrupts();
  if (!test) {
    Serial.println("... does not exist.");
    return;
  }
  //start the player!
  //NOTE: block out 're-triggering'
  // if (playSdWav1.isPlaying() == false) {
  playSdWav1.play(filename);
  // }
  //mark the indicator : HIGH: ON
  digitalWrite(13, HIGH);
  //to wait a bit for updating isPlaying()
  delay(10);
}
void sample_player_stop() {
  //filename buffer - 8.3 naming convension! 8+1+3+1 = 13
  char filename[13] = "NNN.WAV";
  //search for the sound file
  int note = sample_now;
  int nnn = (note % 1000);  // 0~999
  int nn =  (note % 100);   // 0~99
  filename[0] = '0' + (nnn / 100); // N__.WAV
  filename[1] = '0' + (nn / 10);   // _N_.WAV
  filename[2] = '0' + (nn % 10);   // __N.WAV
  //TEST
  Serial.println(filename);
  AudioNoInterrupts();
  bool test = SD.exists(filename);
  AudioInterrupts();
  if (!test) {
    Serial.println("... does not exist.");
    return;
  }
  //stop the player.
  if (playSdWav1.isPlaying() == true) {
    playSdWav1.stop();
  }
}
void sample_player_check() {
  if (playSdWav1.isPlaying() == false) {
    //mark the indicator : LOW: OFF
    digitalWrite(13, LOW);
    //let speaker leave turned ON!
    sine1.amplitude(IDLE_AMP);
  }
  else {
    //let speaker leave turned ON!
    sine1.amplitude(0);
  }
}
//
Task sample_player_start_task(0, TASK_ONCE, sample_player_start);
Task sample_player_stop_task(0, TASK_ONCE, sample_player_stop);
Task sample_player_check_task(0, TASK_FOREVER, sample_player_check, &runner, true);

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
    sample_now = key;
    //
    int velocity = str_velocity.toInt(); // 0 ~ 127
    float amp_gain = (float)velocity / 127.0;
    amp1.gain(amp_gain);
    amp2.gain(amp_gain);
    //
    int gate = str_gate.toInt();
    if (gate == 0) {
      sample_player_stop_task.restart();
      Serial.println("sample_player_stop_task");
    } else {
      sample_player_start_task.restart();
      Serial.println("sample_player_start_task");
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

  //SD - AudioPlaySdWav @ teensy audio library needs SD.begin() first. don't forget/ignore!
  //+ let's additionally check contents of SD.
  if (!SD.begin()) {
    // if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("[sd] initialization failed!");
    return;
  }
  Serial.println("[sd] initialization done.");
  root = SD.open("/");
  printDirectory(root, 0);

  //audio
  AudioMemory(20);
#if !defined(TEENSY36)
  //NOTE!! teensy36 board.. output broken? .. so disable this for teensy36.. this is the cause??
  dacs1.analogReference(EXTERNAL);
#endif
  mixer1.gain(0,1.0);
  mixer1.gain(1,1.0);
  mixer1.gain(2,0);
  mixer1.gain(3,0);
  mixer2.gain(0,1.0);
  mixer2.gain(1,1.0);
  mixer2.gain(2,0);
  mixer2.gain(3,0);
  amp1.gain(1.0);
  amp2.gain(1.0);

  //let auto-poweroff speakers stay turned ON!
  sine1.frequency(IDLE_FREQ);

  //led
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW); // LOW: OFF

  //player task
  runner.addTask(sample_player_start_task);
  runner.addTask(sample_player_stop_task);

  //
  Serial.println("[setup] done.");
}

void loop() {
  runner.execute();
}
