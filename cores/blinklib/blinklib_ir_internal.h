#ifndef BLINKLIB_IR_INTERNAL_H_
#define BLINKLIB_IR_INTERNAL_H_

namespace blinklib {

namespace ir {

namespace internal {

void MaybeEnableSendPostponeWarmSleep();

void ReceiveFaceData();
void SendFaceData();

}  // namespace internal

}  // namespace ir

}  // namespace blinklib

#endif
