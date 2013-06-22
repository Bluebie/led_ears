// Fox Ears sketch rewrite v2
#include <Adafruit_NeoPixel.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>
#include "utilities.h"

// default settings for different people:
// note these aren't used anymore - legacy stuff TODO: REMOVE THIS after fixing gradient edge animation
#define HueDefaultBluebie 85
#define HueDefaultDresona 42
#define HueDefault HueDefaultBluebie
#define GradientEdgeDefaultHue 185
// note that hues range from blue at 0, through to purple, red, orange, green, aqua, blue again at 255

// constants used to identify different groups of pixels
#define EarsInner 1
#define EarsEdge 0

#define PrimaryButton 5
#define SecondaryButton 1

#define NumPrograms (sizeof(programs) / sizeof(PGM_P))

// library used to communcate to these LED pixels in the loop function:
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(6, 0, NEO_RGB + NEO_KHZ800);
// currently selected program:
byte selected_program_idx = 0;
// list of available programs
PGM_P programs[] PROGMEM = {
  (PGM_P) solid,
  (PGM_P) edge,
  (PGM_P) inner,
  (PGM_P) gradient_edge,
  (PGM_P) opposites,
  (PGM_P) rainbow_opposites,
  (PGM_P) strobe,
  (PGM_P) alternating_strobe,
  (PGM_P) bicolor_strobe
};

struct pixel_request {
  byte idx;
  byte kind;
  byte height;
};

// some state stuff
RGBPixel primary_color;
byte primary_color_hue = HueDefault;

void setup() {
  pinMode(5, INPUT);
  pinMode(1, INPUT);
  // set our two input buttons high
  digitalWrite(5, HIGH);
  digitalWrite(1, HIGH);
  
  primary_color_hue = EEPROM.read(0);
  selected_program_idx = EEPROM.read(1) % NumPrograms;
  
  pixels.begin();
}

void loop() {
  // load the current time in milliseconds
  unsigned long time = millis();
  
  primary_color = color_wheel(primary_color_hue);
  
  update_eeprom();
  
  // detect when button is pressed down
  handle_primary_button(time);
  handle_secondary_button(time);
  
  // load our currently selected function
  //RGBPixel (*program_function)(unsigned long time, byte pixel_idx, byte pixel_kind)
  //  = (RGBPixel(*)(unsigned long, byte, byte)) pgm_read_word(programs + selected_program_idx);
  RGBPixel (*program_function)(struct pixel_request, unsigned long)
    = (RGBPixel(*)(struct pixel_request, unsigned long)) pgm_read_word(programs + selected_program_idx);
  
  // ask function for colours of all our LEDs
  struct pixel_request pixel; // variable to store our requests
  for (pixel.idx = 0; pixel.idx < pixels.numPixels(); pixel.idx++) {
    pixel.kind = (pixel.idx == 2 || pixel.idx == 3) ? EarsInner : EarsEdge;
    RGBPixel color = program_function(pixel, time);
    pixels.setPixelColor(pixel.idx, color);
  }
  
  // send new colours to LEDs
  pixels.show();
}

#define EEPROMUpdateInterval 500
void update_eeprom(void) {
  static unsigned long update_clock;
  unsigned long time = millis();
  if ((time - update_clock) > EEPROMUpdateInterval) {
    if (EEPROM.read(0) != primary_color_hue) EEPROM.write(0, primary_color_hue);
    if (EEPROM.read(1) != selected_program_idx) EEPROM.write(1, selected_program_idx);
    update_clock = time;
  }
}

void handle_primary_button(byte time) {
  Debounce(time, digitalRead(PrimaryButton));
  static boolean recharged;
  
  if (!digitalRead(PrimaryButton)) {
    if (recharged) {
      selected_program_idx = (selected_program_idx + 1) % NumPrograms;
      //if (selected_program_idx >= NumPrograms) selected_program_idx = 0;
      indicate(color_wheel(HueDefault)); //RGB(255, 255, 255));
      recharged = false;
    }
  } else {
    recharged = true;
  }
}

#define HueSelectDiv 20
void handle_secondary_button(unsigned int time) {
  Debounce(time, digitalRead(PrimaryButton));
  if (!digitalRead(SecondaryButton)) {
    static byte prev_time;
    if (prev_time != (time / HueSelectDiv) % 256) {
      prev_time = (time / HueSelectDiv);
      primary_color_hue += 1;
    }
  }
}

void indicate(RGBPixel color) {
  for (int i = 255; i >= 0; i--) {
    for (byte pixel = 0; pixel < pixels.numPixels(); pixel++) {
      pixels.setPixelColor(pixel, multiply_colors(color, GRAY(i)));
    }
    pixels.show();
  }
}
