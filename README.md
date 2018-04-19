table of content
-----------------------

1) What is NesSnesN64GC2Usb ?
2) USB Implementation
3) Compilation, Installation and Fuse bits
4) vendor id/product id pair


1) What is NesSnesN64GC2Usb ?
-------------------------------------------

NesSnesN64GC2Usb is a project i started after having salvaged a few ATMega8's and wanting to do things with them.
As i often play a game on an NES, SNES or GC emulator and having the actual controllers and console i quickly realised that raphnet.net's projects were perfect.
This is a project in which i merged, and modified , his NesSnes2USB and gc_n64_usb to support all 4 at once.
the firmware allows for 2 nes or snes controllers, 1 GC controller and 1 N64 controller

at this moment , due to size restraint on the ATMega8 , a switch is on PB0 to select NES/SNES or GC/N64. 
in the future i will try to rewrite a part of the code hoping i can use them all at once to control 1 actual controller...
else, ill have to switch to the ATMega168 which has 16KB flash instead of 8 that the ATMega8 has.

[b]EDIT : [/b] the commit on 19/04/2018 should have this fixed. 2 resistors should be added and then it detects automatically, giving priority to (S)NES controller!



2) USB Implementation
-------------------------------------------

just like raphnet.net's project this uses the software level USB driver from Objective Development, v-usb.
see http://www.obdev.at/products/avrusb/index.html for more



3) Compilation, Installation and Fuse bits
-------------------------------------------

To compile the code you will need avr-gcc and avr-libc either in Linux or Windows. For Windows you can use WinAVR or Cygwin.
simply open a terminal/Command prompt in the directory of the source and execute 'make'.

after the source is compiled you need to put the firmware on the chip. you can do this in any way you like. 
i have used tinyusb with avrdude.
after this, the fuse bits need to be set for this to work.

verify the makefile for your chip to verify the following numbers are correct!
ATMega8 : 
-------------
Low byte : 0x9F
High byte : 0xC9

ATMega168 : 
-------------
Low Byte : 0xD7
High Byte : 0xD5
Extended Byte : 0x01




4) vendor id/product id pair
-------------------------------------------

Just as mentioned in the source and readme of raphnet.net , please change the vendor/product ID when using this in a different project.
you can use one for free from obdev's USB-IDs-for-free.txt !
