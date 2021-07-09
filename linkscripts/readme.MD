## Custom linker scripts

### Code

We want user code to be org'ed at 0x000. Duing the build cycle we will move it up to the "build-in game area" of the flash where it lives until the blink repowers or the user wants to seed the local game, at which time it is coped down to 0x0000 by the BIOS and then jumpped into.

We also need the Arduino code to know where the bootloader starts so it can call the BIOS vectors. These vectors are named "boot_vect_X" and are hard coded to come at the start of the bootloader section. 

## Data

We use a shared data block for communication between the bootloader code and the foreground game. This lets us load that stuff once rather than having to include it in every game image. 

The shared memory block lives at a fixed location at the bottom of RAM using the `ipcram` sections. 

We must also update the size of the `data` section to match the RAM available on each chip. 

