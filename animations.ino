// This file contains all of the animation functions implemented thus far:
// To create your own animations, describe a function here which returns an RGBPixel colour.
// It is given a pixel_request and the current time in milliseconds since the device was powered
// on. The pixel request struct has the following properties:
// 
//   pixel.idx    -> The numeric integer index of the LED pixel on the strip - the order in which
//                   they are wired up
//   pixel.kind   -> Either EarsInner or EarsEdge, depending on the function of this LED aethetically
//   pixel.height -> A numeric index of how high up this pixel's light shines primarily when seen
//                   by a regular observer
//   
// More properties and kinds maybe added in the future
// Your function may also optionally return CURRENT_COLOR in place of an actual RGBPixel color.
// This signals that the device should leave it set to it's existing colour - your function opts
// not to change it during this update.
//
// To add your animation to the deck, add it to the "PGM_P animations[] PROGMEM" array in fox_ears_v2.ino
// The order of this array is the order these animations will be in when clicking through via device buttons

// simple solid colour on all LEDs
RGBPixel solid(struct pixel_request pixel, unsigned long time) {
  return primary_color;
}

// simple edge coloured, inners black
RGBPixel edge(struct pixel_request pixel, unsigned long time) {
  return pixel.kind == EarsEdge ? primary_color : BLACK;
}

// simple inner ear coloured, edges black
RGBPixel inner(struct pixel_request pixel, unsigned long time) {
  return pixel.kind == EarsInner ? primary_color : BLACK;
}

// Bluebie's natural resting heartbeat duration is about about 1.1 seconds
#define HEARTBEAT_DURATION 1200 /* duration of entire animation loop */
#define HEARTBEAT_MICROS_INTERVAL ((HEARTBEAT_DURATION * 768UL) / 1000UL)
RGBPixel heartbeat(struct pixel_request pixel, unsigned long time) {
  static unsigned int animation_frame; // a number between 0 and 767, looping
  static unsigned int last_micros;
  unsigned int current_micros = micros();
  
  // step forward heart's internal clock with system clock
  if (current_micros > last_micros + HEARTBEAT_MICROS_INTERVAL || current_micros < last_micros) {
    animation_frame++;
    animation_frame %= 768;
    last_micros = current_micros;
  }
  
  // run animation based on heart's internal clock
  if (animation_frame < 512 && pixel.kind == EarsInner) {
    return multiply_colors(GRAY(255 - (animation_frame % 256)), primary_color);
  } else {
    return BLACK;
  }
}

// inner ears black, and different hues on each side of edge optic fibre to give a neat irredescent look
//RGBPixel gradient_edge(unsigned long time, byte pixel_idx, byte pixel_kind) {
RGBPixel gradient_edge(struct pixel_request pixel, unsigned long time) {
  if (pixel.kind == EarsEdge) {
    return pixel.idx % 2 == 0 ? primary_color : color_wheel(primary_color_hue + 60);
  } else {
    return BLACK;
  }
}

RGBPixel opposites(struct pixel_request pixel, unsigned long time) {
  return (pixel.kind == EarsEdge) ? primary_color : color_wheel(primary_color_hue + 128);
}

RGBPixel rainbow_opposites(struct pixel_request pixel, unsigned long time) {
  byte hue = time;
  return (pixel.kind == EarsEdge) ? color_wheel(hue) : color_wheel(hue + 128);
}

RGBPixel white(struct pixel_request pixel, unsigned long time) {
  return WHITE;
}

#define WaveDuration 350
#define WavesTotalDuration 1500
#define WaveOffset 21 /* in hundredths of seconds, unusually */
RGBPixel wave(struct pixel_request pixel, unsigned long time) {
  // triangle wave pulse one pixel at a start time for a duration
//void wave_light(byte pixel_id, RGBPixel color, unsigned long start_time, unsigned long duration) {
  unsigned long start_time = (time - (time % WavesTotalDuration)) + (WaveOffset * pixel.height);
  long relative_time = time - start_time;
  relative_time *= 256;
  relative_time /= WaveDuration;
  
  byte intensity = 0;
  if (relative_time >= 0 && relative_time <= 511) {
    intensity = lookup_sine(relative_time / 2);
  }
  
  // multiply primary colour with intensity to do fade
  return multiply_colors(primary_color, GRAY(intensity));
}

RGBPixel oceanic(struct pixel_request pixel, unsigned long time) {
  unsigned int step = time / 5;
  byte lightness = (lookup_sine((step % 512) / 2) + lookup_sine((step % 768) / 3)) / 4;
  byte blue = 192 + (lookup_sine((step % 256)) / 4);
  signed char color_wiggler = (lookup_sine((step % 1024) / 4) / 8) - 16;
  byte left = constrain(lightness - color_wiggler, 0, 255);
  byte right = constrain(lightness + color_wiggler, 0, 255);
  return (pixel.idx < 3) ? RGB(left, right, blue) : RGB(right, left, blue);
}

#define StrobeFrequency 16
RGBPixel strobe(struct pixel_request pixel, unsigned long time) {
  if (time % (1000 / StrobeFrequency) > (500 / StrobeFrequency)) {
    return primary_color;
  } else {
    return BLACK;
  }
}

RGBPixel alternating_strobe(struct pixel_request pixel, unsigned long time) {
  boolean toggle = time % (1000 / StrobeFrequency) > (500 / StrobeFrequency);
  if ((pixel.kind == EarsEdge) ? toggle : !toggle) {
    return primary_color;
  }
  
  return BLACK;
}

#define BicolorStrobeFrequency StrobeFrequency / 2
RGBPixel bicolor_strobe(struct pixel_request pixel, unsigned long time) {
  byte idx = (time / BicolorStrobeFrequency) % 4;
  if (idx & 1) return BLACK;
  if (idx != 0) return primary_color;
  return color_wheel(primary_color_hue + 128);
}


// calculate forest walk colors
RGBPixel forest_walk(struct pixel_request pixel, unsigned long time) {
  unsigned int step = (time / 3) + (pixel.height * 2);
  unsigned int green = perlin(step) + 70;
  
  byte lightness = green / 4;
  lightness += green > 255 ? green - 255 : 0;
  green -= green > 255 ? green - 255 : 0;
  signed char color_wiggler = (lookup_sine((step % 1024) / 4) / 8) - 16;
  
  RGBPixel output_hue = color_wheel(primary_color_hue + color_wiggler);
  RGBPixel output_lightness = GRAY(lightness);
  RGBPixel output_saturation = GRAY(green);
  return maximum_mix_colors(multiply_colors(output_hue, output_saturation), output_lightness);
}

#define RandomWalkerInterval 64 /* milliseconds */
RGBPixel random_walk(struct pixel_request pixel, unsigned long time) {
  RGBPixel color = color_wheel(sp_random(time / RandomWalkerInterval));
  byte target_pixel = sp_random(time / RandomWalkerInterval + 1000) % pixels.numPixels();
  if (target_pixel == pixel.idx) {
    return color;
  }
  return CURRENT_COLOR;
  
  /*static unsigned int prev_time;
  if (prev_time + RANDOM_WALKER_WAIT < time || time < prev_time) {
    prev_time = current_time;
    
    byte target_pixel = rand() % headband.numPixels();
    RGBPixel color = color_wheel(rand());
    headband.setPixelColor(target_pixel, color);
  } */
}

