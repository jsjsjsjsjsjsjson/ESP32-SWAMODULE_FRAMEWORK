/*********************************************************************
This is a library for the SSD1322, based directly on the Adafruit SSD1327
library. The original comment for that library is left below for reference.
*********************************************************************/

/*********************************************************************
This is a library for our grayscale OLEDs based on SSD1327 drivers

  Pick one up today in the adafruit shop!
  ------> https://www.adafruit.com/products/4741

These displays use I2C or SPI to communicate

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen below must be included in any
redistribution
*********************************************************************/

#include "Adafruit_SSD1322.h"
#include "splash.h"

// CONSTRUCTORS, DESTRUCTOR ------------------------------------------------

/*!
    @brief  Constructor for SPI SSD1322 displays, using software (bitbang)
            SPI.
    @param  w
            Display width in pixels
    @param  h
            Display height in pixels
    @param  mosi_pin
            MOSI (master out, slave in) pin (using Arduino pin numbering).
            This transfers serial data from microcontroller to display.
    @param  sclk_pin
            SCLK (serial clock) pin (using Arduino pin numbering).
            This clocks each bit from MOSI.
    @param  dc_pin
            Data/command pin (using Arduino pin numbering), selects whether
            display is receiving commands (low) or data (high).
    @param  rst_pin
            Reset pin (using Arduino pin numbering), or -1 if not used
            (some displays might be wired to share the microcontroller's
            reset pin).
    @param  cs_pin
            Chip-select pin (using Arduino pin numbering) for sharing the
            bus with other devices. Active low.
    @note   Call the object's begin() function before use -- buffer
            allocation is performed there!
*/
Adafruit_SSD1322::Adafruit_SSD1322(int8_t mosi_pin,
                                   int8_t sclk_pin, int8_t dc_pin,
                                   int8_t rst_pin, int8_t cs_pin, 
                                   int8_t variant)
    : Adafruit_GrayOLED(4, 256, 64, mosi_pin, sclk_pin, dc_pin, rst_pin, cs_pin),
      variant(variant) {}

/*!
    @brief  Constructor for SPI SSD1322 displays, using native hardware SPI.
    @param  w
            Display width in pixels
    @param  h
            Display height in pixels
    @param  spi
            Pointer to an existing SPIClass instance (e.g. &SPI, the
            microcontroller's primary SPI bus).
    @param  dc_pin
            Data/command pin (using Arduino pin numbering), selects whether
            display is receiving commands (low) or data (high).
    @param  rst_pin
            Reset pin (using Arduino pin numbering), or -1 if not used
            (some displays might be wired to share the microcontroller's
            reset pin).
    @param  cs_pin
            Chip-select pin (using Arduino pin numbering) for sharing the
            bus with other devices. Active low.
    @param  bitrate
            SPI clock rate for transfers to this display. Default if
            unspecified is 8000000UL (8 MHz).
    @note   Call the object's begin() function before use -- buffer
            allocation is performed there!
*/
Adafruit_SSD1322::Adafruit_SSD1322(SPIClass *spi,
                                   int8_t dc_pin, int8_t rst_pin, int8_t cs_pin,
                                   int8_t variant,
                                   uint32_t bitrate)
    : Adafruit_GrayOLED(4, 256, 64, spi, dc_pin, rst_pin, cs_pin, bitrate),
      variant(variant) {}

/*!
    @brief  Destructor for Adafruit_SSD1322 object.
*/
Adafruit_SSD1322::~Adafruit_SSD1322(void) {}

// Register definitions

#define SSD1322_BLACK 0x0
#define SSD1322_WHITE 0xF

#define SSD1322_SETCOLUMN 0x15

#define SSD1322_WRITERAM 0x5C

#define SSD1322_SETROW 0x75

#define SSD1322_SEGREMAP 0xA0
#define SSD1322_SETSTARTLINE 0xA1
#define SSD1322_SETDISPLAYOFFSET 0xA2
#define SSD1322_DISPLAYALLOFF 0xA4
#define SSD1322_DISPLAYALLON 0xA5
#define SSD1322_NORMALDISPLAY 0xA6
#define SSD1322_INVERTDISPLAY 0xA7
#define SSD1322_SETMULTIPLEX 0xA8
#define SSD1322_EXITPARTIALDISPLAY 0xA9
#define SSD1322_REGULATOR 0xAB
#define SSD1322_DISPLAYOFF 0xAE
#define SSD1322_DISPLAYON 0xAF

