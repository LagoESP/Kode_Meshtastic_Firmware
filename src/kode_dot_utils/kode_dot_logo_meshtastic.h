// meshtastic_logo.h
#ifndef MESHTASTIC_LOGO_H
#define MESHTASTIC_LOGO_H

#include <stdint.h>
#include <Arduino.h>

// Image dimensions
const int meshtastic_logo_width = 410;  // Replace with your actual width
const int meshtastic_logo_height = 502; // Replace with your actual height

// Image data (this is your large array from the converter)
extern const uint16_t meshtastic_logo[] PROGMEM;

#endif