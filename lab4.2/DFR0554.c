/*
 * C file for LCD display API
 * REFERENCES:
 * 		https://github.com/DFRobot/DFRobot_RGBLCD/
 * 		https://iotexpert.com/i2c-detect-with-psoc-6/
 */

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "DFR0554.h"

#define LCD_ADDRESS 0x3E
#define RGB_ADDRESS 0x60
#define LCD_FUNCTION_SET 0b00101000
#define LCD_ENTRY_MODE_SET 0b00000110 // last two bits indicate increment mode and shift on
#define LCD_DISPLAY_ON 0b00001100 // last three bits indicate display, cursor, blink
#define LCD_DISPLAY_OFF 0b00001000
#define LCD_DISPLAY_BLINK_MASK 0b00000001
#define LCD_DISPLAY_CLEAR 0b00000001
#define LCD_SCROLL_RIGHT 0b00011100
#define LCD_SCROLL_LEFT 0b00011000
#define DATA_WRITE 0b10000000

#define REG_MODE1       0x00
#define REG_MODE2       0x01
#define REG_OUTPUT      0x08

#define REG_RED         0x04
#define REG_GREEN       0x03
#define REG_BLUE        0x02

CySCB_Type *master = I2C_HW;
cy_stc_scb_i2c_context_t *global_context;
uint8_t cursor_blink;

// Connects PSoC 6 I2C MASTER with associated CONTEXT to a connected DFR0554
// 16x2 backlight RGB LCD. Text direction will be fixed left-to-right.
// Initial settings:
// Display On
// Color White
// Blink Off
// Cursor Off
// Returns FALSE on error, TRUE otherwise.
bool LCD_Start(CySCB_Type *master, cy_stc_scb_i2c_context_t *context) {
	// set global_context
	global_context = context;

	printf("\r\nScanning...\r\n");
	uint32_t result;
	for (uint32_t address = 0; address < 0x80; address++)
	{
		result = Cy_SCB_I2C_MasterSendStart(master,address,CY_SCB_I2C_WRITE_XFER,10,global_context);
		if (result == CY_SCB_I2C_SUCCESS) {
			printf("	Found: %02X\r\n",(unsigned int)address);
		}
		Cy_SCB_I2C_MasterSendStop(master,0,context);
	}
	printf("Finished Scan\r\n");


	printf("Starting initialization sequence...\r\n");
	// initialize LCD
	// result gets or'd with the output of each call, so we can check if any were unsuccessful
	result = 0;
	result |= Cy_SCB_I2C_MasterSendStart(master,LCD_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);

	result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_FUNCTION_SET, 10, global_context);
	Cy_SysLib_Delay(5);

	result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_DISPLAY_ON, 10, global_context);
	Cy_SysLib_Delay(5);

	result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_DISPLAY_CLEAR, 10, global_context);
	Cy_SysLib_Delay(5);

	result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_ENTRY_MODE_SET, 10, global_context);
	Cy_SysLib_Delay(5);

	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	if (result) { // one of the calls didn't return success
		return false;
	}
	printf("	LCD initialized\r\n");
	cursor_blink = 0b00;
	result = 0;

	// initialize RGB
	//setReg(regmode1, 0)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_MODE1, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, 0x00, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	//setReg(regoutput, 0xff)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_OUTPUT, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, 0xff, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	//setreg(regmode2, 0x20)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_MODE2, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, 0x20, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	//setReg(RED, ff)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_RED, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, 0xff, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	//setReg(GREEN, ff)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_GREEN, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, 0xff, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	//setReg(BLUE, ff)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_BLUE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, 0xff, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	if (result) {
		return false;
	}

	printf("	RGB backlight initialized\r\n");
	printf("Finished Successfully\r\n");
	return true;
}

// Turns the display on if MODE is On, turns display off otherwise.
// Returns FALSE on error, TRUE otherwise.
bool LCD_Display(enum mode MODE) {
	if ((MODE != On) && (MODE != Off)) {
		return false;
	}

	// initialize communication
	uint32_t result = 0;
	result |= Cy_SCB_I2C_MasterSendStart(master,LCD_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);

	result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_FUNCTION_SET, 10, global_context);
	Cy_SysLib_Delay(5);

	// turn on or off blink mode
	if (MODE == On) {
		result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
		Cy_SysLib_Delay(5);
		result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_DISPLAY_ON | cursor_blink, 10, global_context);
		Cy_SysLib_Delay(5);
	}
	else if (MODE == Off) {
		result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
		Cy_SysLib_Delay(5);
		result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_DISPLAY_OFF | cursor_blink, 10, global_context);
		Cy_SysLib_Delay(5);
	}
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	if (result) {
		return false;
	}
	return true;
}

