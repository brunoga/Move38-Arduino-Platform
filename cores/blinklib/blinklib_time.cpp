#include "blinklib_time.h"

#include <avr/interrupt.h>

namespace blinklib {

namespace time {

// Millis snapshot for this pass though loop
millis_t now;

// Capture time snapshot
// It is 4 bytes long so we cli() so it can not get updated in the middle of
// us grabbing it
void updateNow() {
  cli();
  now = blinkbios_millis_block.millis;
  sei();
}

millis_t millis() { return now; }

}  // namespace time

}  // namespace blinklib
