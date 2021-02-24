#include "blinklib_ir.h"

#include <string.h>

#include "blinklib_common.h"
#include "blinklib_ir_internal.h"
#include "blinklib_time_internal.h"
#include "blinklib_timer.h"
#include "blinklib_warm_sleep_internal.h"
#include "shared/blinkbios_shared_functions.h"
#include "shared/blinkbios_shared_irdata.h"

#ifdef BGA_CUSTOM_BLINKLIB_SEND_PROBE_TIMEOUT_MS
#define TX_PROBE_TIME_MS BGA_CUSTOM_BLINKLIB_SEND_PROBE_TIMEOUT_MS
#else
#define TX_PROBE_TIME_MS \
  150  // How often to do a blind send when no RX has happened recently to
       // trigger ping pong Nice to have probe time shorter than expire time so
       // you have to miss 2 messages before the face will expire
#endif

#ifdef BGA_CUSTOM_BLINKLIB_FACE_EXPIRATION_TIMEOUT_MS
#define RX_EXPIRE_TIME_MS BGA_CUSTOM_BLINKLIB_FACE_EXPIRATION_TIMEOUT_MS
#else
#define RX_EXPIRE_TIME_MS \
  200  // If we do not see a message in this long, then show that face as
       // expired
#endif

#define SEND_POSTPONE_WARM_SLEEP_LOCKOUT_MS \
  2000  // Any viral button presses received from IR within this time period are
        // ignored since insures that a single press can not circulate around
        // indefinitely.

#ifndef BGA_CUSTOM_BLINKLIB_DISABLE_DATAGRAM
#if IR_DATAGRAM_LEN > IR_RX_PACKET_SIZE
#error IR_DATAGRAM_LEN must not be bigger than IR_RX_PACKET_SIZE
#endif
#endif

namespace blinklib {

namespace ir {

namespace internal {

// All semantics chosen to have sane startup 0 so we can keep this in bss
// section and have it zeroed out at startup.

// Guaranteed delivery: Keeps track of the current datagram sequence and the
// sequence we are sending acks to.
union Header {
  struct {
    byte sequence : 3;
    byte ack_sequence : 3;
    bool postpone_sleep : 1;
    bool non_special : 1;
  };

  byte as_byte;
};

struct FaceData {
  BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE
  in_value;  // Last received value on this face, or 0 if no neighbor
             // ever seen since startup
#ifndef BGA_CUSTOM_BLINKLIB_DISABLE_DATAGRAM
  byte in_datagram[IR_DATAGRAM_LEN];
  byte in_datagram_len;  // 0= No datagram waiting to be read
#endif

  Header out_header;
  BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE
  out_value;  // Value we send out on this face
#ifndef BGA_CUSTOM_BLINKLIB_DISABLE_DATAGRAM
  byte out_datagram[IR_DATAGRAM_LEN];
  byte out_datagram_len;  // 0= No datagram waiting to be sent
#endif

