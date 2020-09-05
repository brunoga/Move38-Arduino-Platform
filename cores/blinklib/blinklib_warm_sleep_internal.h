#ifndef BLINKLIB_WARM_SLEEP_INTERNAL_H_
#define BLINKLIB_WARM_SLEEP_INTERNAL_H_

#include "blinklib_common.h"

// This is a special byte that triggers a warm sleep cycle when received
// It must appear in the first & second byte of data
// When we get it, we virally send out more warm sleep packets on all the faces
// and then we go to warm sleep.

#define TRIGGER_WARM_SLEEP_SPECIAL_VALUE 0b00010101

// This is a special byte that does nothing.
// It must appear in the first & second byte of data.
// We send it when we warm wake to warm wake our neighbors.

#define NOP_SPECIAL_VALUE 0b00110011

namespace blinklib {

namespace warm_sleep {

namespace internal {

// Remembers if we have woken from either a BIOS sleep or a blinklib forced
// sleep.
extern byte has_warm_woken_flag_;

// When will we warm sleep due to inactivity.Reset by a button press or seeing a
// button press bit on an incoming packet.
extern Timer timer_;

void ResetTimer();
void Enter();

}  // namespace internal

}  // namespace warm_sleep

}  // namespace blinklib

#endif