#define SSD1322_PHASELEN 0xB1
#define SSD1322_DCLK 0xB3
#define SSD1322_DISPLAYENHANCE 0xB4
#define SSD1322_SETGPIO 0xB5
#define SSD1322_PRECHARGE2 0xB6
#define SSD1322_GRAYTABLE 0xB8
#define SSD1322_SELECTDEFAULTGRAYSCALE 0xB9
#define SSD1322_SETPRECHARGEVOLTAGE 0xBB
#define SSD1322_SETVCOM 0xBE

#define SSD1322_SETCONTRASTCURRENT 0xC1
#define SSD1322_MASTERCURRENTCONTROL 0xC7
#define SSD1322_SETMUXRATIO 0xCA

#define SSD1322_DISPLAYENHANCEB 0xD1
#define SSD1322_FUNCSELB 0xD5

#define SSD1322_CMDLOCK 0xFD

// Different commands (and partial commands/bitfields) for SH1122.
// SH1122_SETCOLUMN gets OR'ed with the high 3 bits of the column address.
// The low 4 bits go in a second command byte.
#define SH1122_SETCOLUMN 0x10
#define SH1122_SETROW 0xB0
// SH1122_SETSTARTLINE gets OR'ed with the start line in the lower 6 bits.
#define SH1122_SETSTARTLINE 0x40
#define SH1122_SETCONTRAST 0x81
#define SH1122_DC_DC_CONTROL 0xAD
#define SH1122_OUTPUT_SCAN_DIRECTION_UP 0xC0
#define SH1122_OUTPUT_SCAN_DIRECTION_DOWN 0xC8
#define SH1122_DISPLAY_OFFSET 0xD3
#define SH1122_DISPLAY_CLOCK 0xD5
#define SH1122_PRECHARGE_PERIOD 0xD9
#define SH1122_VCOMH 0xDB
#define SH1122_VSEGM_LEVEL 0xDC

// ALLOCATE & INIT DISPLAY -------------------------------------------------