  Timer expire_time;  // When this face will be considered to be expired (no
                      // neighbor there)
  Timer
      send_time;  // Next time we will transmit on this face (set to 0 every
                  // time we get a good message so we ping-pong across the link)

#ifdef BGA_CUSTOM_BLINKLIB_TRACK_FACE_CONNECTION
  bool connected;  // True if the face is currently connected.
#endif
};

static FaceData face_data_[FACE_COUNT];

static Timer send_postpone_warm_sleep_timer_;  // Set each time we send a viral
                                               // button press to avoid sending
                                               // getting into a circular loop.

#ifndef BGA_CUSTOM_BLINKLIB_DISABLE_DATAGRAM
#ifdef BGA_CUSTOM_BLINKLIB_ENABLE_CHECKSUM
static byte __attribute__((noinline))
compute_checksum(const byte *data, byte len) {
  byte checksum = 0;
  for (byte i = 0; i < len; ++i) {
    checksum = (checksum >> 1) + ((checksum & 1) << 7);
    checksum = (checksum + data[i]);
  }

  return checksum;
}
#endif
#endif

// Called anytime a the button is pressed or anytime we get a viral button press
// form a neighbor over IR Note that we know that this can not become cyclical
// because of the lockout delay.
void MaybeEnableSendPostponeWarmSleep() {
  if (send_postpone_warm_sleep_timer_.isExpired()) {
    send_postpone_warm_sleep_timer_.set(SEND_POSTPONE_WARM_SLEEP_LOCKOUT_MS);

    FOREACH_FACE(f) { face_data_[f].out_header.postpone_sleep = true; }

    // Prevent warm sleep
    blinklib::warm_sleep::internal::ResetTimer();

    // We also need to extend hardware sleep since we may not have got a
    // physical button press.
    BLINKBIOS_POSTPONE_SLEEP_VECTOR();
  }
}

static bool valid_data_received(volatile ir_rx_state_t *ir_rx_state) {
#ifdef BGA_CUSTOM_BLINKLIB_ENABLE_CHECKSUM
  if (ir_rx_state->packetBuffer[0] != IR_USER_DATA_HEADER_BYTE) return false;

  return (compute_checksum((const byte *)&ir_rx_state->packetBuffer[1],
                           ir_rx_state->packetBufferLen - 2) ==
          ir_rx_state->packetBuffer[ir_rx_state->packetBufferLen - 1]);
#else
  return ir_rx_state->packetBuffer[0] == IR_USER_DATA_HEADER_BYTE;
#endif
}

bool Send(byte face, const byte *data, byte len) {
#ifdef BGA_CUSTOM_BLINKLIB_ENABLE_CHECKSUM
  byte buffer[len + 1];
  memcpy(buffer, data, len);
  buffer[len] = compute_checksum(buffer, len);

  return BLINKBIOS_IRDATA_SEND_PACKET_VECTOR(face, buffer, len + 1);
#else
  return BLINKBIOS_IRDATA_SEND_PACKET_VECTOR(face, data, len);
#endif
}

void __attribute__((noinline)) ReceiveFaceData() {
  //  Use these pointers to step though the arrays
  FaceData *face_data = face_data_;
  volatile ir_rx_state_t *ir_rx_state = blinkbios_irdata_block.ir_rx_states;

  FOREACH_FACE(f) {
    // Check for anything new coming in...

#ifdef BGA_CUSTOM_BLINKLIB_TRACK_FACE_CONNECTION
    bool was_connected = face_data->connected;
#endif

    if (ir_rx_state->packetBufferReady) {
      // Some data is available.

      if (valid_data_received(ir_rx_state)) {
        // Got something that looks valid, so we know there is someone out
        // there.
#ifdef BGA_CUSTOM_BLINKLIB_TRACK_FACE_CONNECTION
        if (!was_connected) {
          face_data->connected = true;
          MaybeEnableSendPostponeWarmSleep();
        }
#endif

        face_data->expire_time.set(RX_EXPIRE_TIME_MS);

        // Clear to send on this face immediately to ping-pong
        // messages at max speed without collisions
        face_data->send_time.set(0);

        // packetData points just after the BlinkBIOS packet type byte.
        volatile const byte *packetData = (&ir_rx_state->packetBuffer[1]);

#ifdef BGA_CUSTOM_BLINKLIB_ENABLE_CHECKSUM
        byte packetDataLen =
            (ir_rx_state->packetBufferLen) -
            2;  // deduct the BlinkBIOS packet type byte and checksum.
#else
        byte packetDataLen = (ir_rx_state->packetBufferLen) -
                             1;  // deduct the BlinkBIOS packet type byte.
#endif

        // Guaranteed delivery: Parse incoming header.
        Header incoming_header;
        incoming_header.as_byte = packetData[0];

        if (incoming_header.non_special) {
          // Save face value.
          face_data->in_value =
              *((BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE *)&packetData[1]);

          // If there is a datagram, its is a normal one.

          if (incoming_header.postpone_sleep) {
            // The blink on on the other side of this connection
            // is telling us that a button was pressed recently
            // Send the viral message to all neighbors.
            MaybeEnableSendPostponeWarmSleep();
          }

#ifndef BGA_CUSTOM_BLINKLIB_DISABLE_DATAGRAM
          if (incoming_header.ack_sequence == face_data->out_header.sequence) {
            // We received an ack for the datagram we were sending. Mark it as
            // delivered.
            face_data->out_datagram_len = 0;
          }

          if (packetDataLen > 1 + sizeof(BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE)) {
            // We also received a datagram to process. If there is not already
            // a datagram in the local buffer, we will copy it there. If there
            // is one we will pretend we did not receive it so the other end
            // will retry sending. This allows delayed propagation of
            // datagrams in a cluster without losing data.
            if (incoming_header.sequence !=
                face_data->out_header.ack_sequence) {
              if (face_data->in_datagram_len == 0) {
                // Looks like a new one and there is no pending datagram to be
                // processed in this face. Record it and start sending acks
                // for it.
                face_data->out_header.ack_sequence = incoming_header.sequence;
                face_data->in_datagram_len =
                    packetDataLen -
                    sizeof(BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE) -
                    1;  // Subtract face
                        // value type and
                        // header byte.
                memcpy(&face_data->in_datagram,
                       (const void *)&packetData
                           [1 + sizeof(BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE)],
                       face_data->in_datagram_len);
              }
            }
          }
#endif
        } else {
          // Special packet.
          if (packetData[0] == TRIGGER_WARM_SLEEP_SPECIAL_VALUE &&
              packetData[1] == TRIGGER_WARM_SLEEP_SPECIAL_VALUE) {
            blinklib::warm_sleep::internal::Enter();
          }
        }
      }

      // No matter what, mark buffer as read so we can get next packet
      ir_rx_state->packetBufferReady = 0;
    }

#ifdef BGA_CUSTOM_BLINKLIB_TRACK_FACE_CONNECTION
    if (was_connected && face_data->expire_time.isExpired()) {
      face_data->connected = false;
      MaybeEnableSendPostponeWarmSleep();
    }
#endif

    // Increment our pointers.
    face_data++;
    ir_rx_state++;
  }
}

void SendFaceData() {
  //  Use these pointers to step though the arrays
  FaceData *face_data = face_data_;

  FOREACH_FACE(f) {
    // Send one out too if it is time....

    if (face_data->send_time.isExpired()) {  // Time to send on this face?
      // Note that we do not use the rx_fresh flag here because we want the
      // timeout to do automatic retries to kickstart things when a new
      // neighbor shows up or when an IR message gets missed

      face_data->out_header.non_special = true;

      // Total length of the outgoing packet. Face value + header + datagram.
#ifndef BGA_CUSTOM_BLINKLIB_DISABLE_DATAGRAM
      byte outgoingPacketLen = 1 + sizeof(BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE) +
                               face_data->out_datagram_len;
#else
      byte outgoingPacketLen = 1 + sizeof(BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE);
#endif
      // Ok, it is time to send something on this face.

      // Send packet.
      if (Send(f, (const byte *)&face_data->out_header, outgoingPacketLen)) {
        face_data->out_header.postpone_sleep = false;
      }

      // If the above returns 0, then we could not send because there was an RX
      // in progress on this face. In this case we will send this packet again
      // when the ongoing transfer finishes.

      // Guaranteed delivery: If it returns non-zero, we can not be sure the
      // datagram was received at the other end so we wait for confirmation
      // before marking the data as sent.

      // Here we set a timeout to keep periodically probing on this face,
      // but if there is a neighbor, they will send back to us as soon as
      // they get what we just transmitted, which will make us immediately
      // send again. So the only case when this probe timeout will happen is
      // if there is no neighbor there or if transmitting a datagram took more
      // time than the probe timeout (which will happen with big datagrams).
      face_data->send_time.set(TX_PROBE_TIME_MS + f);
    }
    face_data++;
  }
}

}  // namespace internal

}  // namespace ir

}  // namespace blinklib

