#ifndef BLINKLIB_TIMER_H_
#define BLINKLIB_TIMER_H_

#include <stdint.h>

// Number of running milliseconds since power up.
//
// Important notes:
// 1) does not increment while sleeping
// 2) is only updated between loop() interations
// 3) is not monotonic, so always use greater than
//    and less than rather than equals for comparisons
// 4) overflows after about 50 days
// 5) is only accurate to about +/-10%

class Timer {
 private:
  uint32_t m_expireTime;  // When this timer will expire

 public:
  Timer(){};  // Timers come into this world pre-expired.

  bool isExpired();

  uint32_t getRemaining();

  void set(uint32_t ms);  // This time will expire ms milliseconds from now

  void add(uint16_t ms);

  void never(void);  // Make this timer never expire (unless set())
};

#endif
