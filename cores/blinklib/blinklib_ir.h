#ifndef BLINKLIB_IR_H_
#define BLINKLIB_IR_H_

#include "blinklib_common.h"

#if __has_include(<blinklib_config.h>)
#include <blinklib_config.h>
#endif

#ifndef BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE
#define BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE byte
#endif

// The value of the data sent and received on faces via IR can be between 0 and
// IR_DATA_VALUE_MAX.
#define IR_DATA_VALUE_MAX 255

// Returns the last received value on the indicated face. Between 0 and
// IR_DATA_VALUE_MAX inclusive. Returns 0 if no neighbor was ever seen on this
// face since power-up so best to only use after checking if face is not expired
// first.
BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE getLastValueReceivedOnFace(byte face);

// Did the neighborState value on this face change since the last time we
// checked? Note the a face expiring has no effect on the last value.
bool didValueOnFaceChange(byte face);

// False if messages have been recently received on the indicated face.
bool isValueReceivedOnFaceExpired(byte face);

// Returns false if there has been a neighboor seen recently on any face,
// returns true otherwise.
bool isAlone();

// Set value that will be continuously broadcast on specified face. Value should
// be between 0 and IR_DATA_VALUE_MAX inclusive. By default we power up with all
// faces sending the value 0.
void setValueSentOnFace(BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE value, byte face);

// Same as setValueSentOnFace(), but sets all faces in one call.
void setValueSentOnAllFaces(BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE value);

#ifndef BGA_CUSTOM_BLINKLIB_DISABLE_DATAGRAM
// Must be smaller than IR_RX_PACKET_SIZE.
#ifdef BGA_CUSTOM_BLINKLIB_IR_DATAGRAM_LEN
#define IR_DATAGRAM_LEN BGA_CUSTOM_BLINKLIB_IR_DATAGRAM_LEN
#else
#define IR_DATAGRAM_LEN 16
#endif

// Returns the number of bytes waiting in the data buffer, or 0 if no
// datagram is ready.
byte getDatagramLengthOnFace(byte face);

// Returns true if a datagram is available in the buffer.
bool isDatagramReadyOnFace(byte face);

// Returns true is a datagram is pending to be sent on the given face.
bool isDatagramPendingOnFace(byte face);

// Returns true is a datagram is pending to be sent on any face.
bool isDatagramPendingOnAnyFace();

// If a datagram is pending to be sent on a given face, this will reset it and
// it will not be delivered anymore.
void resetPendingDatagramOnFace(byte face);

// Returns a pointer to the actual received datagram data and resets the length
// of the data in the buffer to 0.
const byte *getDatagramOnFace(byte face);

// DEPRECATED: This is currently a no-op.
void markDatagramReadOnFace(byte face);

// Send a datagram.
bool sendDatagramOnFace(const void *data, byte len, byte face);
#endif

#endif
