#ifndef BLINKLIB_IR_INTERNAL_H_
#define BLINKLIB_IR_INTERNAL_H_

#include "blinklib_common.h"

namespace blinklib {

namespace ir {

namespace internal {

bool Send(byte face, const byte *data, byte len);

void MaybeEnableSendPostponeWarmSleep();

void ReceiveFaceData();
void SendFaceData();

}  // namespace internal

}  // namespace ir

}  // namespace blinklib

#endif