/*!
    @brief  Allocate RAM for image buffer, initialize peripherals and pins.
    @param  reset
            If true, and if the reset pin passed to the constructor is
            valid, a hard reset will be performed before initializing the
            display. If using multiple SSD1322 displays on the same bus, and
            if they all share the same reset pin, you should only pass true
            on the first display being initialized, false on all others,
            else the already-initialized displays would be reset. Default if
            unspecified is true.
    @return true on successful allocation/init, false otherwise.
            Well-behaved code should check the return value before
            proceeding.
    @note   MUST call this function before any drawing or updates!
*/
bool Adafruit_SSD1322::begin(bool reset) {

	// set pin directions (superclass takes care of dcPin and rstPin)
	pinMode(csPin, OUTPUT);

	// The SSD1322 doesn't support I2C, so the address will never be used.
	if (!Adafruit_GrayOLED::_init(0, reset)) {
		return false;
	}

	// Init sequence. For SSD1322, his is copied from the initialization in:
	// https://github.com/winneymj/ESP8266_SSD1322
	// with a couple of modifications gleaned from the Arduino tutorial here:
	// https://www.buydisplay.com/white-3-2-inch-arduino-raspberry-pi-oled-display-module-256x64-spi
	// For SH1122, it is derived from 8051 the example code here:
	// https://www.buydisplay.com/white-2-08-inch-graphic-oled-display-panel-256x64-parallel-spi-i2c

	if (variant == VARIANT_SSD1322) {
		spi_command(SSD1322_CMDLOCK, // 0xFD
		0x12);// Unlock OLED driver IC

		spi_command(SSD1322_DISPLAYOFF);// 0xAE

		spi_command(SSD1322_DCLK,// 0xB3
		0x91);

		spi_command(SSD1322_SETMUXRATIO, // 0xCA
		0x3F);// duty = 1/64

		spi_command(SSD1322_SETDISPLAYOFFSET, // 0xA2
		0x00);

		spi_command(SSD1322_SETSTARTLINE, // 0xA1
		0x00);

		spi_command(SSD1322_SEGREMAP, // 0xA0
		0x14, //Horizontal address increment,Disable Column Address Re-map,Enable Nibble Re-map,Scan from COM[N-1] to COM0,Disable COM Split Odd Even
		0x11);//Enable Dual COM mode

		spi_command(SSD1322_SETGPIO, // 0xB5
		0x00);// Disable GPIO Pins Input

		spi_command(SSD1322_REGULATOR, // 0xAB
		0x01);// selection external vdd

		spi_command(SSD1322_DISPLAYENHANCE, // 0xB4
		0xA0,// enables the external VSL
		0xFD);// 0xfFD,Enhanced low GS display quality;default is 0xb5(normal),

		spi_command(SSD1322_SETCONTRASTCURRENT, // 0xC1
		// 0xFF);// 0xFF - default is 0x7f
		0x80);

		spi_command(SSD1322_MASTERCURRENTCONTROL, // 0xC7
		0x0F);// default is 0x0F

		// Set grayscale
		spi_command(SSD1322_SELECTDEFAULTGRAYSCALE); // 0xB9

		spi_command(SSD1322_PHASELEN, // 0xB1
		0xE2);// default is 0x74

		spi_command(SSD1322_DISPLAYENHANCEB, // 0xD1
		0x82, // Reserved;default is 0xa2(normal)
		0x20);//

		spi_command(SSD1322_SETPRECHARGEVOLTAGE, // 0xBB
		0x1F);// 0.6xVcc

		spi_command(SSD1322_PRECHARGE2, // 0xB6
		0x08);// default

		spi_command(SSD1322_SETVCOM, // 0xBE
		0x07);// 0.86xVcc;default is 0x04

		spi_command(SSD1322_NORMALDISPLAY);// 0xA6

		spi_command(SSD1322_EXITPARTIALDISPLAY);// 0xA9
	}
	else if (variant == VARIANT_SSH1122) {
		
		// Display Off (0xAE/0xAF)
		spi_command(SSD1322_DISPLAYOFF);

		// set start line to 0.
		spi_command(SH1122_SETSTARTLINE); 

		// Set brightness 
		spi_command(SH1122_SETCONTRAST, 0x80);

		// Set segment re-map   The right (0) or left(1) rotation
		spi_command(SSD1322_SEGREMAP | 0);

		// 0xA4=normal display ; 0xA5=Entire Display ON
		spi_command(SSD1322_DISPLAYALLOFF);

		// 0x00=normal display; 0x01=reverse display
		spi_command(SSD1322_DISPLAYALLOFF);

		// Set multiplex ratio to 1/64 Duty (0x0F~0x3F) (default is 0x3F)
		spi_command(SSD1322_SETMULTIPLEX, 0x3F);

		// Set the DC-DC voltage and the switch frequency
		spi_command(SH1122_DC_DC_CONTROL, 0x80);

		// Set Row Address
		spi_command(SH1122_SETROW, 0x00);

		// Common output scan direction: C0= scan from COM0 to COM[N-1}; C8=scan From COM[N-1] to COM0
		spi_command(SH1122_OUTPUT_SCAN_DIRECTION_UP);

		// Set Display Offset
		spi_command(SH1122_DISPLAY_OFFSET, 0x00);

		// Set Display clock (default is 0x50, 0x90 is 80 Frames/Sec)
		spi_command(SH1122_DISPLAY_CLOCK, 0x90);
		
		// Set Dis-Charge/Pre-Charge Period 
		spi_command(SH1122_PRECHARGE_PERIOD, 0x76);

		// Set VCOM Deselect Level Data
		spi_command(SH1122_VCOMH, 0x3B);

		// Set the segment pad output voltage level at pre-charge stage
		spi_command(SH1122_VSEGM_LEVEL, 0x1a);

		// Set the discharbe voltage level
		spi_command(0x30);

	// Set_Display_On_Off(0xAE);				// Display Off (0xAE/0xAF)		
	// Set_Start_Line(0x00);					// Set Display Start Line (0x00~0x7F)
	// Set_Contrast_Control(Brightness);		// Set Scale Factor of Segment Output Current Control
	// Set_Remap_Format(0x00);					// Set segment re-map   The right (0) or left(1) rotation
	// Set_Entire_Display_OFF_On(0x00);		//0x00= normal display ; 0x01=Entire Display ON
	// Set_Display_Mode(0x00);					//0x00= normal display; 0x01=reverse display
	// Set_Multiplex_Ratio(0x3F);				// 1/64 Duty (0x0F~0x3F)
	// Set_DC_DC_Control(0x80);				//control the DC-DC voltage and the switch frequency
	// Set_Row_Address_Set(0x00);					//Row Address Set
  	// Set_Common_Output_ScanDirection(0xC0);	             //C0= scan from COM0 to COM[N-1}; C8=scan From COM[N-1] to COM0
	// Set_Display_Offset(0x00);				//Set Offset Data 
	// Set_Display_Clock(0x90);				// Set Clock as 80 Frames/Sec
	// Set_Precharge_Period(0x76);				// Set Dis-Charge/Pre-Charge Period 
	// Set_VCOMH(0x3B);						// Set VCOM Deselect Level Data 
   	// Set_VSEGM_Level(0x1a);					//Set the segment pad output voltage level at pre-charge stage
	// Set_Discharge_Voltage_VSL(0x30);		//Set the discharbe voltage level		  
	}

	delay(100);                      // 100ms delay recommended

	spi_command(SSD1322_DISPLAYON); // 0xaf

  // The default "set contrast" command (0x81) doesn't appear in the SSD1322 datasheet.
  // Calling this might be bad?
//   setContrast(0x2F);

  return true; // Success
}

