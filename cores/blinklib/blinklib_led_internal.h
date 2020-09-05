#ifndef BLINKLIB_LED_INTERNAL_H_
#define BLINKLIB_LED_INTERNAL_H_

#include "blinklib_led.h"

namespace blinklib {

namespace led {

namespace internal {

void SaveState();
void RestoreState();

void SetColorNow(Color new_color);

}  // namespace internal

}  // namespace led

}  // namespace blinklib

#endif