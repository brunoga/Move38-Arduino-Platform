/*
 *
 *  This library lives in userland and acts as a shim to th blinkos layer
 *
 *  This view tailored to be idiomatic Arduino-y. There are probably better
 * views of the interface if you are not an Arduinohead.
 *
 * In this view, each tile has a "state" that is represented by a number between
 * 1 and 127. This state value is continuously broadcast on all of its faces.
 * Each tile also remembers the most recently received state value from he
 * neighbor on each of its faces.
 *
 * You supply setup() and loop().
 *
 * While in loop(), the world is frozen. All changes you make to the pixels and
 * to data on the faces is buffered until loop returns.
 *
 */

#include "blinklib.h"

#include <avr/interrupt.h>  // cli() and sei() so we can get snapshots of multibyte variables
#include <avr/pgmspace.h>  // PROGMEM for parity lookup table
#include <avr/wdt.h>  // Used in randomize() to get some entropy from the skew between the WDT oscilator and the system clock.
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ArduinoTypes.h"

// Here are our magic shared memory links to the BlinkBIOS running up in the
// bootloader area. These special sections are defined in a special linker
// script to make sure that the addresses are the same on both the foreground
// (this blinklib program) and the background (the BlinkBIOS project compiled to
// a HEX file)

// The actual memory for these blocks is allocated in main.cpp. Remember, it
// overlaps with the same blocks in BlinkBIOS code running in the bootloader!

#include "blinklib_ir_internal.h"
#include "blinklib_led_internal.h"
#include "blinklib_time_internal.h"
#include "blinklib_warm_sleep_internal.h"
#include "shared/blinkbios_shared_button.h"
#include "shared/blinkbios_shared_functions.h"  // Gets us ir_send_packet()
#include "shared/blinkbios_shared_irdata.h"
#include "shared/blinkbios_shared_pixel.h"

// --------------Button code

// Here we keep a local snapshot of the button block stuff

static uint8_t buttonSnapshotDown;  // 1 if button is currently down (debounced)

static uint8_t buttonSnapshotBitflags;

static uint8_t
    buttonSnapshotClickcount;  // Number of clicks on most recent multiclick

bool buttonDown(void) { return buttonSnapshotDown != 0; }

static bool __attribute__((noinline)) grabandclearbuttonflag(uint8_t flagbit) {
  bool r = buttonSnapshotBitflags & flagbit;
  buttonSnapshotBitflags &= ~flagbit;
  return r;
}

bool buttonPressed() { return grabandclearbuttonflag(BUTTON_BITFLAG_PRESSED); }

bool buttonReleased() {
  return grabandclearbuttonflag(BUTTON_BITFLAG_RELEASED);
}

bool buttonSingleClicked() {
  return grabandclearbuttonflag(BUTTON_BITFLAG_SINGLECLICKED);
}

bool __attribute__((noinline)) buttonDoubleClicked() {
  return grabandclearbuttonflag(BUTTON_BITFLAG_DOUBLECLICKED);
}

bool buttonMultiClicked() {
  return grabandclearbuttonflag(BUTTON_BITFLAG_MULITCLICKED);
}

// The number of clicks in the longest consecutive valid click cycle since the
// last time called.
byte buttonClickCount() { return buttonSnapshotClickcount; }

// Remember that a long press fires while the button is still down
bool buttonLongPressed() {
  return grabandclearbuttonflag(BUTTON_BITFLAG_LONGPRESSED);
}

// 6 second press. Note that this will trigger seed mode if the blink is alone
// so you will only ever see this if blink has neighbors when the button hits
// the 6 second mark. Remember that a long press fires while the button is
// still down
bool buttonLongLongPressed() {
  return grabandclearbuttonflag(BUTTON_BITFLAG_3SECPRESSED);
}

// --- Utility functions

// OMG, the Arduino rand() function is just a mod! We at least want a uniform
// distibution.

// We base our generator on a 32-bit Marsaglia XOR shifter
// https://en.wikipedia.org/wiki/Xorshift

/* The state word must be initialized to non-zero */

// Here we use Marsaglia's seed (page 4)
// https://www.jstatsoft.org/article/view/v008i14
static uint32_t rand_state = 2463534242UL;

// Generate a new seed using entropy from the watchdog timer
// This takes about 16ms * 32 bits = 0.5s