void Adafruit_SSD1322::start_write(uint16_t start_column, uint16_t start_row, uint16_t end_column, uint16_t end_row)
{
	if (variant == VARIANT_SSD1322) {
		// This variant wants a full window (including width/height) and auto-increments if it's not the full display width.
		spi_command(SSD1322_SETCOLUMN, uint8_t(start_column + 0x1c), uint8_t(end_column + 0x1c));
		spi_command(SSD1322_SETROW, uint8_t(start_row), uint8_t(end_row));
		spi_command(SSD1322_WRITERAM);
	}
}

void Adafruit_SSD1322::continue_write(uint16_t column, uint16_t row)
{
	if (variant == VARIANT_SSH1122) {
		// Column is in the right format for the SSD1322. The SH1122 wants it doubled.
		column <<= 1;
		// We need to set a new column and row address for each line.
		spi_command(SH1122_SETCOLUMN | ((column >> 4) & 0x07), column & 0x0F);
		spi_command(SH1122_SETROW, row);
	}
}

/*!
    @brief  Do the actual writing of the internal frame buffer to display RAM
*/
void Adafruit_SSD1322::display(void) {
	// ESP8266 needs a periodic yield() call to avoid watchdog reset.
	// With the limited size of SSD1322 displays, and the fast bitrate
	// being used (1 MHz or more), I think one yield() immediately before
	// a screen write and one immediately after should cover it.  But if
	// not, if this becomes a problem, yields() might be added in the
	// 32-byte transfer condition below.
	yield();

	// If the dirty window is empty, early-exit.
	if ((window_x1 > window_x2) || (window_y1 > window_y2))
	{
		window_x1 = 1024;
		window_y1 = 1024;
		window_x2 = -1;
		window_y2 = -1;
		return;
	}

	uint8_t *ptr = buffer;
	uint8_t rows = HEIGHT;

	uint8_t bytes_per_row = WIDTH / 2;

	// On the SH1122, the "bytes == bytes_per_row" case seems to be faster for most cases
	// (even with writing more data, apparently the reduced overhead is a win).
	// Expand the window to the full width of the display to take advantage of this.
	// Only do this if we're above a certain width threshold, since it might not be a win for very narrow blits.
	if ((variant == VARIANT_SSH1122) &&
		(window_x2 - window_x1 > 16)) {
		window_x1 = 0;
		window_x2 = WIDTH - 1;
	}

	// Column addresses seem to be in 2-byte (4-pixel) units.
	int16_t start_column = min(int16_t((WIDTH/4)), int16_t(window_x1 / 4));
	int16_t end_column = max(int16_t(0), int16_t(window_x2 / 4));

	int16_t start_row = min(int16_t(rows - 1), int16_t(window_y1));
	// The dirty window seems to need to be expanded by 1 in y.
	int16_t end_row = max(int16_t(0), int16_t(window_y2 + 1));

	start_write(start_column, start_row, end_column, end_row);

	// Need to write two bytes for every column.
	size_t bytes = (end_column - start_column + 1) * 2;

	if (bytes == bytes_per_row)
	{
		// Contiguous write case -- just write the entire buffer
		continue_write(start_column, start_row);
		bytes *= end_row - start_row;
		ptr = buffer + (uint16_t)start_row * (uint16_t)bytes_per_row;
		ptr += (start_column * 2);
		// Write the entire buffer in one go.
		// Serial.printf("contiguous write %d, %d -> %d (%d)\n", start_column, start_row, end_row, bytes);
		spi_data(ptr, bytes);
	}
	else
	{
		// Serial.printf("writing %d rows at %d bytes each\n", int(end_row - start_row), int(bytes));

		for (uint8_t row = start_row; row < end_row; row++) 
		{
			continue_write(start_column, row);
			ptr = buffer + (uint16_t)row * (uint16_t)bytes_per_row;

			// fast forward to dirty rectangle beginning
			ptr += (start_column * 2);

			// Write the entire contents of this row in one go.
			// Serial.printf("row write %d, %d (%d)\n", start_column, row, bytes);
			spi_data(ptr, bytes);
			// yield();

		}
	}

	// reset dirty window
	window_x1 = 1024;
	window_y1 = 1024;
	window_x2 = -1;
	window_y2 = -1;
}

