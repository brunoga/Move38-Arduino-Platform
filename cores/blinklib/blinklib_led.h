#ifndef BLINKLIB_LED_H_
#define BLINKLIB_LED_H_

#include "blinklib_common.h"
#include "shared/blinkbios_shared_pixel.h"

#define MAX_BRIGHTNESS (255)

typedef pixelColor_t Color;

// Number of visible brightness levels in each channel of a color
#define BRIGHTNESS_LEVELS_5BIT 32
#define MAX_BRIGHTNESS_5BIT (BRIGHTNESS_LEVELS_5BIT - 1)

// R,G,B are all in the domain 0-31
// Here we expose the internal color representation, but it is worth it
// to get the performance and size benefits of static compilation
// Shame no way to do this right in C/C++

#define RED \
  pixelColor_t { .as_uint16 = 63 }
#define ORANGE \
  pixelColor_t { .as_uint16 = 1023 }
#define YELLOW \
  pixelColor_t { .as_uint16 = 2047 }
#define GREEN \
  pixelColor_t { .as_uint16 = 1985 }
#define CYAN \
  pixelColor_t { .as_uint16 = 65473 }
#define BLUE \
  pixelColor_t { .as_uint16 = 63489 }
#define MAGENTA \
  pixelColor_t { .as_uint16 = 63551 }
#define WHITE \
  pixelColor_t { .as_uint16 = 65535 }
#define OFF \
  pixelColor_t { .as_uint16 = 1 }

// Dim the specified color. Brightness is 0-255 (0=off, 255=don't dim at
// all-keep original color).
Color dim(Color color, byte brightness);

// Brighten the specified color. Brightness is 0-255 (0=unaltered color,
// 255=full white).
Color lighten(Color color, byte brightness);

// Change the tile to the specified color.
//
// NOTE: All color changes are double buffered and the display is updated when
// loop() returns
void setColor(Color newColor);

// Set the pixel on the specified face (0-5) to the specified color.
//
// NOTE: All color changes are double buffered and the display is updated when
// loop() returns.
void setColorOnFace(Color newColor, byte face);

// This maps 0-255 values to 0-31 values with the special case that 0 (in 0-255)
// is the only value that maps to 0 (in 0-31) This leads to some slight
// non-linearity since there are not a uniform integral number of 1-255 values
// to map to each of the 1-31 values.

// Make a new color from RGB values. Each value can be 0-255.
Color makeColorRGB(byte red, byte green, byte blue);

// Make a new color in the HSB colorspace. All values are 0-255.
Color makeColorHSB(byte hue, byte saturation, byte brightness);

#endif