void randomize() {
  WDTCSR = _BV(WDIE);  // Enable WDT interrupt, leave timeout at 16ms (this is
                       // the shortest timeout)

  // The WDT timer is now generating an interrupt about every 16ms
  // https://electronics.stackexchange.com/a/322817

  for (uint8_t bit = 32; bit; bit--) {
    blinkbios_pixel_block.capturedEntropy =
        0;  // Clear this so we can check to see when it gets set in the
            // background
    while (blinkbios_pixel_block.capturedEntropy == 0 ||
           blinkbios_pixel_block.capturedEntropy == 1)
      ;  // Wait for this to get set in the background when the WDT ISR fires
         // We also ignore 1 to stay balanced since 0 is a valid possible TCNT
         // value that we will ignore
    rand_state <<= 1;
    rand_state |= blinkbios_pixel_block.capturedEntropy &
                  0x01;  // Grab just the bottom bit each time to try and
                         // maximum entropy
  }

  wdt_disable();
}

// Note that rand executes the shift feedback register before returning the
// next result so hopefully we will be spreading out the entropy we get from
// randomize() on the first invokaton.

static uint32_t nextrand32() {
  // Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs"
  uint32_t x = rand_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rand_state = x;
  return x;
}

#define GETNEXTRANDUINT_MAX ((word)-1)

word randomWord(void) {
  // Grab bottom 16 bits

  return ((uint16_t)nextrand32());
}

// return a random number between 0 and limit inclusive.
// https://stackoverflow.com/a/2999130/3152071

word random(uint16_t limit) {
  word divisor = GETNEXTRANDUINT_MAX / (limit + 1);
  word retval;

  do {
    retval = randomWord() / divisor;
  } while (retval > limit);

  return retval;
}

/*

    The original Arduino map function which is wrong in at least 3 ways.

    We replace it with a map function that has proper types, does not
   overflow, has even distribution, and clamps the output range.

    Our code is based on this...

    https://github.com/arduino/Arduino/issues/2466

    ...downscaled to `word` width and with up casts added to avoid overflows
   (yep, even the corrected code in the `map() function equation wrong`
   discoussion would still overflow :/ ).

    In the casts, we try to keep everything at the smallest possible width as
   long as possible to hold the result, but we have to bump up to do the
   multiply. We then cast back down to (word) once we divide the (uint32_t) by
   a (word) since we know that will fit.

    We could trade code for performance here by special casing out each
   possible overflow condition and reordering the operations to avoid the
   overflow, but for now space more important than speed. User programs can
   alwasy implement thier own map() if they need it since this will not link
   in if it is not called.

    Here is some example code on how you might efficiently handle those
   multiplys...

    http://ww1.microchip.com/downloads/en/AppNotes/Atmel-1631-Using-the-AVR-Hardware-Multiplier_ApplicationNote_AVR201.pdf

*/

word map(word x, word in_min, word in_max, word out_min, word out_max) {
  // if input is smaller/bigger than expected return the min/max out ranges
  // value
  if (x < in_min) {
    return out_min;

  } else if (x > in_max) {
    return out_max;

  } else {
    // map the input to the output range.
    if ((in_max - in_min) > (out_max - out_min)) {
      // round up if mapping bigger ranges to smaller ranges
      // the only time we need full width to avoid overflow is after the
      // multiply but before the divide, and the single (uint32_t) of the
      // first operand should promote the entire expression - hopefully
      // optimally.
      return (word)(((uint32_t)(x - in_min)) * (out_max - out_min + 1) /
                    (in_max - in_min + 1)) +
             out_min;

    } else {
      // round down if mapping smaller ranges to bigger ranges
      // the only time we need full width to avoid overflow is after the
      // multiply but before the divide, and the single (uint32_t) of the
      // first operand should promote the entire expression - hopefully
      // optimally.
      return (word)(((uint32_t)(x - in_min)) * (out_max - out_min) /
                    (in_max - in_min)) +
             out_min;
    }
  }
}

// Returns the device's unique 8-byte serial number
// TODO: This should this be in the core for portability with an extra "AVR"
// byte at the front.

// 0xF0 points to the 1st of 8 bytes of serial number data
// As per "13.6.8.1. SNOBRx - Serial Number Byte 8 to 0"

