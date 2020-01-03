//
// wirelessly connected cloud (Wireless Mesh Networking)
// MIDI-like
// spacial
// sampler keyboard
//

//
// RTA @ Seoul w/ Post Territory Ujeongguk
//

//
// (part-3) teensy35 : 'client:sampler' (mesh post --> play sounds)
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
//
// 'USE_SD'
// --> a original sd card driver..
//
// #define USE_SD
//
// 'USE_SDFATSDIO'
// --> a faster sd card driver..
//
// #define USE_SDFATSDIO
//
// 'USE_SDFATBETA'
// --> a faster sd card driver..
//
#define USE_SDFATBETA
//----------</configuration>----------

//watchdog
#include <Adafruit_SleepyDog.h>

//teensy audio
#if defined(USE_SD)
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#define xFile File
//
#elif defined(USE_SDFATSDIO)
#include <Audio.h>
#include <SdFat.h>
SdFatSdio SD;
// SdFatSdioEX SD;
#include <SerialFlash.h>
#define xFile File
//
#elif defined(USE_SDFATBETA)
#include <Audio.h>
#include <SdFat.h>
#include <SerialFlash.h>
SdFs SD;
#define xFile FsFile
// SdExFat SD;
// #define xFile ExFile
#endif

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

//threads
#include <TeensyThreads.h>
//following class is from --> https://github.com/ftrias/TeensyThreads/blob/master/examples/Runnable/
class Runnable {
private:
protected:
  virtual void runTarget(void *arg) = 0;
public:
  virtual ~Runnable(){
  }

  static void runThread(void *arg) {
    Runnable *_runnable = static_cast<Runnable*> (arg);
    _runnable->runTarget(arg);
  }
};

//NOTE: using TeensyThreads == nightmare of 'volatile' problems.
// ==> wrap all the variables that is used to comm. between threads into a 'global' & 'volatile' object..
class vVoice {
public:
  int note_now;
  int velocity_now;
  int start_offset;
  bool note_on_req;
  bool note_off_req;
  bool is_playing;

  vVoice() {
    note_now = 0;
    velocity_now = 0;
    start_offset = 0;
    note_on_req = false;
    note_off_req = false;
    is_playing = false;
  }

  vVoice(const vVoice &vv) {
    note_now = vv.note_now;
    velocity_now = vv.velocity_now;
    start_offset = vv.start_offset;
    note_on_req = vv.note_on_req;
    note_off_req = vv.note_off_req;
    is_playing = vv.is_playing;
  }
};
//
class Voice : public Runnable {
  //private

  //teensy audio
  AudioPlaySdWav& player;
  AudioAmplifier& ampL;
  AudioAmplifier& ampR;

  //teensythreads
  std::thread* thrd;

protected:
  //teensythreads
  void runTarget(void *arg) {

    // a filename buffer
    char filename[13] = "NN.WAV";

    //
    while(1) {
      {
        Threads::Scope m(mx);

        // 'note off' request
        if (vv.note_off_req == true) {
          vv.note_off_req = false;
          player.stop();
          vv.note_now = 0; // declare that [i'm free.]
          vv.is_playing = false;
        }
        // 'note on' request
        else if (vv.note_on_req == true) {
          vv.note_on_req = false;
          // set filename to play...
          int nn = (vv.note_now % 100);   // 0~99
          filename[0] = '0' + (nn / 10);   // [N]N.WAV
          filename[1] = '0' + (nn % 10);   // N[N].WAV
          // the filename to play is...
          Serial.println(filename);
          // go! (re-triggering)
          // threads.delay(vv.start_offset); // <-- so.. this also means 'yield' so.. sth. unexpected happening here? so, we cannot do this? -> SAD.
          AudioNoInterrupts(); // maybe .. 'AudioNoInterrupts' helps for the stability?
          // apply gains
          int val = vv.velocity_now;
          if (val < 0) val = 0;
          float gg = (float)val / 127; // allowing +gain for values over 127.
          ampL.gain(gg);
          ampR.gain(gg);
          bool res = player.play(filename);
          Serial.print("player.play -> "); Serial.println(res); // maybe.. meaningless? play() is kinda blocking call... -> SAD
          AudioInterrupts();
          // --> we just believe that this 'file' is existing & available. NO additional checking.
          // threads.delay(10);
          // --> let's wait a bit before exit, to give more room to work for background workers(==filesystem|audio-interrupts)
          // --> if we get too fast 'player.play' twice, then the system might get broken/stalled. ?
          vv.is_playing = true;
        }
      }

      //
      threads.yield();
      // threads.delay(5);
    }
  }

public:

