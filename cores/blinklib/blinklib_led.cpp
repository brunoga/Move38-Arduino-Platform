#include <string.h>

#include "blinklib_common.h"
#include "blinklib_led_internal.h"
#include "shared/blinkbios_shared_pixel.h"

namespace blinklib {

namespace led {

namespace internal {

pixelColor_t buffer_[PIXEL_COUNT];

void SaveState() {
  memcpy(buffer_, blinkbios_pixel_block.pixelBuffer,
         PIXEL_COUNT * sizeof(pixelColor_t));
}

void RestoreState() {
  memcpy(blinkbios_pixel_block.pixelBuffer, buffer_,
         PIXEL_COUNT * sizeof(pixelColor_t));
}

}  // namespace internal

}  // namespace led

}  // namespace blinklib