const byte *const serialno_addr = (const byte *)0xF0;

// Read the unique serial number for this blink tile
// There are 9 bytes in all, so n can be 0-8

byte getSerialNumberByte(byte n) {
  if (n > 8) return (0);

  return serialno_addr[n];
}

// Returns the currently blinkbios version number.
// Useful to check is a newer feature is available on this blink.

byte getBlinkbiosVersion() { return BLINKBIOS_VERSION_VECTOR(); }

// Returns 1 if we have slept and woken since last time we checked
// Best to check as last test at the end of loop() so you can
// avoid intermediate display upon waking.

bool hasWoken() {
  bool ret = false;

  if (blinklib::warm_sleep::internal::has_warm_woken_flag_) {
    ret = true;
    blinklib::warm_sleep::internal::has_warm_woken_flag_ = 0;
  }

  if (blinkbios_button_block.wokeFlag ==
      0) {  // This flag is set to 0 when waking!
    ret = true;
    blinkbios_button_block.wokeFlag = 1;
  }

  return ret;
}

// Information on how the current game was loaded

uint8_t startState(void) {
  switch (blinkbios_pixel_block.start_state) {
    case BLINKBIOS_START_STATE_DOWNLOAD_SUCCESS:
      return START_STATE_DOWNLOAD_SUCCESS;

    case BLINKBIOS_START_STATE_WE_ARE_ROOT:
      return START_STATE_WE_ARE_ROOT;
  }

  // Safe catch all to be safe in case new ones are ever added
  return START_STATE_POWER_UP;
}

// This truly lovely code from the FastLED library
// https://github.com/FastLED/FastLED/blob/master/lib8tion/trig8.h
// ...adapted to save RAM by stroing the table in PROGMEM

/// Fast 8-bit approximation of sin(x). This approximation never varies more
/// than 2% from the floating point value you'd get by doing
///
///     float s = (sin(x) * 128.0) + 128;
///
/// @param theta input angle from 0-255
/// @returns sin of theta, value between 0 and 255

PROGMEM const uint8_t b_m16_interleave[] = {0, 49, 49, 41, 90, 27, 117, 10};

byte sin8_C(byte theta) {
  uint8_t offset = theta;
  if (theta & 0x40) {
    offset = (uint8_t)255 - offset;
  }
  offset &= 0x3F;  // 0..63

  uint8_t secoffset = offset & 0x0F;  // 0..15
  if (theta & 0x40) secoffset++;

  uint8_t section = offset >> 4;  // 0..3
  uint8_t s2 = section * 2;
  const uint8_t *p = b_m16_interleave;
  p += s2;

  uint8_t b = pgm_read_byte(p);
  p++;
  uint8_t m16 = pgm_read_byte(p);

  uint8_t mx = (m16 * secoffset) >> 4;

  int8_t y = mx + b;
  if (theta & 0x80) y = -y;

  y += 128;

  return y;
}

// #define NO_STACK_WATCHER to disable the stack overflow detection.
// saves a few bytes of flash and 2 bytes RAM

#ifdef NO_STACK_WATCHER

void statckwatcher_init() {}

uint8_t stackwatcher_intact() { return 1; }

#else

// We use this sentinel to see if we blew the stack
// Note that we can not statically initialize this because it is not
// in the initialized part of the data section.
// We check it periodically from the ISR

uint16_t __attribute__((section(".stackwatcher"))) stackwatcher;

#define STACKWATCHER_MAGIC_VALUE 0xBABE

void statckwatcher_init() { stackwatcher = STACKWATCHER_MAGIC_VALUE; }

uint8_t stackwatcher_intact() {
  return stackwatcher == STACKWATCHER_MAGIC_VALUE;
}

#endif

uint8_t __attribute__((weak)) sterileFlag =
    0;  // Set to 1 to make this game sterile. Hopefully LTO will compile this
        // away for us? (update: Whooha yes! ) We make `weak` so that the user
        // program can override it

// This is the main event loop that calls into the arduino program
// (Compiler is smart enough to jmp here from main rather than call!
//     It even omits the trailing ret!
//     Thanks for the extra 4 bytes of flash gcc!)

