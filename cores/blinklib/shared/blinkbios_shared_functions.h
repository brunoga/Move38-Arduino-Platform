/*
 * blinkbios_shared_functions.h
 *
 * Defines the entry points into the BLINKBIOS called functions
 *
 */

// Note: Aesthetically it might seem better to put these function declarations with their functional groups, but ultimately I decided to
//       keep them all here in one place to avoid vector collisions. It is not a decision without downside though since a client who, say,
//       only wants the IR send funtion gets polluted with the others as well. :/

// The VECTOR definitions are used on the BIOS side to hook the correct incoming vector.
// The function definitions are used on the caller side to make the actual call to the vector.
// The vector number and the jmp address must be manually matched.

// The actual vector number is not important as long as it is not used for an actual ISR.  We really just use the vector pattern so the
// caller has a well known address to jmp to and vectors are always at a well known address.

// The `jmp`s just work because the caller will set up the args in the right registers and then make the jmp to the
// interrupt vector. Once there, the vector will directly call the target function with all the args still in place,
// then the target will return back to the original caller. Cool right? The optimizer will even put the target function at
// the vector jump address if it has no other callers, saving a jmp.

#ifndef BLINKBIOS_SHARED_FUCNTIONS_H_
#define BLINKBIOS_SHARED_FUCNTIONS_H_

// Send a user data packet
// See what we did here - we do a naked jump into vector_4, which is a jump to the `uint8_t ir_send_userdata( uint8_t face, const uint8_t *data , uint8_t len )` function
// it all works out because the params happened to be in the same registers because of the AVR C calling convention.
//  When compiling the BIOS with LTO, it even puts the send packet function right at the target of the vector.

// We use unused interrupt vectors to link between the user and BIOS code since they are at a known place.
// The links are defined as symbols like `boot_vectorX` where X is the number of the unused vector we are taking.
// In the BIOS project, these appear in the vector table in `startup.S`.
// IN the user mode projects, these appear in the linkscript and are hard coded to the correct addressed (based at the bootloader vtable at 0x3800).

// boot_vector4 is defined in the linkerscript and points to the boot loader's interrupt vector 4 at address 0x3810
// This is just a prototype so gcc knows what args to pass. The linker will resolve it to a jump to the address.

#define BLINKBIOS_IRDATA_SEND_PACKET_VECTOR boot_vector4      // This lands at base + 4 bytes per vector * 4th vector (init is at 0) = 0x10

extern "C" uint8_t BLINKBIOS_IRDATA_SEND_PACKET_VECTOR(  uint8_t face, const uint8_t *data , uint8_t len )  __attribute__((used)) __attribute__((noinline));

// Translate and copy the RGB values in the pixel buffer to raw PWM values in the
// raw pixel buffer. Waits for next vertical blanking interval to avoid
// display of partial update.

#define BLINKBIOS_DISPLAY_PIXEL_BUFFER_VECTOR boot_vector8

extern "C" void BLINKBIOS_DISPLAY_PIXEL_BUFFER_VECTOR()  __attribute__((used)) __attribute__((noinline));



#define BLINKBIOS_BOOTLOADER_SEED_VECTOR boot_vector9

extern "C" void BLINKBIOS_BOOTLOADER_SEED_VECTOR()  __attribute__((used)) __attribute__((noinline));


// Push back the inactivity sleep timer
// Can be called with interrupts off, so you can adjust the
// blinkbios_millis_block.millis and then call BLINKBIOS_POSTPONE_SLEEP_VECTOR
// to reset the sleep_time to match the new timebase
// Leaves with interrupts ON 

#define BLINKBIOS_POSTPONE_SLEEP_VECTOR boot_vector10

extern "C" void BLINKBIOS_POSTPONE_SLEEP_VECTOR()  __attribute__((used)) __attribute__((noinline));


#define BLINKBIOS_SLEEP_NOW_VECTOR boot_vector12

extern "C" void BLINKBIOS_SLEEP_NOW_VECTOR()  __attribute__((used)) __attribute__((noinline));

// Fill the flash page buffer using the boot_page_fill() function,
// then call here. Will write the page to flash, wait for it to complete,
// re-enable the rww part of flash, then return - so you can call this from
// the non-bootloader part of memory

#define BLINKBIOS_WRITE_FLASH_PAGE_VECTOR boot_vector13

extern "C" void BLINKBIOS_WRITE_FLASH_PAGE_VECTOR(uint8_t page) __attribute__((used)) __attribute__((noinline));


// Returns the version of the blinksbios present

#define BLINKBIOS_VERSION_VECTOR boot_vector14

extern "C" uint8_t BLINKBIOS_VERSION_VECTOR()  __attribute__((used)) __attribute__((noinline));


// Error mode. Blinks red until button pressed, then soft reset

#define BLINKBIOS_ABEND_VECTOR boot_vector15

extern "C" void BLINKBIOS_ABEND_VECTOR( uint8_t blinkCount ) __attribute__((used)) __attribute__((noinline)) __attribute__((noreturn));

#endif /* BLINKBIOS_SHARED_FUCNTIONS_H_ */