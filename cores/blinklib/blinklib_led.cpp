#include <string.h>

#include "blinklib_common.h"  // TODO(bga): Added to account for missing header
                              // inclusion in the shared/* stuff. Fix it there.
#include "blinklib_led.h"
#include "blinklib_led_internal.h"
#include "shared/blinkbios_shared_functions.h"

namespace blinklib {

namespace led {

namespace internal {

static pixelColor_t buffer_[PIXEL_COUNT];

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

void __attribute__((noinline)) setColorOnFace(Color newColor, byte face) {
  // This is so ugly, but we need to match the volatile in the shared block to
  // the newColor There must be a better way, but I don't know it other than a
  // memcpy which is even uglier!

  // This at least gets the semantics right of coping a snapshot of the actual
  // value.

  blinkbios_pixel_block.pixelBuffer[face] = newColor;  // Size = 1940 bytes

  // This BTW compiles much worse

  //  *( const_cast<Color *> (&blinkbios_pixel_block.pixelBuffer[face])) =
  //  newColor;       // Size = 1948 bytes
}

void setColor(Color newColor) {
  FOREACH_FACE(f) { setColorOnFace(newColor, f); }
}

Color dim(Color color, byte brightness) {
  return {1, (byte)((color.r * (brightness + 1)) >> 8),
          (byte)((color.g * (brightness + 1)) >> 8),
          (byte)((color.b * (brightness + 1)) >> 8)};
}

Color __attribute__((noinline)) lighten(Color color, byte brightness) {
  return {1,
          (byte)(color.r +
                 (((MAX_BRIGHTNESS_5BIT - color.r) * (brightness + 1)) >> 8)),
          (byte)(color.g +
                 (((MAX_BRIGHTNESS_5BIT - color.g) * (brightness + 1)) >> 8)),
          (byte)(color.b +
                 (((MAX_BRIGHTNESS_5BIT - color.b) * (brightness + 1)) >> 8))};
}

Color makeColorRGB(byte red, byte green, byte blue) {
  // Internal color representation is only 5 bits, so we have to divide down
  // from 8 bits
  return {0, (byte)(red >> 3), (byte)(green >> 3), (byte)(blue >> 3)};
}

Color makeColorHSB(byte hue, byte saturation, byte brightness) {
  byte r;
  byte g;
  byte b;

  if (saturation == 0) {
    // achromatic (grey)
    r = g = b = brightness;
  } else {
    unsigned int scaledHue = (hue * 6);
    unsigned int sector =
        scaledHue >> 8;  // sector 0 to 5 around the color wheel
    unsigned int offsetInSector =
        scaledHue - (sector << 8);  // position within the sector
    unsigned int p = (brightness * (255 - saturation)) >> 8;
    unsigned int q =
        (brightness * (255 - ((saturation * offsetInSector) >> 8))) >> 8;
    unsigned int t =
        (brightness * (255 - ((saturation * (255 - offsetInSector)) >> 8))) >>
        8;

    switch (sector) {
      case 0:
        r = brightness;
        g = t;
        b = p;
        break;
      case 1:
        r = q;
        g = brightness;
        b = p;
        break;
      case 2:
        r = p;
        g = brightness;
        b = t;
        break;
      case 3:
        r = p;
        g = q;
        b = brightness;
        break;
      case 4:
        r = t;
        g = p;
        b = brightness;
        break;
      default:  // case 5:
        r = brightness;
        g = p;
        b = q;
        break;
    }
  }

  return (makeColorRGB(r, g, b));
}
