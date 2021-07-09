// Bare metal serial for blinks by Move38.com 

// An example showing how to output serial chars using as little as
// one word of flash to initialize and one additional word for each char sent. 

// This can be handy when you are looking for a bug but are very short on flash memory.
// Note that you would use this technique _instead_ of the normal `Serial.h`. 

// Note that these functions can only output bytes so you can not do something like `print(x)`. Instead, you can do something like `SERIAL_TX('L')` and
// then if you see `L` come up in the serial monitor you know that line got hit. Using a few of these, you can trace the path your code is taking.
// You can also do tricks like `SERIAL_TX(x+'0')` which will print the value of `x` as long as it is between 0 and 9. If you need to check larger values,
// you will need to use a different serial termial that lets you see raw data (Arduino serial monitor can not do this). Then you can do something like
// `SERIAL_TX(b)` and see what value the byte `x` is beteween 0 and 255. You can also then do `SERIAL_TX(w>>8);SERIAL_TX(w&0xff);` to see the value of
// the word in `w` (you do need to do a bit of math!). 

// To use: 
// 1. Put the `SERIAL_ENABLE_500KBS()` function somewhere in your code (probably at the top of `setup()`. This enables the serial trasnmitter at the default baud rate of 500Kbs. 
// 2. Optionally call the `SERIAL_2X_SPEED()` function. This will increase the serial speed to 1Mbs (as used by the normal blinks serial support) at the cost of an additional word of flash memory.
// 3. Call the `SERIAL_TX_BLIND()` and/or `SERIAL_TX_BLIND()` functions in your code to write a byte to the serial port. 

// The `SERIAL_TX_BLIND()` function writes blindly to the serial buffer so if you call it 3 times in a row faster than it can write the first two
// bytes out the serial port, then the third byte will be lost. At 1Mbs, the serial port sends a byte every 9 cycles, so even if you do only
// a tiny bit of work between calls then you are likely going to be safe. The actual write to the buffer only (and always) takes a single cycle.

// The `SERIAL_TX()` function will check to make sure the previously writen byte is no longer in the buffer (it has started actually transmitting)
// before writing this new byte, so this new byte will always be sent out the serial port. The cost of this extra check is up to 18 cycles and
// 6 additional words of flash memory per use. 

// Some defines for hardware register and bit locations. Humans can ignore these. 

#define _MMIO_BYTE(mem_addr) (*(volatile uint8_t *)(mem_addr))

#define _SFR_MEM8(mem_addr) _MMIO_BYTE(mem_addr)      // "special function register definitions look like C variables or simple constants" - https://www.nongnu.org/avr-libc/user-manual/group__avr__sfr__notes.html
#define UCSR0B                  _SFR_MEM8(0xC1)       // "USART Control and Status Register 0 B" - DS40001909A-page 224
#define TXEN0                   3                     // "Transmitter Enable 0" - DS40001909A-page 225
#define UCSR0A                  _SFR_MEM8(0xC0)       // "USART Control and Status Register 0 A" - DS40001909A-page 223
#define U2X0                    1                     // "Double the USART Transmission Speed"- DS40001909A-page 224
#define UDR0                    _SFR_MEM8(0xC6)       // "USART I/O Data Register 0" - DS40001909A-page 222
#define UDRE0                   5                     // "USART Data Register Empty" - DS40001909A-page 223

// Usable functions. You need to call the enable function at leat once (probably in 'setup()') and then you can use either or both
// of the write funtions any time after. 

// Enable serial port transmitter (disables digital mode on T pin). Uses default speed of 500Kbps. Must be called first. 
#define SERIAL_ENABLE_500KBS()  (UCSR0B  =  1<<TXEN0 )

// Optionally update the serial port to run at 2X speed, so if currently at 500Kbs will jump to 1Mbs
#define SERIAL_2X_SPEED()     (UCSR0A = 1<<U2X0 )        

// This will blindly write the specified char to the serial port. If there is already a char in the transmit buffer then this
// new char will be dropped, so only use if you know that consecutive chars will have enough time to transmit (or you don't
// care if some get lost). Completes in a single processor cycle and uses one flash word. 
#define SERIAL_TX_BLIND(c) (UDR0=c)

// Send a byte out the serial port. If the transmit buffer is already full then it will wait until the buffer is free.
// Takes up to 18 cycles (assuming it is called immedeatly after the buffer was filled) and uses 8 words of flash. 
// (The 'do{} while (0)' is a trick to allow multiplue statments in a #define) - https://stackoverflow.com/questions/257418/do-while-0-what-is-it-good-for
#define SERIAL_TX(c)  do {while (!(UCSR0A&(1<<UDRE0))); SERIAL_TX_BLIND(c);} while (0)

void setup() {
  SERIAL_ENABLE_500KBS();   // Enable the serial transmitter hardware. By default, this chip starts up at 500000 baud, which you can pick in the Arduino serial monitor pulldown if you want to use it.
  SERIAL_2X_SPEED();        // Switch to 1000000 baud. This is optional, but it is twice as fast and matches the speed of the normal serial functions so usually worth the extra word of flash memory it costs
  SERIAL_TX_BLIND('S');     // Note we can always use blind version here because we *know* nothing is in the buffer yet. 
}

void loop() {
  
  if (buttonSingleClicked()) {
    SERIAL_TX('1');
    SERIAL_TX('2');   
    SERIAL_TX('3');
  }


  if (buttonDoubleClicked()) {
    SERIAL_TX_BLIND('A');   // This will always work becuase your finger is not fast enough to double click again before the last transmit completed. 
    SERIAL_TX_BLIND('B');   // This will always work becase the 'A` above will start transmitting immedeately, so the single byte serial buffer will be free.
    SERIAL_TX_BLIND('C');   // Note that this 'C' will likely not actually be transmitted becuase the buffer was filled by the previous line.                               
  }

}
