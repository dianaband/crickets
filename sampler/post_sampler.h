#pragma once

#include "../post.h"

//
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
//
