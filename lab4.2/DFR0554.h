/*********************************************************************
*
* Copyright (C) 2022 David C. Harrison. All right reserved.
*
* You may not use, distribute, publish, or modify this code without
* the express written permission of the copyright holder.
*
*********************************************************************/
/*********************************************************************
* DO NOT MODIFY THIS FILE
*********************************************************************/

enum color { White, Red, Green, Blue };
enum direction { Left, Right };
enum mode { Off, On };

// Connects PSoC 6 I2C MASTER with associated CONTEXT to a connected DFR0554
// 16x2 backlight RGB LCD. Text direction will be fixed left-to-right.
// Initial settings:
// Display On
// Color White
// Blink Off
// Cursor Off
// Returns FALSE on error, TRUE otherwise.
bool LCD_Start(CySCB_Type *master, cy_stc_scb_i2c_context_t *context);

// Turns the display on if MODE is On, turns display off otherwise.
// Returns FALSE on error, TRUE otherwise.
bool LCD_Display(enum mode);

// Sets the display to blink if MODE is On, non-blinking otherwise.
// Returns FALSE on error, TRUE otherwise.
bool LCD_Blink(enum mode);

// Shows the cursor if MODE is On, hides it otherwise.
// Returns FALSE on error, TRUE otherwise.
bool LCD_Cursor(enum mode);

// Scrolls display on char left if DIRECTION is Left, one char right otherwise.
// Returns FALSE on error, TRUE otherwise.
bool LCD_Scroll(enum direction);

// Sets display color if COLOR is White, Red, Green, or Blue, no-op otherwise.
// Returns FALSE on error, TRUE otherwise.
bool LCD_SetColor(enum color);

// Sets display color according to RED, GREEN, BLUE.
// Example: LCD_SetRGB(0, 255, 255) for cyan.
// Returns FALSE on error, TRUE otherwise.
bool LCD_SetRGB(uint8_t red, uint8_t green, uint8_t blue);

// Positions the cursor at COL and ROW where 0<=COL<=15 and 0<=ROW<=1
// Returns FALSE on error, TRUE otherwise.
bool LCD_SetCursor(uint8_t col, uint8_t row);

// Clears the display.
// Returns FALSE on error, TRUE otherwise.
bool LCD_Clear();

// Prints all characters from zero terminated string STR.
// Returns FALSE on error, TRUE otherwise.
bool LCD_Print(char *str);
