#include <avr/interrupt.h>

#include "blinklib_ir_internal.h"
#include "blinklib_led_internal.h"
#include "blinklib_timer.h"
#include "blinklib_warm_sleep_internal.h"
#include "shared/blinkbios_shared_button.h"
#include "shared/blinkbios_shared_functions.h"
#include "shared/blinkbios_shared_irdata.h"
#include "shared/blinkbios_shared_millis.h"

#define WARM_SLEEP_TIMEOUT_MS \
  (10 * 60 * 1000UL)  // 10 mins
                      // We will warm sleep if we do not see a button press or
                      // remote button press in this long

#define SLEEP_ANIMATION_DURATION_MS 300
#define SLEEP_ANIMATION_MAX_BRIGHTNESS 30
#define SLEEP_PACKET_REPEAT_COUNT \
  5  // How many times do we send the sleep and wake packets for redunancy?

namespace blinklib {

namespace warm_sleep {

namespace internal {

byte has_warm_woken_flag_ = 0;

Timer timer_;

// A special warm sleep trigger packet has len 2 and the two bytes are both the
// special cookie value Because it must be 2 long, this means that the cookie
// can still be a data value since that value would only have a 1 byte packet

static byte force_sleep_packet[2] = {TRIGGER_WARM_SLEEP_SPECIAL_VALUE,
                                     TRIGGER_WARM_SLEEP_SPECIAL_VALUE};

// This packet does nothing except wake up our neighbors

static byte nop_wake_packet[2] = {NOP_SPECIAL_VALUE, NOP_SPECIAL_VALUE};

static void __attribute__((noinline)) clear_packet_buffers() {
  FOREACH_FACE(f) {
    blinkbios_irdata_block.ir_rx_states[f].packetBufferReady = 0;
  }
}

void ResetTimer() { timer_.set(WARM_SLEEP_TIMEOUT_MS); }

void Enter() {
  BLINKBIOS_POSTPONE_SLEEP_VECTOR();  // Postpone cold sleep so we can warm
                                      // sleep for a while
  // The cold sleep will eventually kick in if we
  // do not wake from warm sleep in time.

  // Save the games pixels so we can restore them on waking
  // we need to do this because the sleep and wake animations
  // will overwrite whatever is there.

  blinklib::led::internal::SaveState();

  // Ok, now we are virally sending FORCE_SLEEP out on all faces to spread the
  // word and the pixels are off so the user is happy and we are saving power.

  // First send the force sleep packet out to all our neighbors
  // We are indiscriminate, just splat it 5 times everywhere.
  // This is a brute force approach to make sure we get though even with
  // collisions and long packets in flight.

  // We also show a little animation while transmitting the packets

  // Figure out how much brightness to animate on each packet

  // For the sleep animation we start bright and dim to 0 by the end

  // This code picks a start near to SLEEP_ANIMATION_MAX_BRIGHTNESS that makes
  // sure we end up at 0
  uint8_t fade_brightness = SLEEP_ANIMATION_MAX_BRIGHTNESS;

  for (uint8_t n = 0; n < SLEEP_PACKET_REPEAT_COUNT; n++) {
    FOREACH_FACE(f) {
      blinklib::led::internal::SetColorNow(Color{1, 0, 0, fade_brightness});

      fade_brightness--;

      blinklib::ir::internal::Send(f, force_sleep_packet, 2);
    }
  }

  // Ensure that we end up completely off
  blinklib::led::internal::SetColorNow(OFF);

  // We need to save the time now because it will keep ticking while we are in
  // pre-sleep (where were can get woken back up by a packet). If we did not
  // save it and then restore it later, then all the user timers would be
  // expired when we woke.

  // Save the time now so we can go back in time when we wake up
  cli();
  millis_t save_time = blinkbios_millis_block.millis;
  sei();

  // OK we now appear asleep
  // We are not sending IR so some power savings
  // For the next 2 hours will will wait for a wake up signal
  // TODO: Make this even more power efficient by sleeping between checks for
  // incoming IR.

  blinkbios_button_block.bitflags = 0;

  // Here is wuld be nice to idle the CPU for a bit of power savings, but there
  // is a potential race where the BIOS could put us into deep sleep mode and
  // then our idle would be deep sleep. you'd think we could turn of ints and
  // set out mode right before entering idle, but we needs ints on to wake form
  // idle on AVR.

  clear_packet_buffers();  // Clear out any left over packets that were there
                           // when we started this sleep cycle and might trigger
                           // us to wake unapropriately

  uint8_t saw_packet_flag = 0;

  // Wait in idle mode until we either see a non-force-sleep packet or a button
  // press or woke. Why woke? Because eventually the BIOS will make us powerdown
  // sleep inside this loop When that happens, it will take a button press to
  // wake us

  blinkbios_button_block.wokeFlag = 1;  // // Set to 0 upon waking from sleep

  while (!saw_packet_flag &&
         !(blinkbios_button_block.bitflags & BUTTON_BITFLAG_PRESSED) &&
         blinkbios_button_block.wokeFlag) {
    // TODO: This sleep mode currently uses about 2mA. We can get that way down
    // by...
    //       1. Adding a supporess_display_flag to pixel_block to skip all of
    //       the display code when in this mode
    //       2. Adding a new_pack_recieved_flag to ir_block so we only scan when
    //       there is a new packet
    // UPDATE: Tried all that and it only saved like 0.1-0.2mA and added dozens
    // of bytes of code so not worth it.

    ir_rx_state_t *ir_rx_state = blinkbios_irdata_block.ir_rx_states;

    FOREACH_FACE(f) {
      if (ir_rx_state->packetBufferReady) {
        if (ir_rx_state->packetBuffer[1] == NOP_SPECIAL_VALUE &&
            ir_rx_state->packetBuffer[2] == NOP_SPECIAL_VALUE) {
          saw_packet_flag = 1;
        }

        ir_rx_state->packetBufferReady = 0;
      }

      ir_rx_state++;
    }
  }

  cli();
  blinkbios_millis_block.millis = save_time;
  BLINKBIOS_POSTPONE_SLEEP_VECTOR();  // It is ok top call like this to reset
                                      // the inactivity timer
  sei();

  has_warm_woken_flag_ = 1;  // Remember that we warm slept
  ResetTimer();

  // Forced sleep mode
  // Really need button down detection in bios so we only wake on lift...
  // BLINKBIOS_SLEEP_NOW_VECTOR();

  // Clear out old packets (including any old FORCE_SLEEP packets so we don't go
  // right back to bed)

  clear_packet_buffers();

  // Show smooth wake animation

  // This loop empirically works out to be about the right delay.
  // I know this hardcode is hackyish, but we need to save flash space

  // For the wake animation we start off and dim to MAX by the end

  fade_brightness = 0;

  for (uint8_t n = 0; n < SLEEP_PACKET_REPEAT_COUNT; n++) {
    FOREACH_FACE(f) {
      // INcrement first - they are already seeing OFF when we start
      fade_brightness--;
      blinklib::led::internal::SetColorNow(
          Color{1, fade_brightness, fade_brightness, fade_brightness});

      blinklib::ir::internal::Send(f, nop_wake_packet, 2);
    }
  }

  // restore game pixels

  blinklib::led::internal::RestoreState();
}

}  // namespace internal

}  // namespace warm_sleep

}  // namespace blinklib