void Adafruit_SSD1322::spi_command(uint8_t c)
{
  // Serial.printf("command: %02x\n", c);
  spi_command_data(c, (uint8_t*)NULL, 0);
}

void Adafruit_SSD1322::spi_command(uint8_t c, uint8_t d1)
{
  // Serial.printf("command: %02x %02x\n", c, d1);
  spi_command_data(c, &d1, 1);
}

void Adafruit_SSD1322::spi_command(uint8_t c, uint8_t d1, uint8_t d2)
{
  // Serial.printf("command: %02x %02x %02x\n", c, d1, d2);
  uint8_t buf[] = {d1, d2};
  spi_command_data(c, buf, 2);
}

void Adafruit_SSD1322::spi_command_data(uint8_t c, uint8_t *data, size_t count)
{
	digitalWrite(dcPin, LOW);
	spi_dev->write(&c, 1);
	if (count > 0) {
		if (variant == VARIANT_SSD1322) {
			// The SSD1322 wants DC pulled high for subsequent bytes of multi-byte commands
			spi_data(data, count);
		} else {
			// The SSH1122 wants DC to stay low for all command bytes
			spi_dev->write(data, count);
		}
	}
}

void Adafruit_SSD1322::spi_data(uint8_t *data, size_t count)
{
  digitalWrite(dcPin, HIGH);
  spi_dev->write(data, count);    
}

/*!
    @brief  Enable or disable display invert mode (white-on-black vs
            black-on-white). Handy for testing!
    @param  i
            If true, switch to invert mode (black-on-white), else normal
            mode (white-on-black).
*/
void Adafruit_SSD1322::invertDisplay(bool i) {
  spi_command(i ? SSD1322_INVERTDISPLAY : SSD1322_NORMALDISPLAY);
}

void Adafruit_SSD1322::setContrast(uint8_t level)
{
	if (variant == VARIANT_SSD1322) {
		spi_command(SSD1322_SETCONTRASTCURRENT, level);
	} else if (variant == VARIANT_SSH1122) {
		spi_command(SH1122_SETCONTRAST, level);
	}
}
