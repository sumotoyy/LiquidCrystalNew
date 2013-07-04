/*---------------------------------------------------------------------------------------
A better universal alphanumeric LCD library.
This library it's basically LiquidCrystal440 http://code.google.com/p/liquidcrystal440/
Modified in many parts and I added the ability to drive LCD's with an external GPIO
(MCP23s16 for SPI or MCP23017 for I2C) that allow to drive even 2 chip big LCD displays 
with 2 or 3 wires. Apart that the main differences with the classic LiquidCrystal is
the ability to drive big displays with 2 or more chips a correct cursor handle and better
overall timing so it should be faster.
-----------------------------------------------------------------------------------------*/




#include "Arduino.h"
#include "LiquidCrystalNew.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <../SPI/SPI.h>



//2 chip hardwired
LiquidCrystalNew::LiquidCrystalNew(uint8_t rs,uint8_t en1,uint8_t en2,uint8_t d0,uint8_t d1,uint8_t d2,uint8_t d3){
	init(255,255,rs,255,en1,en2,d0,d1,d2,d3);
}

//1 chip hardwired
LiquidCrystalNew::LiquidCrystalNew(uint8_t rs,uint8_t en1,uint8_t d0,uint8_t d1,uint8_t d2,uint8_t d3){
	init(255,255,rs,255,en1,255,d0,d1,d2,d3);
}

//1/2 chip with SPI GPIO (3 wire)
LiquidCrystalNew::LiquidCrystalNew(uint8_t cs,uint8_t chip,uint8_t adrs){
	uint8_t e2;
	if (chip == 0){
		e2 = 255;
	} else {
		//e2 = LCDPIN_EN2;
		e2 = (1 << LCDPIN_EN2);
	}
	//init(adrs,cs,LCDPIN_RS,255,LCDPIN_EN,e2,LCDPIN_D4,LCDPIN_D5,LCDPIN_D6,LCDPIN_D7);
	init(adrs,cs,(1 << LCDPIN_RS),255,(1 << LCDPIN_EN),e2,(1 << LCDPIN_D4),(1 << LCDPIN_D5),(1 << LCDPIN_D6),(1 << LCDPIN_D7));
}

//1/2 chip with I2C GPIO (2 wire)
LiquidCrystalNew::LiquidCrystalNew(uint8_t chip,uint8_t adrs){
	uint8_t e2;
	if (chip == 0){
		e2 = 255;
	} else {
		//e2 = LCDPIN_EN2;
		e2 = (1 << LCDPIN_EN2);
	}
	//init(adrs,255,LCDPIN_RS,255,LCDPIN_EN,e2,LCDPIN_D4,LCDPIN_D5,LCDPIN_D6,LCDPIN_D7);
	init(adrs,255,(1 << LCDPIN_RS),255,(1 << LCDPIN_EN),e2,(1 << LCDPIN_D4),(1 << LCDPIN_D5),(1 << LCDPIN_D6),(1 << LCDPIN_D7));
}


/*
set variables
*/
void LiquidCrystalNew::init(uint8_t adrs,uint8_t cs,uint8_t rs,uint8_t rw,uint8_t en1,uint8_t en2,uint8_t d0,uint8_t d1,uint8_t d2,uint8_t d3){

	_chip = 0;
	
	_cs = cs;
	_adrs = adrs;
	
	_scroll_count = 0;      //to fix the bug if we scroll and then setCursor w/o home() or clear()
	_x = 0;
	_y = 0;
	
	_setCursFlag = 0;
	_direction = LCD_Right;
	
	_data_pins[0] = d0;
	_data_pins[1] = d1;
	_data_pins[2] = d2;
	_data_pins[3] = d3; 
	_en1 = en1;
	_en2 = en2;
	_rs_pin = rs;
	if (_cs != 255 && _adrs != 255){ //SPI
		_driveType = 1;
/*  	_data_pins[0] = (1 << d0);
		_data_pins[1] = (1 << d1);
		_data_pins[2] = (1 << d2);
		_data_pins[3] = (1 << d3);  
		_en1 = (1 << en1);
		_en2 = (1 << en2);
		_rs_pin = (1 << rs); */
	} else if (_adrs != 255 && _cs == 255){ //I2C
		_driveType = 2;
		// _data_pins[0] = (1 << d0);
		// _data_pins[1] = (1 << d1);
		// _data_pins[2] = (1 << d2);
		// _data_pins[3] = (1 << d3); 
		// _en1 = (1 << en1);
		// _en2 = (1 << en2);
		// _rs_pin = (1 << rs);
	} else { //hardwired
		_driveType = 0;
		// _data_pins[0] = d0;
		// _data_pins[1] = d1;
		// _data_pins[2] = d2;
		// _data_pins[3] = d3; 
		// _en1 = en1;
		// _en2 = en2;
		// _rs_pin = rs;
	}
	
	row_offsets[0] = 00;   // DDRAM addresses inside the HD44780 are strange: 0-nColumns-1 on line 0
	row_offsets[1] = 0x40; // 64-(63+nColumns) for line 1
	row_offsets[2] = 0x14; // 20- (19+nColumns) for line 2 --- NOTHING FROM 40-63 !
	row_offsets[3] = 0x54; // 84 - (83+nColumns) for line 3  -- so 80 characters tops out at #103 !
	_theData = 0b00000000;
}

