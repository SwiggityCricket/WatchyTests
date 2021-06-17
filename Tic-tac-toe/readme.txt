Tic-tac-toe on the Watchy
Made by SwiggityCricket 6/16/2020

Use all code for whatever you want.
It doesn't really have anything specific to do with Watchy other than the button inits and display settings, but this is a good base to start from for "games" and whatnot. At least it is for me! Please note this absolutely destroys battery because the processor is looping forever, even though the display isn't refreshing. Could fix this by putting the game logic in the button interrupts and putting it to sleep in between.
   L2: cycle spaces
   R1: reset and clear the screen
   R2: place mark

NOTES ON HOW FAST THE SCREEN UPDATES:
I worked quite hard to figure out the source of the screen's slowness. Total time it takes to call display.display(true) is about 500ms. This is literally because the GxEPD2_EPD::_waitWhileBusy funtion is waiting for the display to come back and say "yup, done displaying stuff." You can make the game feel significantly faster by changing the while(1) to while(0) in that function. This allows your loop to continue while the screen is refreshing, BUT can introduce weird artifacts where the screen updates as you are sending it info occasionally.
   ***
   Line 103 in the libraries\GxEPD2\src\GxEPD2_EPD.cpp file. 
   ***
Because the SPI data transfer takes mere milliseconds compared to the gargantuan 430ms it takes the display to refresh, you can actually press buttons faster than the display can update. But that's not necessarily a bad thing, you just have to be aware of it. 
   
Why 430ms when I said 500ms before:
For a while I thought the slowness was due to the GxEPD2 library's digital_write calls for every byte of SPI data, but removing those only saved ~70ms bringing the total time to call display.display(true) to 430ms. The screen really is just that slow.