using blinklib::ir::internal::face_data_;
using blinklib::ir::internal::FaceData;

#ifndef BGA_CUSTOM_BLINKLIB_DISABLE_DATAGRAM
byte getDatagramLengthOnFace(byte face) {
  return face_data_[face].in_datagram_len;
}

bool isDatagramReadyOnFace(byte face) {
  return getDatagramLengthOnFace(face) != 0;
}

bool isDatagramPendingOnFace(byte face) {
  return face_data_[face].out_datagram_len != 0;
}

bool isDatagramPendingOnAnyFace() {
  FOREACH_FACE(face) {
    if (isDatagramPendingOnFace(face)) return true;
  }

  return false;
}

void resetPendingDatagramOnFace(byte face) {
  face_data_[face].out_datagram_len = 0;
}

const byte *getDatagramOnFace(byte face) {
  return face_data_[face].in_datagram;
}

void markDatagramReadOnFace(byte face) { face_data_[face].in_datagram_len = 0; }

bool sendDatagramOnFace(const void *data, byte len, byte face) {
  if (len > IR_DATAGRAM_LEN) return false;

  FaceData *face_data = &face_data_[face];

  if (face_data->out_datagram_len != 0) return false;

  // Guaranteed delivery: Increment sequence number.
  face_data->out_header.sequence = face_data->out_header.sequence + 1;

  face_data->out_datagram_len = len;
  memcpy(face_data->out_datagram, data, len);

  return true;
}
#endif

BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE getLastValueReceivedOnFace(byte face) {
  return face_data_[face].in_value;
}

bool didValueOnFaceChange(byte face) {
  static byte prev_state[FACE_COUNT];

  byte curr_state = getLastValueReceivedOnFace(face);

  if (curr_state == prev_state[face]) {
    return false;
  }

  prev_state[face] = curr_state;

  return true;
}

bool __attribute__((noinline)) isValueReceivedOnFaceExpired(byte face) {
  return face_data_[face].expire_time.isExpired();
}

bool isAlone() {
  FOREACH_FACE(face) {
    if (!isValueReceivedOnFaceExpired(face)) {
      return false;
    }
  }
  return true;
}

void setValueSentOnAllFaces(BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE value) {
  FOREACH_FACE(face) { setValueSentOnFace(value, face); }
}

void setValueSentOnFace(BGA_CUSTOM_BLINKLIB_FACE_VALUE_TYPE value, byte face) {
  face_data_[face].out_value = value;
}
