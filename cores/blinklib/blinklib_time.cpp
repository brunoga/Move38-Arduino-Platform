#include "blinklib_time.h"

#include <avr/interrupt.h>

namespace blinklib {

namespace time {

// Millis snapshot for this pass though loop
millis_t now;

void updateNow() { now = currentMillis(); }

millis_t millis() { return now; }

// Capture time snapshot
// It is 4 bytes long so we cli() so it can not get updated in the middle of
// us grabbing it
millis_t currentMillis() {
  cli();
  millis_t currentNow = blinkbios_millis_block.millis;
  sei();

  return currentNow;
}

}  // namespace time

}  // namespace blinklib
