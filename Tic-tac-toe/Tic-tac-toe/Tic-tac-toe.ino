/* Tic-tac-toe on the Watchy
   Made by SwiggityCricket 6/15/2020
   Use all code for whatever you want.
   It doesn't really have anything specific to do with Watchy other than the button inits and display settings,
   but this is a good base to start from for "games" and whatnot. At least it is for me! Please note this absolutely
   destroys battery because the processor is looping forever, even though the display isn't refreshing. Could fix 
   this by putting the game logic in the button interrupts and putting it to sleep in between.
   L2: cycle spaces
   R1: reset and clear the screen
   R2: place mark

   NOTES ON HOW FAST THE SCREEN UPDATES:
   I worked quite hard to figure out the source of the screen's slowness. Total time it takes to call display.display(true)
   is about 500ms. This is literally because the GxEPD2_EPD::_waitWhileBusy funtion is waiting for the display to come back
   and say "yup, done displaying stuff." You can make the game feel significantly faster by changing the while(1) to while(0)
   in that function. This allows your loop to continue while the screen is refreshing, BUT can introduce weird artifacts
   where the screen updates as you are sending it info occasionally.
   ***
   Line 103 in the libraries\GxEPD2\src\GxEPD2_EPD.cpp file. 
   ***
   Because the SPI data transfer takes mere milliseconds compared to the gargantuan 430ms it takes the display to refresh, 
   you can actually press buttons faster than the display can update. But that's not necessarily a bad thing, you just have 
   to be aware of it. 
   
   Why 430ms when I said 500ms before:
   For a while I thought the slowness was due to the GxEPD2 library's digital_write calls for every byte of SPI data, but
   removing those only saved ~70ms bringing the total time to call display.display(true) to 430ms. The screen really is just 
   that slow.
*/
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>

// Display setup variables
const int CS = 5;
const int DC = 10;
const int RESET = 9;
const int PIN_BUSY = 19;

GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(CS, DC, RESET, PIN_BUSY));

// Game variables
int current_space = 0;
String turn = "X";
String board[9] = {"", "", "", "", "", "", "", "", ""};
bool can_draw = false;

// Button setup
#define debounce 100 // time to wait for debounce in ms
struct Btn {
  const uint8_t pin;
  bool pressed;
  unsigned long debounce_timer;
};

Btn L1 = {25, false, 0};
Btn L2 = {26, false, 0};
Btn R1 = {32, false, 0};
Btn R2 = {4, false, 0};

void IRAM_ATTR handleL1() {
  if (L1.debounce_timer == 0) {
    L1.pressed = true;
    L1.debounce_timer = millis();
  }
}
void IRAM_ATTR handleL2() {
  if (L2.debounce_timer == 0) {
    L2.pressed = true;
    L2.debounce_timer = millis();
  }
}
void IRAM_ATTR handleR1() {
  if (R1.debounce_timer == 0) {
    R1.pressed = true;
    R1.debounce_timer = millis();
  }
}
void IRAM_ATTR handleR2() {
  if (R2.debounce_timer == 0) {
    R2.pressed = true;
    R2.debounce_timer = millis();
  }
}

void plotLineLow(int x0, int y0, int x1, int y1, int col) {
  //Using Bresenham's line algorithm for integer arithmetic: https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
  int dx = x1 - x0;
  int dy = y1 - y0;
  int yi = 1;
  if (dy < 0) {
    yi = -1;
    dy = -dy;
  }
  int D = (2 * dy) - dx;
  int y = y0;

  for (int x = x0; x < x1; x++) {
    display.drawPixel(x, y, col);
    if (D > 0) {
      y = y + yi;
      D = D + (2 * (dy - dx));
    } else {
      D = D + 2 * dy;
    }
  }
}

void plotLineHigh(int x0, int y0, int x1, int y1, int col) {
  //Using Bresenham's line algorithm for integer arithmetic: https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
  int dx = x1 - x0;
  int dy = y1 - y0;
  int xi = 1;

  if (dx < 0) {
    xi = -1;
    dx = -dx;
  }

  int D = (2 * dx) - dy;
  int x = x0;

  for (int y = y0; y < y1; y++) {
    display.drawPixel(x, y, col);
    if (D > 0) {
      x = x + xi;
      D = D + (2 * (dx - dy));
    } else {
      D = D + 2 * dx;
    }
  }
}

void drawLine(int x0, int y0, int x1, int y1, int col) {
  // Iterate along a line, drawing pixels as we go.
  // Using Bresenham's line algorithm for integer arithmetic: https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
  if (abs(y1 - y0) < abs(x1 - x0)) {
    if (x0 > x1) {
      plotLineLow(x1, y1, x0, y0, col);
    } else {
      plotLineLow(x0, y0, x1, y1, col);
    }
  } else {
    if (y0 > y1) {
      plotLineHigh(x1, y1, x0, y0, col);
    } else {
      plotLineHigh(x0, y0, x1, y1, col);
    }
  }
}