void __attribute__((noreturn)) run(void) {
  // TODO: Is this right? Should hasWoke() return true or false on the first
  // check after start up?

  blinkbios_button_block.wokeFlag =
      1;  // Clear any old wakes (wokeFlag is cleared to 0 on wake)

  blinklib::time::internal::updateNow();  // Initialize out internal millis so
                                          // that when we reset the warm sleep
                                          // counter it is right, and so setup
                                          // sees the right millis time
  blinklib::warm_sleep::internal::ResetTimer();

  statckwatcher_init();  // Set up the sentinel byte at the top of RAM used by
                         // variables so we can tell if stack clobbered it

  setup();

  while (1) {
    // Did we blow the stack?

    if (!stackwatcher_intact()) {
      // If so, show error code to user
      BLINKBIOS_ABEND_VECTOR(4);
    }

    // Here we check to enter seed mode. The button must be held down for 6
    // seconds and we must not have any neighbors Note that we directly read
    // the shared block rather than our snapshot. This lets the 6 second flag
    // latch and so to the user program if we do not enter seed mode because
    // we have neighbors. See?

    if ((blinkbios_button_block.bitflags & BUTTON_BITFLAG_3SECPRESSED) &&
        isAlone() && !sterileFlag) {
      // Button has been down for 6 seconds and we are alone...
      // Signal that we are about to go into seed mode with full blue...

      // First save the game pixels because our blue seed spin is going to
      // mess them up and we will need to get them back if the user continues
      // to hold past the seed phase and into the warm sleep phase.

      blinklib::led::internal::SaveState();

      // Now wait until either the button is lifted or is held down past 7
      // second mark so we know what to do

      uint8_t face = 0;

      while (blinkbios_button_block.down &&
             !(blinkbios_button_block.bitflags & BUTTON_BITFLAG_6SECPRESSED)) {
        // Show a very fast blue spin that it would be hard for a user program
        // to make during the 1 second they have to let for to enter seed mode

        setColor(OFF);
        setColorOnFace(BLUE, face++);
        if (face == FACE_COUNT) face = 0;
        BLINKBIOS_DISPLAY_PIXEL_BUFFER_VECTOR();
      }

      blinklib::led::internal::RestoreState();

      if (blinkbios_button_block.bitflags & BUTTON_BITFLAG_6SECPRESSED) {
        // Held down past the 7 second mark, so this is a force sleep request

        blinklib::warm_sleep::internal::Enter();
      } else {
        // They let go before we got to 7 seconds, so enter SEED mode! (and
        // never return!)

        // Give instant visual feedback that we know they let go of the button
        // Costs a few bytes, but the checksum in the bootloader takes a a sec
        // to complete before we start sending)
        blinklib::led::internal::SetColorNow(OFF);

        BLINKBIOS_BOOTLOADER_SEED_VECTOR();

        __builtin_unreachable();
      }
    }

    if ((blinkbios_button_block.bitflags & BUTTON_BITFLAG_6SECPRESSED)) {
      blinklib::warm_sleep::internal::Enter();
    }

    // Capture time snapshot
    // Used by millis() and Timer thus functions
    // This comes after the possible button holding to enter seed mode

    blinklib::time::internal::updateNow();

    if (blinkbios_button_block.bitflags &
        BUTTON_BITFLAG_PRESSED) {  // Any button press resets the warm sleep
                                   // timeout
      blinklib::ir::internal::MaybeEnableSendPostponeWarmSleep();
    }

    // Update the IR RX state
    // Receive any pending packets
    blinklib::ir::internal::ReceiveFaceData();

    cli();
    buttonSnapshotDown = blinkbios_button_block.down;
    buttonSnapshotBitflags |=
        blinkbios_button_block
            .bitflags;  // Or any new flags into the ones we got
    blinkbios_button_block.bitflags =
        0;  // Clear out the flags now that we have them
    buttonSnapshotClickcount = blinkbios_button_block.clickcount;
    sei();

    loop();

    // Update the pixels to match our buffer

    BLINKBIOS_DISPLAY_PIXEL_BUFFER_VECTOR();

    // Transmit any IR packets waiting to go out
    // Note that we do this after loop had a chance to update them.
    blinklib::ir::internal::SendFaceData();

    if (blinklib::warm_sleep::internal::timer_.isExpired()) {
      blinklib::warm_sleep::internal::Enter();
    }
  }
}
