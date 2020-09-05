#include <string.h>

#include "blinklib_common.h"  // TODO(bga): Added to account for missing header
                              // inclusion in the shared/* stuff. Fix it there.
#include "blinklib_led.h"
#include "blinklib_led_internal.h"
#include "shared/blinkbios_shared_functions.h"

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

// Set the color and display it immediately. For internal use where we do not
// want the loop buffering.
void __attribute__((noinline)) SetColorNow(Color new_color) {
  setColor(new_color);
  BLINKBIOS_DISPLAY_PIXEL_BUFFER_VECTOR();
}

}  // namespace internal

}  // namespace led

}  // namespace blinklib

// --- Pixel functions

// Change the tile to the specified color
// NOTE: all color changes are double buffered
// and the display is updated when loop() returns

// Set the pixel on the specified face (0-5) to the specified color
// NOTE: all color changes are double buffered
// and the display is updated when loop() returns

// A buffer for the colors.
// We use a buffer so we can update all faces at once during a vertical
// retrace to avoid visual tearing from partially applied updates

void setColorOnFace(Color newColor, byte face) {
  // This is so ugly, but we need to match the volatile in the shared block to
  // the newColor There must be a better way, but I don't know it other than a
  // memcpy which is even uglier!

  // This at least gets the semantics right of coping a snapshot of the actual
  // value.

  blinkbios_pixel_block.pixelBuffer[face].as_uint16 =
      newColor.as_uint16;  // Size = 1940 bytes

  // This BTW compiles much worse

  //  *( const_cast<Color *> (&blinkbios_pixel_block.pixelBuffer[face])) =
  //  newColor;       // Size = 1948 bytes
}

void setColor(Color newColor) {
  FOREACH_FACE(f) { setColorOnFace(newColor, f); }
}

void setFaceColor(byte face, Color newColor) { setColorOnFace(newColor, face); }

Color dim(Color color, byte brightness) {
  return MAKECOLOR_5BIT_RGB((GET_5BIT_R(color) * brightness) / MAX_BRIGHTNESS,
                            (GET_5BIT_G(color) * brightness) / MAX_BRIGHTNESS,
                            (GET_5BIT_B(color) * brightness) / MAX_BRIGHTNESS);
}
