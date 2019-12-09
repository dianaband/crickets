#define MSG_LENGTH_MAX 256

#define SPEAK_A_HELLO         (ID_SPEAK_A + 1)
#define SPEAK_A_SLEEPING      (ID_SPEAK_A + 2)
#define SPEAK_A_TIC           (ID_SPEAK_A + 3)
#define SPEAK_A_TAC           (ID_SPEAK_A + 4)
#define SPEAK_A_TOE           (ID_SPEAK_A + 5)
#define SPEAK_A_PLAYMODE      (ID_SPEAK_A + 6) // + XX : 0: Individual random, 1: Directional Propagation
#define SPEAK_A_PARA_SPEED    (ID_SPEAK_A + 7) // + XX : speed of rhythm
#define SPEAK_A_PARA_SNDSET   (ID_SPEAK_A + 8) // + XX : sound set select

//for group (all speakers)
#define SPEAKERS_HELLO        (ID_SPEAKERS + 1)
#define SPEAKERS_SLEEPING     (ID_SPEAKERS + 2)
#define SPEAKERS_TIC          (ID_SPEAKERS + 3)
#define SPEAKERS_TAC          (ID_SPEAKERS + 4)
#define SPEAKERS_TOE          (ID_SPEAKERS + 5)
#define SPEAKERS_PLAYMODE     (ID_SPEAKERS + 6) // + XX : 0: Individual random, 1: Directional Propagation
#define SPEAKERS_PARA_SPEED   (ID_SPEAKERS + 7) // + XX : speed of rhythm
#define SPEAKERS_PARA_SNDSET  (ID_SPEAKERS + 8) // + XX : sound set select

//common constants for all speakers
#define SPEAKERS_PLAYMODE_INDEP   (ID_SPEAKERS + 50)
#define SPEAKERS_PLAYMODE_PROPA   (ID_SPEAKERS + 51)
// #define SPEAKERS_LEADING       (ID_SPEAKERS + 80)
// #define SPEAKERS_FOLLOWING     (ID_SPEAKERS + 81)
