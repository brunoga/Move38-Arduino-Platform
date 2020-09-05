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

#define MAKECOLOR_5BIT_RGB(r, g, b) (pixelColor_t(r, g, b, 1))

#define RED MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT, 0, 0)
#define ORANGE \
  MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT, MAX_BRIGHTNESS_5BIT / 2, 0)
#define YELLOW MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT, MAX_BRIGHTNESS_5BIT, 0)
#define GREEN MAKECOLOR_5BIT_RGB(0, MAX_BRIGHTNESS_5BIT, 0)
#define CYAN MAKECOLOR_5BIT_RGB(0, MAX_BRIGHTNESS_5BIT, MAX_BRIGHTNESS_5BIT)
#define BLUE MAKECOLOR_5BIT_RGB(0, 0, MAX_BRIGHTNESS_5BIT)
#define MAGENTA MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT, 0, MAX_BRIGHTNESS_5BIT)

#define WHITE                                                  \
  MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT, MAX_BRIGHTNESS_5BIT, \
                     MAX_BRIGHTNESS_5BIT)

#define OFF MAKECOLOR_5BIT_RGB(0, 0, 0)

#define GET_5BIT_R(color) (color.r)
#define GET_5BIT_G(color) (color.g)
#define GET_5BIT_B(color) (color.b)

/*

        This set of functions lets you control the colors on the face RGB LEDs

*/

// Dim the specified color. Brightness is 0-255 (0=off, 255=don't dim at
// all-keep original color).
Color dim(Color color, byte brightness);

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

#endif