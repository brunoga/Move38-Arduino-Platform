/*
 * blkinklib.h
 *
 * This defines a statefull view of the blinks tile interactions with neighbors.
 *
 * In this view, each tile has a "state" that is represented by a number between
 * 1 and 127. This state value is continuously broadcast on all of its faces.
 * Each tile also remembers the most recently received state value from he
 * neighbor on each of its faces.
 *
 * Note that this library depends on the blinklib library for communications
 * with neighbors. The blinklib IR read functions are not available when using
 * the blinkstate library.
 *
 * Note that the beacon transmissions only occur when the loop() function
 * returns, so it is important that sketches using this model return from loop()
 * frequently.
 *
 */

#ifndef BLINKLIB_H_
#define BLINKLIB_H_

// Define this so programs can require this specific version.
#define BGA_CUSTOM_BLINKLIB

#include <limits.h>  // UINTLONG_MAX for NEVER

#include "ArduinoTypes.h"
#include "blinklib_common.h"
#include "blinklib_ir.h"
#include "blinklib_led.h"
#include "blinklib_time.h"
#include "blinklib_timer.h"

/*

        This set of functions let you test for changes in the environment.

*/

/*

    Button functions

*/

// Debounced view of button state. true if the button currently pressed.

bool buttonDown(void);

// Was the button pressed or lifted since the last time we checked?
// Note that these register the change the instant the button state changes
// without any delay, so good for latency sensitive cases.
// It is debounced, so the button must have been in the previous state a minimum
// debounce time before a new detection will occur.

bool buttonPressed(void);
bool buttonReleased(void);

// Was the button single, double , or multi clicked since we last checked?
// Note that there is a delay after the button is first pressed
// before a click is registered because we have to wait to
// see if another button press is coming.
// A multiclick is 3 or more clicks

// Remember that these click events fire a short time after the button is lifted
// on the final click If the button is held down too long on the last click,
// then click interaction is aborted.

bool buttonSingleClicked();

bool buttonDoubleClicked();

bool buttonMultiClicked();

// The number of clicks in the longest consecutive valid click cycle since the
// last time called.
byte buttonClickCount();

// Remember that a long press fires while the button is still down
bool buttonLongPressed();

// 6 second press. Note that this will trigger seed mode if the blink is alone
// so you will only ever see this if blink has neighbors when the button hits
// the 6 second mark. Remember that a long press fires while the button is still
// down
bool buttonLongLongPressed();

/*

    Utility functions

*/

// Return a random number between 0 and limit inclusive.
// By default uses a hardcoded seed. If you need different blinks
// to generate different streams of random numbers, then call
// randomize() once (probably in setup()) to generate a truely random seed.

word random(word limit);

// Generate a random 16 bit word. Slightly faster than random(),
// but be careful because you will not get a uniform distribution
// unless your desired range is a power of 2.

word randomWord(void);

// Generate a new random seed using entropy from the watchdog timer
// This takes about 16ms * 32 bits = 0.5s

void randomize();

// Read the unique serial number for this blink tile
// There are 9 bytes in all, so n can be 0-8

#define SERIAL_NUMBER_LEN 9

byte getSerialNumberByte(byte n);

// Returns the current blinkbios version number.
// Useful to check is a newer feature is available on this blink.

byte getBlinkbiosVersion();

// Map one set to another
// Note that this explodes to big code, so do the explicit calculations
// by hand if you are running out of flash space.

word map(word x, word in_min, word in_max, word out_min, word out_max);

// Maps theta 0-255 to values 0-255 in a sine wave
// Based on fabulous FastLED library code here...
// https://github.com/FastLED/FastLED/blob/master/lib8tion/trig8.h#L159

byte sin8_C(byte theta);

/* Power functions */

// The blink will automatically sleep if the button has not been pressed in
// more than 10 minutes. The sleep is preemptive - the blink stops in the middle
// of whatever it happens to be doing.

// The blink wakes from sleep when the button is pressed. Upon waking, it picks
// up from exactly where it left off. It is up to the programmer to check to see
// if the blink has slept and then woken and react accordingly if desired.

// Returns 1 if we have woken from sleep since last time we checked

bool hasWoken();

// Information on how the current game was loaded

#define START_STATE_POWER_UP \
  0  // Loaded the built-in game (for example, after battery insertion or failed
     // download)
#define START_STATE_WE_ARE_ROOT \
  1  // Completed sending a download seed (running built-in game)
#define START_STATE_DOWNLOAD_SUCCESS \
  2  // Completed receiving a downloaded game (running the downloaded game)

byte startState(void);

// Make the current game sterile so that it can not be propagated to other
// blinks (niche) If sterileFlag==1 then holding the button down will never
// enter seed mode, it will just eventually sleep. This essentially means that
// once this built-in game is programmed into a blink it will not transfer to
// other blinks.

// There are two ways you can use this. If you statically want a game to be
// sterile, you can add... uint8_t sterileFlag=1;             // Make this game
// sterile. outside of any function block. This will completely disable all
// seeding and also give you some extra flash memory since the seeding code does
// not even get linked into the executable (thanks gcc LTO!) Alternately, you
// can use.. sterileFlag=1;
// ...and...
// sterileFlag=0;
// inside your code to dynamically enable and disable seeding at any time.
// NB: setting sterileFlag only suppress button-initiated seeds.

extern uint8_t sterileFlag;  // Set to 1 to make this game sterile.

/*

    These hook functions are filled in by the sketch

*/

// Called when this sketch is first loaded and then
// every time the tile wakes from sleep

void setup(void);

// Called repeatedly just after the display pixels
// on the tile face are updated

void loop();

#endif /* BLINKLIB_H_ */