void drawBoard() {
  int black = GxEPD_BLACK;
  // board setup variables
  int screen_size = 200;
  int border = 25;
  int box_size = round((screen_size - border * 2) / 3);

  display.fillScreen(GxEPD_WHITE);
  
  // horizontal lines
  drawLine(border, border + box_size, screen_size - border, border + box_size, black);
  drawLine(border, border + box_size * 2, screen_size - border, border + box_size * 2, black);

  // vertical lines
  drawLine(border + box_size, border, border + box_size, screen_size - border, black);
  drawLine(border + box_size * 2, border, border + box_size * 2, screen_size - border, black);
  
  // spaces and marks
  display.setFont(&FreeMonoBold12pt7b);
  display.setTextColor(GxEPD_BLACK);
  int font_offset = 5;
  for (int i = 0; i < 9; i++) {
    int ypos = floor(i / 3);
    int xpos = i - 3 * (ypos);
    int x = border + box_size / 2 + xpos * box_size;
    int y = border + box_size / 2 + ypos * box_size;
    // draw cursor
    if (current_space == i) {
      drawLine(x - box_size / 3, y - box_size / 3, x + box_size / 3, y - box_size / 3, black);
      drawLine(x - box_size / 3, y + box_size / 3, x + box_size / 3, y + box_size / 3, black);
      drawLine(x - box_size / 3, y - box_size / 3, x - box_size / 3, y + box_size / 3, black);
      drawLine(x + box_size / 3, y - box_size / 3, x + box_size / 3, y + box_size / 3, black);
    }
    display.setCursor(x - font_offset, y + font_offset);
    display.print(board[i]);
  }

  // info text
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(0, 10);
  display.println("Tic-tac-toe Turn:" + turn);
  display.setCursor(10, 190);
  display.println("Move");
  display.setCursor(140, 190);
  display.println("Place");
}

void setup() {
  display.init(0, true); // 0 is default SPI baud rate, probably 9600
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);

  pinMode(L2.pin, INPUT);
  pinMode(R1.pin, INPUT);
  pinMode(L2.pin, INPUT);
  pinMode(L1.pin, INPUT);

  // please note there is no debounce 
  attachInterrupt(L2.pin, handleL2, HIGH);
  attachInterrupt(L1.pin, handleL1, HIGH);
  attachInterrupt(R1.pin, handleR1, HIGH);
  attachInterrupt(R2.pin, handleR2, HIGH);

  drawBoard();
  display.display(false);

}


void loop() {
  bool refresh = false;
  unsigned long millisRef = millis();
  can_draw = false;

  // handle button debounce
  
  if (L1.debounce_timer > 0 && millisRef - L1.debounce_timer >= debounce) {
      L1.debounce_timer = 0;
  }
  if (L2.debounce_timer > 0 && millisRef - L2.debounce_timer >= debounce) {
      L2.debounce_timer = 0;
  }
  if (R1.debounce_timer > 0 && millisRef - R1.debounce_timer >= debounce) {
      R1.debounce_timer = 0;
  }
  if (R2.debounce_timer > 0 && millisRef - R2.debounce_timer >= debounce) {
      R2.debounce_timer = 0;
  }
  
  // handle button presses
  
  if (L1.pressed) {
    L1.pressed = false;
  }
  
  if (L2.pressed) {
    // Bottom left button iterates through the board
    current_space++;
    if (current_space > 8) {
      current_space = 0;
    }
    L2.pressed = false;
    can_draw = true;
  }

  if (R1.pressed) {
    // Top right button clears the screen and resets the game
    for (int i = 0; i < 9; i++) {
      board[i] = "";
    }
    turn = "X";
    current_space = 0;
    refresh = true;
    R1.pressed = false;
  }

  if (R2.pressed) {
    // Set the current space
    if (board[current_space] == "") {
      board[current_space] = turn;
      if (turn == "X") {
        turn = "O";
      } else {
        turn = "X";
      }
    }
    R2.pressed = false;
    can_draw = true;
  }

  // Not exactly sure what these do when you aren't initializing the screen but they make the screen incredibly slow. 
  // Commenting them out makes it way faster but at the cost of more ghost artifacts. Looks fine to me tho.
  //display.init(0, false);
  //display.setFullWindow();

  // Refresh if needed and draw the screen. Note that the display doesn't update unless a button is pressed.
  
  if (refresh) {
    display.fillScreen(GxEPD_BLACK);
    display.display(true); 
    can_draw = true;
    refresh = false;
  }

  drawBoard();
  
  if (can_draw) {
    display.display(true);
  }
}
