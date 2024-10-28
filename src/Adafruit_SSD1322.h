/*********************************************************************
This is a library for the SSD1322, based directly on the Adafruit SSD1327
library. The original comment for that library is left below for reference.
*********************************************************************/

/*********************************************************************
This is a library for our Grayscale OLEDs based on SSD1327 drivers

  Pick one up today in the adafruit shop!
  ------> https://www.adafruit.com/products/4741

These displays use I2C or SPI to communicate

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

#include <Adafruit_GrayOLED.h>


/*! The controller object for SSD1322 OLED displays */
class Adafruit_SSD1322 : public Adafruit_GrayOLED {
public:
  enum {
    VARIANT_SSD1322,
    VARIANT_SSH1122
  };

  Adafruit_SSD1322(int8_t mosi_pin, int8_t sclk_pin,
                   int8_t dc_pin, int8_t rst_pin, int8_t cs_pin, int8_t variant = VARIANT_SSD1322);
  Adafruit_SSD1322(SPIClass *spi, int8_t dc_pin,
                   int8_t rst_pin, int8_t cs_pin, int8_t variant = VARIANT_SSD1322, uint32_t bitrate = 8000000UL);
  ~Adafruit_SSD1322(void);

  bool begin(bool reset = true);
  void display();
  void invertDisplay(bool i);

  // Slightly different from the default implementation in the superclass --
  // range is from 0x00 to 0xFF
  void setContrast(uint8_t level);

private:
  int8_t page_offset = 0;
  int8_t column_offset = 0;
  int8_t variant;
 
  // internal methods
  void start_write(uint16_t start_column, uint16_t start_row, uint16_t end_column, uint16_t end_row);
  void continue_write(uint16_t column, uint16_t row);

  // convenience methods
  void spi_command(uint8_t c);
  void spi_command(uint8_t c, uint8_t d1);
  void spi_command(uint8_t c, uint8_t d1, uint8_t d2);
 
  // core spi write methods
  void spi_command_data(uint8_t c, uint8_t *data, size_t count);
  void spi_data(uint8_t *data, size_t count);
};
