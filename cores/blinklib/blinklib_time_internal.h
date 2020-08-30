#ifndef BLINKLIB_TIME_INTERNAL_H_
#define BLINKLIB_TIME_INTERNAL_H_

#include "shared/blinkbios_shared_millis.h"

namespace blinklib {

namespace time {

namespace internal {

extern millis_t now;

void updateNow();

millis_t currentMillis();

}  // namespace internal

}  // namespace time

}  // namespace blinklib

#endif