#include "blinklib_time.h"

#include <avr/interrupt.h>

#include "blinklib_time_internal.h"

namespace blinklib {

namespace time {

namespace internal {

// Millis snapshot for this pass though loop
millis_t now;

void updateNow() { now = currentMillis(); }

// Capture time snapshot
// It is 4 bytes long so we cli() so it can not get updated in the middle of
// us grabbing it
millis_t currentMillis() {
  cli();
  millis_t currentNow = blinkbios_millis_block.millis;
  sei();

  return currentNow;
}

}  // namespace internal

}  // namespace time

}  // namespace blinklib

millis_t millis() { return blinklib::time::internal::now; }
