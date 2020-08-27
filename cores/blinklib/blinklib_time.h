#ifndef BLINKLIB_TIME_H_
#define BLINKLIB_TIME_H_

#include "shared/blinkbios_shared_millis.h"

namespace blinklib {

namespace time {

extern millis_t now;

void updateNow();

millis_t millis();

millis_t currentMillis();

}  // namespace time

}  // namespace blinklib

#endif