// Sets the display to blink if MODE is On, non-blinking otherwise.
// Returns FALSE on error, TRUE otherwise.
bool LCD_Blink(enum mode MODE) {
	if ((MODE != On) && (MODE != Off)) {
		return false;
	}

	// initialize communication
	uint32_t result = 0;
	result |= Cy_SCB_I2C_MasterSendStart(master,LCD_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);

	result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_FUNCTION_SET, 10, global_context);
	Cy_SysLib_Delay(5);

	// turn on or off blink mode
	if (MODE == On) { // turn on display with blink mode
		cursor_blink |= 0b01; // turn on blinking in global cursor_blink variable
		result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
		Cy_SysLib_Delay(5);
		result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_DISPLAY_ON | cursor_blink, 10, global_context);
		Cy_SysLib_Delay(5);
	}
	else if (MODE == Off) { // turn on display without blink mode
		cursor_blink &= 0b10; // turn off blinking in global cursor_blink variable
		result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
		Cy_SysLib_Delay(5);
		result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_DISPLAY_ON | cursor_blink, 10, global_context);
		Cy_SysLib_Delay(5);
	}
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	if (result) {
		return false;
	}
	return true;
}

// Shows the cursor if MODE is On, hides it otherwise.
// Returns FALSE on error, TRUE otherwise.
bool LCD_Cursor(enum mode MODE) {
	if ((MODE != On) && (MODE != Off)) {
		return false;
	}

	// initialize communication
	uint32_t result = 0;
	result |= Cy_SCB_I2C_MasterSendStart(master,LCD_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);

	result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_FUNCTION_SET, 10, global_context);
	Cy_SysLib_Delay(5);

	// turn on or off cursor mode
	if (MODE == On) { // turn on display with cursor mode
		cursor_blink |= 0b10; // turn on cursor in global cursor_blink variable
		result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
		Cy_SysLib_Delay(5);
		result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_DISPLAY_ON | cursor_blink, 10, global_context);
		Cy_SysLib_Delay(5);
	}
	else if (MODE == Off) { // turn on display without cursor mode
		cursor_blink &= 0b01; // turn off cursor in global cursor_blink variable
		result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
		Cy_SysLib_Delay(5);
		result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_DISPLAY_ON | cursor_blink, 10, global_context);
		Cy_SysLib_Delay(5);
	}
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	if (result) {
		return false;
	}
	return true;
}

// Scrolls display on char left if DIRECTION is Left, one char right otherwise.
// Returns FALSE on error, TRUE otherwise.
bool LCD_Scroll(enum direction DIRECTION) {
	if ((DIRECTION != Left) && (DIRECTION != Right)) {
		return false;
	}

	// initialize communication
	uint32_t result = 0;
	result |= Cy_SCB_I2C_MasterSendStart(master,LCD_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);

	result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_FUNCTION_SET, 10, global_context);
	Cy_SysLib_Delay(5);

	// scroll left or right
	if (DIRECTION == Left) {
		result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
		Cy_SysLib_Delay(5);
		result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_SCROLL_LEFT, 10, global_context);
		Cy_SysLib_Delay(5);
	}
	else if (DIRECTION == Right) {
		result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
		Cy_SysLib_Delay(5);
		result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_SCROLL_RIGHT, 10, global_context);
		Cy_SysLib_Delay(5);
	}
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	if (result) {
		return false;
	}
	return true;
}

// Sets display color if COLOR is White, Red, Green, or Blue, no-op otherwise.
// Returns FALSE on error, TRUE otherwise.
bool LCD_SetColor(enum color COLOR) {

	// set colors
	uint8_t r;
	uint8_t g;
	uint8_t b;
	if (COLOR == White) {
		r = g = b = 0xff;
	}
	else if (COLOR == Red) {
		r = 0xff;
		g = b = 0x00;
	}
	else if (COLOR == Green) {
		g = 0xff;
		r = b = 0x00;
	}
	else if (COLOR == Blue) {
		b = 0xff;
		g = r = 0x00;
	}
	else {
		return false;
	}

	// initialize Communication
	uint32_t result = 0;
	//setReg(regmode1, 0)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_MODE1, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, 0x00, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	//setReg(regoutput, 0xff)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_OUTPUT, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, 0xff, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	//setreg(regmode2, 0x20)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_MODE2, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, 0x20, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	// set colors
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_RED, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, r, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	//setReg(GREEN, ff)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_GREEN, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, g, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	//setReg(BLUE, ff)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_BLUE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, b, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	if (result) {
		return false;
	}
	return true;

}

