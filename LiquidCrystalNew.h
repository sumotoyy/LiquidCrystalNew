/*---------------------------------------------------------------------------------------
A better universal alphanumeric LCD library.
This library it's basically LiquidCrystal440 http://code.google.com/p/liquidcrystal440/
Modified in many parts and I added the ability to drive LCD's with an external GPIO
(MCP23s16 for SPI or MCP23017 for I2C) that allow to drive even 2 chip big LCD displays 
with 2 or 3 wires. Apart that the main differences with the classic LiquidCrystal is
the ability to drive big displays with 2 or more chips a correct cursor handle and better
overall timing so it should be faster.
-----------------------------------------------------------------------------------------*/

#ifndef LiquidCrystalNew_h
#define LiquidCrystalNew_h

#include <inttypes.h>
#include "Print.h"
#include "Arduino.h"

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00
#define LCD_CURSORS_MASK 0xfc

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
//#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00
#define LCD_Right 0
#define LCD_Left 1

#define DELAYPERCHAR 320

//pin mapped to the MCP23xx8
#define LCDPIN_EN 	2  	
#define LCDPIN_EN2  0   //Not used for 1 chip LCD's
#define LCDPIN_RS 	1  	
#define LCDPIN_D4 	3 	
#define LCDPIN_D5 	4	
#define LCDPIN_D6 	5	
#define LCDPIN_D7 	6	
#define LCDPIN_LD 	7	//Background Led (use a transistor!!!)

class LiquidCrystalNew : public Print {
public:

    LiquidCrystalNew(uint8_t rs,uint8_t en1,uint8_t en2,uint8_t d0,uint8_t d1,uint8_t d2,uint8_t d3);
	LiquidCrystalNew(uint8_t rs,uint8_t en1,uint8_t d0,uint8_t d1,uint8_t d2,uint8_t d3);
	LiquidCrystalNew(uint8_t cs,uint8_t chip=0,uint8_t adrs=0x20);//use SPI
	LiquidCrystalNew(uint8_t chip=0,uint8_t adrs=0x20);//use I2C

// tipical commands	
	void 			init(uint8_t adrs,uint8_t cs,uint8_t rs,uint8_t rw,uint8_t en1,uint8_t en2,uint8_t d0,uint8_t d1,uint8_t d2,uint8_t d3);
	void 			begin(uint8_t cols, uint8_t rows, uint8_t charsize = LCD_5x8DOTS);
	void 			clear();
	void 			home();
	void 			noDisplay();
	void 			display();
	void 			noBlink();
	void 			blink();
	void 			noCursor();
	void 			cursor();
	void 			scrollDisplayLeft();
	void 			scrollDisplayRight();
	void 			leftToRight();
	void 			rightToLeft();
	void 			autoscroll();
	void 			noAutoscroll();
	void			backlight(uint8_t val);
	
	void 			createChar(uint8_t, uint8_t[]);
	void 			setCursor(uint8_t, uint8_t);
    virtual size_t 	write(uint8_t);
	
	uint8_t			getCursorCol(void);
	uint8_t			getCursorRow(void);
	inline void 	command(uint8_t value)  {send(value, LOW);}
	void 			commandBoth(uint8_t);
	inline 			LiquidCrystal440& operator() (uint8_t x, uint8_t y) {setCursor(x,y); return *this;}  //use along w Streaming.h to support: lcd(col,line)<<"a="<<a;
	uint8_t 		numlines;
	uint8_t 		numcols;
	
protected:
//low level commands
	void 			send(uint8_t, uint8_t);
	void 			write4bits(uint8_t);
	void 			initChip(uint8_t dotsize, uint8_t enPin);
	inline void 	delayPerHome(void){delayMicroseconds(2900);}
	
	void 			setDataMode(uint8_t mode);
	void 			pulseEnable(uint8_t enPin);
	void 			writeByte(byte cmd,uint8_t value);
	void 			writeGpio(uint8_t value);
	void 			startSend();
	void 			endSend();
	/* void			pulseEnable(uint8_t data,uint8_t chip); */
	
	uint8_t 		_rs_pin;	// LOW: command.  HIGH: character.
	uint8_t 		_en1; 		// activated by a HIGH pulse.
	uint8_t 		_en2; 		// activated by a HIGH pulse.
	uint8_t 		_chip;
	int8_t 			_scroll_count;
	int8_t 			_x, _y,_setCursFlag;
	uint8_t 		_direction;
	
	uint8_t 		_data_pins[4];
	uint8_t 		_numcols;
	uint8_t 		_numlines;
	uint8_t 		row_offsets[4];
	
	uint8_t 		_displaycontrol;   //display on/off, cursor on/off, blink on/off
	uint8_t 		_displaymode;      //text direction	
	//for use with SPI
	uint8_t			_cs;				//used only in SPI, the CS pin
	uint8_t			_adrs;				//SPI address or I2C address
	
	uint8_t			_driveType;			//0:direct/1:SPI/2:I2C/3:SR
	uint8_t			_backLight;			//0:off/1:ON
	
	byte			_theData;			//8 bit of the GPIO chip or SR, not used in direct
	
	//uint8_t 		_currCol;
	//uint8_t 		_currRow;
};

#endif