  //mutex
  Threads::Mutex& mx; // BETTER hav only 1 mutex. for all? so, only 1 change can happen at a time.

  //variables for Voice
  volatile vVoice& vv;

  //
  Voice(AudioPlaySdWav& player_, AudioAmplifier& ampL_, AudioAmplifier& ampR_, Threads::Mutex& mx_, volatile vVoice& vv_)
    : player(player_), ampL(ampL_), ampR(ampR_), mx(mx_), vv(vv_) {
  }

  //threading
  void start() {
    thrd = new std::thread(&Runnable::runThread, this);
  }
};

// voice banks
#include <vector>
#include <deque>
//NOTE: stronger locking? there's only 1 locking mutex.
Threads::Mutex __mx;
//NOTE: using TeensyThreads == nightmare of 'volatile' problems.
// ==> wrap all the variables that is used to comm. between threads into a 'global' & 'volatile' object..
static volatile vVoice __vv1;
static volatile vVoice __vv2;
// static volatile vVoice __vv3;
// static volatile vVoice __vv4;
// ==> and link global volatile objects to the actual objects like :
static Voice __voice_1(playSdWav1, amp1, amp2, __mx, __vv1);
static Voice __voice_2(playSdWav2, amp3, amp4, __mx, __vv2);
// static Voice __voice_3(playSdWav3, amp5, amp6, __mx, __vv3);
// static Voice __voice_4(playSdWav4, amp7, amp8, __mx, __vv4);
static std::vector<Voice> poly_bank;
static std::deque< std::pair<int, int> > poly_queue;

