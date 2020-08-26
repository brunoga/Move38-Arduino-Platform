#include <limits.h>  // Get ULONG_MAX for NEVER

#include "blinklib.h"
#include "blinklib_time.h"

// Note we directly access millis() here, which is really bad style.
// The timer should capture millis() in a closure, but no good way to
// do that in C++ that is not verbose and inefficient, so here we are.

#define NEVER (ULONG_MAX)

// All Timers come into this world pre-expired, so their expireTime is 0
// Here we leave the constructor empty and depend in the BBS section clearing
// to set it to 0 (the constructor mechanism uses lots of flash).

bool Timer::isExpired() { return blinklib::time::millis() > m_expireTime; }

void Timer::set(uint32_t ms) { m_expireTime = blinklib::time::millis() + ms; }

uint32_t Timer::getRemaining() {
  uint32_t timeRemaining;

  if (blinklib::time::millis() >= m_expireTime) {
    timeRemaining = 0;

  } else {
    timeRemaining = m_expireTime - blinklib::time::millis();
  }

  return timeRemaining;
}

void Timer::add(uint16_t ms) {
  // Check to avoid overflow

  uint32_t timeLeft = NEVER - m_expireTime;

  if (ms > timeLeft) {
    m_expireTime = NEVER;

  } else {
    m_expireTime += ms;
  }
}

void Timer::never(void) { m_expireTime = NEVER; }