/*
set pins and protocols
*/
void LiquidCrystalNew::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
	uint8_t i;
	switch(_driveType){
		case 0://direct
			for (i=0;i<4;i++){
				pinMode(_data_pins[i],OUTPUT);
			}
			pinMode(_rs_pin,OUTPUT);
			pinMode(_en1,OUTPUT);
			digitalWrite(_en1, LOW);
			if (_en2 != 255) {
				pinMode(_en2,OUTPUT);  //4X40 LCD
				digitalWrite(_en2, LOW);
			}
			for (i=0;i<4;i++){
				digitalWrite(_data_pins[i],LOW);
			}
		//Serial.println("direct");	
		break;
		case 1://SPI
			SPI.begin();
#if defined(__MK20DX128__)
			SPI_CLOCK_DIV4;
#elif defined(__arm__)//dunnoyet!
			SPI_CLOCK_DIV4;
#endif
			pinMode(_cs, OUTPUT); //set data pin modes
#if defined(__FASTSPI)
			digitalWriteFast(_cs, HIGH);
#else
			digitalWrite(_cs, HIGH);
#endif
			delay(100);
			if (_adrs != 0){
				writeByte(0x05,0b00101000);//HAEN on (IOCON)
			} else {
				writeByte(0x05,0b00100000);//use dedicated cs
			}
			writeByte(0x00,0x00);//set as out (IODIR)
			_theData = 0b00000000;
			writeByte(0x09,_theData);//write all low to GPIO
			 //Serial.println("SPI");
		break;
		case 2://I2C
			//Serial.println("I2C");
		break;
	}
	numcols = _numcols = cols;    //there is an implied lack of trust; the private version can't be munged up by the user.
	numlines = _numlines = lines;
	row_offsets[2] = cols + row_offsets[0];  //should autoadjust for 16/20 or whatever columns now
	row_offsets[3] = cols + row_offsets[1];
	initChip(dotsize,_en1);
	//manage second chip if exists
	if (_en2 != 255) {
		row_offsets[2] = 0;
		row_offsets[3] = 0x40; //each line gets its own little 40 char section of DDRAM--would be fine if there were a 4x32, I suppose
		_chip = 2;
		initChip(dotsize,_en2);//initialize the second HD44780 chip
	}
	backlight(1);
}

/*
phisically initialize the display
*/
void LiquidCrystalNew::initChip(uint8_t dotsize, uint8_t enPin) {  
	uint8_t	displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
	if (_numlines > 1) displayfunction |= LCD_2LINE;
	// for some 1 line displays you can select a 10 pixel high font
	if ((dotsize != 0) && (_numlines == 1)) displayfunction |= LCD_5x10DOTS;

	for (uint8_t i=0;i<18;i++) {
		delayMicroseconds(7500); //I don't think I can adequately test this number; it will depend a little on which Arduino or clone you have and probably
		//could also vary with the power source applied to that board. The way to test is really to load your program, remove power
		//and then reapply power so that the program starts up as power is applied. If anyone finds they need a larger number please
		//let me know: raine001 at tc dot umn dot edu.  delayMicroseconds takes an unsigned int so 65535 is the max. delay doesn't work because of
		//issue 129.
	}

	// Now we pull both RS and R/W low to begin commands
	//digitalWrite(_rs_pin, LOW);
	setDataMode(0);//COMMAND MODE
	//digitalWrite(enPin, LOW);

	write4bits(0x03);
	delayMicroseconds(5000); // I have one LCD for which 4500 here was not long enough.
	// second try
	write4bits(0x03);      
	delayMicroseconds(150); // wait 
	// third go!
	write4bits(0x03); 
	delayMicroseconds(150);
	// finally, set to 4-bit interface
	write4bits(0x02); 
	delayMicroseconds(150);
	// finally, set # lines, font size, etc.
	command(LCD_FUNCTIONSET | displayfunction);  
	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;  
	display();
	// clear it off
	clear();
	// Initialize to default text direction (for romance languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);	
}