// polyphonics
static int note_sched = 0;
static int velocity_sched = 0;
static int offset_sched = 0;
void scheduleNoteOn()
{
  //filename buffer - 8.3 naming convension! 8+1+3+1 = 13
  char fname[13] = "NN.WAV";
  //search for the sound file
  int note = (note_sched % 100);    // 0~99
  fname[0] = '0' + (note / 10);     // [N]N.WAV
  fname[1] = '0' + (note % 10);     // N[N].WAV
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
      is_already = true;
      Voice& v = poly_bank[poly_queue[idx].first];
      {
        Threads::Scope m(v.mx);
        v.vv.note_now = note;
        v.vv.velocity_now = velocity_sched;
        v.vv.start_offset = offset_sched;
        v.vv.note_on_req = true;
      }
      break;
      // (2) do nothing (just let it play till end)
      // is_already = true;
      // break;
      // (3) trigger a new one? (schedule a new one overlapping)
      // --> then you just ignore the fact.
      //     i.e. do not tick 'is_already = true',
      //     then a new one will be automatically assigned.
      // break;
    }
  }
  //it's sth. new..
  if (is_already == false) {
    //fine, is there idle voice?
    bool is_found_idle = false;
    for (uint32_t idx = 0; idx < poly_bank.size(); idx++) {
      Voice& v = poly_bank[idx];
      {
        Threads::Scope m(v.mx);
        if (v.vv.note_now == 0) {
          //cool, got one.
          is_found_idle = true;
          //play start-up
          v.vv.note_now = note;
          v.vv.velocity_now = velocity_sched;
          v.vv.start_offset = offset_sched;
          v.vv.note_on_req = true;
          //leave a record : (# of voice bank, playing note #)
          poly_queue.push_back(std::pair<int, int>(idx, note));
          break;
        }
      }
    }
    //oh, no idle one!
    if (is_found_idle == false) {
      //then, who's the oldest?
      int oldest = poly_queue.front().first;
      //poly_bank[oldest].noteOff();
      poly_queue.pop_front();
      //
      int newentry = oldest;
      //
      Voice& v = poly_bank[newentry];
      {
        Threads::Scope m(v.mx);
        v.vv.note_now = note;
        v.vv.velocity_now = velocity_sched;
        v.vv.start_offset = offset_sched;
        v.vv.note_on_req = true;
      }
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
void scheduleNoteOff() {
  for (auto it = poly_queue.begin(); it != poly_queue.end(); ++it) {
    //is this meaningful, btw?
    if ((*it).second == note_sched) {
      //okay. we've got that.
      Serial.println("okay. we've got that.");
      //a record : (# of voice bank, playing note #)
      Voice& v = poly_bank[(*it).first];
      {
        Threads::Scope m(v.mx);
        v.vv.note_off_req = true; // stop the bank
      }
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
void playcheck() {
  while(1) {
#if defined(LED_INDICATOR)
    bool is_nosound = true;
    for (uint32_t idx = 0; idx < poly_bank.size(); idx++) {
      Voice& v = poly_bank[idx];
      {
        Threads::Scope m(v.mx);
        if (v.vv.is_playing) is_nosound = false;
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

    threads.delay(100);
  }
}

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
    //    : [KKKVVVGOOOOOLIIIII............]
    //    : KKK - Key
    //      .substring(1, 4);
    //    : VVV - Velocity (volume/amp.)
    //      .substring(4, 7);
    //    : G - Gate (note on/off)
    //      .substring(7, 8);
    //    : O - timing offset (plz start after this milli-sec)
    //      .substring(8, 13);
    //    : L - looping mode (plz do no/auto/manual-looping)
    //      .substring(13, 14);
    //        L == 1 -> no-looping. play once.
    //        L == 2 -> auto-restart when the playback ends.
    //        L == 3 -> restart after 'timing interval' milli-sec.
    //    : I - looping interval (only valid for 'manual-looping')
    //      .substring(14, 19); ==> check 'special cases' first.

    String str_key = msg.substring(1, 4);
    String str_velocity = msg.substring(4, 7);
    String str_gate = msg.substring(7, 8);
    String str_offset = msg.substring(8, 13);
    String str_loop = msg.substring(13, 14);
    String str_interval = msg.substring(14, 19);

    //
    int key = str_key.toInt();
    int velocity = str_velocity.toInt(); // 0 ~ 127
    int gate = str_gate.toInt();
    int offset = str_offset.toInt();
    int loop = str_loop.toInt();
    int interval = str_interval.toInt();

    //
    if (gate == 0) {
      note_sched = key;
      scheduleNoteOff();
    } else {
      note_sched = key;
      velocity_sched = velocity;
      offset_sched = offset;
      scheduleNoteOn();
    }
  }
}

// SD TEST
void printDirectory(xFile dir, int numTabs) {
  char filename[256] = "";
  while(true) {

    xFile entry =  dir.openNextFile();
    if (!entry) {
      break;
    }
    for (uint8_t i=0; i<numTabs; i++) {
      Serial.print('\t');
    }
#if defined(USE_SD)
    Serial.print(entry.name());
#elif defined(USE_SDFATSDIO)
    entry.getName(filename, 256);
    Serial.print(filename);
#elif defined(USE_SDFATBETA)
    entry.getName(filename, 256);
    Serial.print(filename);
#endif
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs+1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println((unsigned int)entry.size(), DEC);
    }
    entry.close();
  }
}

//
xFile root;
void setup() {

  //serial monitor
  Serial.begin(115200);
  while (!Serial) {}

  //i2c
  Wire.begin(I2C_ADDR);
  Wire.onReceive(receiveEvent);
  // DISABLED.. due to bi-directional I2C hardship. ==> use UART.
  // Wire.onRequest(requestEvent);

  //SD
#if defined(USE_SD)
  if (!(SD.begin(BUILTIN_SDCARD)))
#elif defined(USE_SDFATSDIO)
  if (!SD.begin())
#elif defined(USE_SDFATBETA)
  if (!SD.begin(SdioConfig(FIFO_SDIO)))
#endif
  {
    Serial.println("[sd] initialization failed!");
    return;
  }
  Serial.println("[sd] initialization done.");
  root = SD.open("/");
  printDirectory(root, 0);

  //polyphonics - 4 voices
  poly_bank.push_back(__voice_1); __voice_1.start();
  poly_bank.push_back(__voice_2); __voice_2.start();
  // poly_bank.push_back(__voice_3); __voice_3.start();
  // poly_bank.push_back(__voice_4); __voice_4.start();

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

  //teensythreads
  threads.addThread(playcheck);

  //led
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW); // LOW: OFF

  //
  Serial.println("[setup] done.");

  //watchdog
  Watchdog.enable(3000);
}

void loop() {
}