// Sets display color according to RED, GREEN, BLUE.
// Example: LCD_SetRGB(0, 255, 255) for cyan.
// Returns FALSE on error, TRUE otherwise.
bool LCD_SetRGB(uint8_t red, uint8_t green, uint8_t blue) {
	if (red > 255 || green > 255 || blue > 255) {
		return false;
	}

	// initialize Communication
	uint32_t result = 0;
	//setReg(regmode1, 0)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_MODE1, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, 0x00, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	//setReg(regoutput, 0xff)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_OUTPUT, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, 0xff, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	//setreg(regmode2, 0x20)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_MODE2, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, 0x20, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	// set colors
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_RED, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, red, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	//setReg(GREEN, ff)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_GREEN, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, green, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	//setReg(BLUE, ff)
	result |= Cy_SCB_I2C_MasterSendStart(master,RGB_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, REG_BLUE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, blue, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	if (result) {
		return false;
	}
	return true;
}

// Positions the cursor at COL and ROW where 0<=COL<=15 and 0<=ROW<=1
// Returns FALSE on error, TRUE otherwise.
bool LCD_SetCursor(uint8_t col, uint8_t row) {
	// make sure row and column are within correct bounds
	if ((col < 0) || (col > 15)) {
		return false;
	}
	if ((row < 0) || (row > 1)){
		return false;
	}

	// set column depending on if we are in row 1 or row 2
	uint8_t position;
	if (row == 0) {
		position = col | 0x80;
	}
	else {
		position = col | 0xc0;
	}

	// initialize communication
	uint32_t result = 0;
	result |= Cy_SCB_I2C_MasterSendStart(master,LCD_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);

	result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_FUNCTION_SET, 10, global_context);
	Cy_SysLib_Delay(5);

	// write the new position to the screen
	result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, position, 10, global_context);
	Cy_SysLib_Delay(5);

	// stop and handle return
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	if (result) {
		return false;
	}
	return true;
}

// Clears the display.
// Returns FALSE on error, TRUE otherwise.
bool LCD_Clear() {
	// initialize communication
	uint32_t result = 0;
	result |= Cy_SCB_I2C_MasterSendStart(master,LCD_ADDRESS,CY_SCB_I2C_WRITE_XFER,10,global_context);
	Cy_SysLib_Delay(50);

	result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_FUNCTION_SET, 10, global_context);
	Cy_SysLib_Delay(5);

	// write clear code to the screen
	result |= Cy_SCB_I2C_MasterWriteByte(master, DATA_WRITE, 10, global_context);
	Cy_SysLib_Delay(5);
	result |= Cy_SCB_I2C_MasterWriteByte(master, LCD_DISPLAY_CLEAR, 10, global_context);
	Cy_SysLib_Delay(5);

	// stop and handle return
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	if (result) {
		return false;
	}
	return true;
}

// Prints all characters from zero terminated string STR.
// Returns FALSE on error, TRUE otherwise.
bool LCD_Print(char *str) {

	// initialize communication
	uint32_t result = 0;
	result |= Cy_SCB_I2C_MasterSendStart(master,LCD_ADDRESS,CY_SCB_I2C_WRITE_XFER ,10,global_context);
	Cy_SysLib_Delay(50);
	result |= Cy_SCB_I2C_MasterWriteByte(master, 0x40, 10, global_context); // we are going to write to CGRAM
	Cy_SysLib_Delay(5);


	// begin writing string
	int i = 0;
	while (str[i]) {
		// print character to screen
		result |= Cy_SCB_I2C_MasterWriteByte(master, str[i], 10, global_context); // write char
		Cy_SysLib_Delay(5);
		i++;
	}

	// stop and handle return
	result |= Cy_SCB_I2C_MasterSendStop(master,0,global_context);

	if (result) {
		return false;
	}
	return true;
}