/********** high level commands, for the user! */
void LiquidCrystalNew::backlight(uint8_t val){
	bitWrite(_theData,LCDPIN_LD,val);
}

void LiquidCrystalNew::clear(){
	if (_en2 != 255) {
		_chip = 2;
		command(LCD_CLEARDISPLAY); 
		_chip = 0;
		command(LCD_CLEARDISPLAY);
		delayPerHome();
		setCursor(0,0);
	}
	else {
		command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
		delayPerHome();
	}
	_scroll_count = 0;
}

void LiquidCrystalNew::home(){
	commandBoth(LCD_RETURNHOME);  // set cursor position to zero      //both chips.
	delayPerHome();
	_scroll_count = 0;
	if (_en2 != 255) setCursor(0,0); 
}


// Turn the display on/off (quickly)
void LiquidCrystalNew::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	commandBoth(LCD_DISPLAYCONTROL | _displaycontrol);  //both chips
}
void LiquidCrystalNew::display() {
	_displaycontrol |= LCD_DISPLAYON;
	commandBoth(LCD_DISPLAYCONTROL | _displaycontrol & LCD_CURSORS_MASK);   //both chips on, both cursors off
	command(LCD_DISPLAYCONTROL | _displaycontrol);              //selected chip gets cursor on
}
// Turns the underline cursor on/off
void LiquidCrystalNew::noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystalNew::cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LiquidCrystalNew::noBlink() {
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystalNew::blink() {
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void LiquidCrystalNew::scrollDisplayLeft(void) {
	commandBoth(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);  //both chips
	_scroll_count++;
	if (_scroll_count >= 40) _scroll_count -= 40;   //  -39< scroll_count<39
}
void LiquidCrystalNew::scrollDisplayRight(void) {
	commandBoth(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);  //both chips
	_scroll_count--;
	if (_scroll_count <= -40) _scroll_count += 40;
}

// This is for text that flows Left to Right
void LiquidCrystalNew::leftToRight(void) {
	_displaymode |= LCD_ENTRYLEFT;
	_direction = LCD_Right;
	commandBoth(LCD_ENTRYMODESET | _displaymode);     //both chips
}

// This is for text that flows Right to Left
void LiquidCrystalNew::rightToLeft(void) {
	_displaymode &= ~LCD_ENTRYLEFT;
	_direction = LCD_Left;
	commandBoth(LCD_ENTRYMODESET | _displaymode);    //both chips
}


// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LiquidCrystalNew::createChar(uint8_t location, uint8_t charmap[]) {    
	location &= 0x7; // we only have 8 locations 0-7
	if (_en2 == 255) {
		command(LCD_SETCGRAMADDR | (location << 3));
		for (int i=0; i<8; i++) {
			send(charmap[i],HIGH);
		}
	} else {
		uint8_t chipSave = _chip;
		_chip = 0;
		command(LCD_SETCGRAMADDR | (location << 3));
		for (uint8_t i=0; i<8; i++) {
			send(charmap[i],HIGH);
		}
		_chip = 2;
		command(LCD_SETCGRAMADDR | (location << 3));
		for (uint8_t i=0; i<8; i++) {
			send(charmap[i],HIGH);
		}
		_chip = chipSave;
	}
}

void LiquidCrystalNew::setCursor(uint8_t col, uint8_t row) { // this can be called by the user but is also called before writing some characters.
	if ( row > _numlines ) row = _numlines-1;    // we count rows starting w/0
	_y = row;
	_x = col;
	_setCursFlag = 0;                                                 //user did a setCursor--clear the flag that may have been set in write()
	int8_t high_bit = row_offsets[row] & 0x40;                        // this keeps coordinates pegged to a spot on the LCD screen even if the user scrolls right or
	int8_t offset = col + (row_offsets[row] &0x3f) + _scroll_count; //left under program control. Previously setCursor was pegged to a location in DDRAM
	//the 3 quantities we add are each <40
	if (offset > 39) offset -= 40;                                    // if the display is autoscrolled this method does not work, however.
	if (offset < 0) offset += 40;
	offset |= high_bit;
	if (_chip != (row & 0b10)) 	{
		command(LCD_DISPLAYCONTROL | _displaycontrol & LCD_CURSORS_MASK);  //turn off cursors on chip we are leaving
		_chip = row & 0b10;																//if it is row 0 or 1 this is 0; if it is row 2 or 3 this is 2
		command(LCD_DISPLAYCONTROL | _displaycontrol);									//turn on cursor on chip we moved to
	}
	command(LCD_SETDDRAMADDR | (byte) offset );
}

// This will 'right justify' text from the cursor 
void LiquidCrystalNew::autoscroll(void) {           //to count the number of times we scrolled; here we'd need to keep track of microseconds and divide. I'm not going there.
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	commandBoth(LCD_ENTRYMODESET | _displaymode);   //both chips
}

// This will 'left justify' text from the cursor
void LiquidCrystalNew::noAutoscroll(void) {
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;   //both chips
	commandBoth(LCD_ENTRYMODESET | _displaymode);
}

uint8_t LiquidCrystalNew::getCursorCol(void) {
	return _x;
}

uint8_t LiquidCrystalNew::getCursorRow(void) {
	return _y;
}

/*********** mid level commands, for sending data/cmds */

void LiquidCrystalNew::commandBoth(uint8_t value) {  //for many of the commands that need to be sent twice if 2 controller chips
	if (_en2 == 255) {
		send(value,LOW);  //not 40x4
	} else {
		uint8_t chipSave = _chip;
		_chip = 0;
		send(value,LOW);   //send command to first HD44780
		_chip = 2;
		send(value,LOW);   //send to 2nd HD44780
		_chip = chipSave;
	}
}

size_t LiquidCrystalNew::write(uint8_t value) {                      //print calls  this to send characters to the LCD
	if ((_scroll_count != 0) || (_setCursFlag != 0) ) setCursor(_x,_y);   //first we call setCursor and send the character
	if ((value != '\r') && (value != '\n')) send(value,HIGH);
	
	_setCursFlag = 0;
	if (_direction == LCD_Right) {                    // then we update the x & y location for the NEXT character
		_x++;
		if ((value == '\r') ||(_x >= _numcols)) {      //romance languages go left to right
			_x = 0;
			_y++;
			_setCursFlag = 1;          //we'll need a setCursor() before the next char to move to begin of next line
		}
	}
	else {
		_x--;
		if ( (value == '\n') || (_x < 0)) {   //emulate right to left text mode which is built in but would be defeated by my code above
			_x = _numcols-1;
			_y++;
			_setCursFlag = 1;
		}
	}
	if (_y >= _numlines) _y = 0;   //wrap last line up to line 0
	return 1; //assume success  added for Arduino 1
}

/************ low level data pushing commands **********/

// write either command or data, with automatic 4/8-bit selection
void LiquidCrystalNew::send(uint8_t value, uint8_t mode) {
		uint8_t en = _en1;
		if ((_en2 != 255) && (_chip)) en = _en2;
		delayMicroseconds(DELAYPERCHAR);
		
		setDataMode(mode);

		switch (_driveType){
			case 0://direct
				#if defined(__FASTSPI)		
				digitalWriteFast(_data_pins[0],value & 0x10);
				digitalWriteFast(_data_pins[1],value & 0x20);
				digitalWriteFast(_data_pins[2],value & 0x40);
				digitalWriteFast(_data_pins[3],value & 0x80);
				pulseEnable(en);
		
				digitalWriteFast(_data_pins[0],value & 0x01);
				digitalWriteFast(_data_pins[1],value & 0x02);
				digitalWriteFast(_data_pins[2],value & 0x04);
				digitalWriteFast(_data_pins[3],value & 0x08);
				#else
				digitalWrite(_data_pins[0],value & 0x10);
				digitalWrite(_data_pins[1],value & 0x20);
				digitalWrite(_data_pins[2],value & 0x40);
				digitalWrite(_data_pins[3],value & 0x80);

				pulseEnable(en);
				digitalWrite(_data_pins[0],value & 0x01);
				digitalWrite(_data_pins[1],value & 0x02);
				digitalWrite(_data_pins[2],value & 0x04);
				digitalWrite(_data_pins[3],value & 0x08);
				#endif
			break;
			case 1://SPI
			case 2://I2C
				bitWrite(_theData,LCDPIN_D4,value & 0x10);
				bitWrite(_theData,LCDPIN_D5,value & 0x20);
				bitWrite(_theData,LCDPIN_D6,value & 0x40);
				bitWrite(_theData,LCDPIN_D7,value & 0x80);
				pulseEnable(en);
				bitWrite(_theData,LCDPIN_D4,value & 0x01);
				bitWrite(_theData,LCDPIN_D5,value & 0x02);
				bitWrite(_theData,LCDPIN_D6,value & 0x04);
				bitWrite(_theData,LCDPIN_D7,value & 0x08);
			break;
		}
		
		pulseEnable(en);
		
	}

void LiquidCrystalNew::write4bits(uint8_t value) {  //still used during init
	register uint8_t v = value;
	uint8_t *pinptr = _data_pins;
	byte en = _en1;
 // 4x40 LCD with 2 controller chips with separate enable lines if we called w 2 enable pins and are on lines 2 or 3 enable chip 2  
	if ((_en2 != 255) && (_chip)) en = _en2;   
	switch (_driveType){
		case 0://direct
#if defined(__FASTSPI)
			digitalWriteFast(*pinptr++,v & 01);
			digitalWriteFast(*pinptr++,(v >>= 1) & 01);
			digitalWriteFast(*pinptr++,(v >>= 1) & 01);
			digitalWriteFast(*pinptr++,(v >>= 1) & 01);
#else
			digitalWrite(*pinptr++,v & 01);
			digitalWrite(*pinptr++,(v >>= 1) & 01);
			digitalWrite(*pinptr++,(v >>= 1) & 01);
			digitalWrite(*pinptr++,(v >>= 1) & 01);
#endif
		break;
		case 1://SPI
		case 2://I2C
			bitWrite(_theData,LCDPIN_D4,v & 01);
			bitWrite(_theData,LCDPIN_D5,(v >>= 1) & 01);
			bitWrite(_theData,LCDPIN_D6,(v >>= 1) & 01);
			bitWrite(_theData,LCDPIN_D7,(v >>= 1) & 01);
		break;
	}	
	
	pulseEnable(en);
	
} 

//0:COMMAND -- 1:DATA
void LiquidCrystalNew::setDataMode(uint8_t mode) {
	switch (_driveType){
		case 0://direct
#if defined(__FASTSPI)
			digitalWriteFast(_rs_pin,mode);
#else
			digitalWrite(_rs_pin,mode);
#endif
		break;
		case 1://SPI
		case 2://I2C
			bitWrite(_theData,LCDPIN_RS,mode);
		break;
	}	
}

void LiquidCrystalNew::pulseEnable(uint8_t enPin) {
	switch (_driveType){
		case 0://direct
#if defined(__FASTSPI)
			digitalWriteFast(enPin,HIGH);   // enable pulse must be >450ns
			digitalWriteFast(enPin,LOW);
#else
			digitalWrite(enPin,HIGH);   // enable pulse must be >450ns
			digitalWrite(enPin,LOW);
#endif
		break;
		case 1://SPI
		case 2://I2C
			writeGpio(_theData | enPin);   // En HIGH
			writeGpio(_theData & ~enPin);  // En LOW
		break;
	}	
}

/* void LiquidCrystalNew::pulseEnable(uint8_t data,uint8_t chip){
	writeGpio(data | chip);   // En HIGH
	writeGpio(data & ~chip);  // En LOW
}
 */
/*
------------------------------SPI
*/
void LiquidCrystalNew::writeByte(byte cmd,uint8_t value){
  startSend();
	SPI.transfer(cmd);
	SPI.transfer(value);
  endSend();
}

void LiquidCrystalNew::writeGpio(uint8_t value){
      // Only write HIGH the values of the ports that have been initialised as outputs updating the output shadow of the device
	_theData = (value & ~(0x00));
  startSend();
	SPI.transfer(0x09);//GPIO
	SPI.transfer(_theData);
  endSend();
}

void LiquidCrystalNew::startSend(){
#if defined(__FASTSPI)
	digitalWriteFast(_cs, LOW);
#else
	digitalWrite(_cs, LOW);
#endif
	SPI.transfer(_adrs << 1);//in write, in read: SPI.transfer((addr << 1) | 1);
}

void LiquidCrystalNew::endSend(){
#if defined(__FASTSPI)
	digitalWriteFast(_cs, HIGH);
#else
	digitalWrite(_cs, HIGH);
#endif
}