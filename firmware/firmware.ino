// Bluetooth Stuff

#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define VERBOSE_MODE                   true  // If set to 'true' enables debug output
#define BUFSIZE                        128   // Size of the read buffer for incoming data

#define BLUEFRUIT_SPI_CS               8
#define BLUEFRUIT_SPI_IRQ              7
#define BLUEFRUIT_SPI_RST              4

Adafruit_BluefruitLE_SPI ble_spi(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
Adafruit_BluefruitLE_UART ble_uart(Serial1, -1);


// LED Stuff

#include <FastLED.h>
#define LED_PIN 13
#define CLOCK_PIN 12
#define COLOR_ORDER GBR
#define NUM_LEDS 47
#define MAX_LED_CURRENT_MA 500
#define MAX_LED_VOLTS 5
#define MAX_BRIGHTNESS 100

#define LED_LOOP_DELAY_MS 20

CRGB leds[NUM_LEDS];

CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette;
TBlendType    currentBlending;

// Pin to trigger sound fx
#define SOUNDFX_PIN 14

// Hit detection stuff
#define INTERRUPT_PIN 11
#define WAIT_TIME_MS 5000

volatile unsigned long timestamp;
volatile byte hitInterruptCounter = 0;

// You can add more emojis in this array. Make sure they are ASCII (no unicode characters)!
// Don't forget to escape quotes.
// Sometimes a character needs to be repeated for it to be sent. Maybe a bug in Bluetooth LE firmware?
char *emojis[] = {
  "\\_(._.)_/",
  "(/^_^)/",
  "\\(\"- x -)/",
  "(; -__--)",
  "(>_<)"
 };



// Helper functions
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void resetTimer() {
  timestamp = millis();
}

unsigned long getTimerElapsed() {
  return(millis() - timestamp);
}


void setupSoundfx() {
  pinMode(SOUNDFX_PIN, OUTPUT);
  digitalWrite(SOUNDFX_PIN, HIGH);
}

void playSoundfx() {
  digitalWrite(SOUNDFX_PIN, LOW);
  delay(130);
  digitalWrite(SOUNDFX_PIN, HIGH);
}


void setupBLE() {
  
  Serial.print(F("Initialising the Bluefruit LE UART module: "));

  if ( !ble_uart.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit UART, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  Serial.print(F("Initialising the Bluefruit LE SPI module: "));

  if ( !ble_spi.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit SPI, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  /* Disable command echo from Bluefruit */
  ble_uart.echo(false);
  ble_spi.echo(false);

  /* Change the device name to make it easier to find */
  Serial.println(F("Setting device name to 'Bluefruit Keyboard UART': "));
  if (! ble_uart.sendCommandCheckOK(F( "AT+GAPDEVNAME=Bluefruit Keyboard UART" )) ) {
    error(F("Could not set device name?"));
  }

  Serial.println(F("Setting device name to 'Bluefruit Keyboard SPI': "));
  if (! ble_spi.sendCommandCheckOK(F( "AT+GAPDEVNAME=Bluefruit Keyboard SPI" )) ) {
    error(F("Could not set device name?"));
  }

  /* Enable HID Service */
  Serial.println(F("Enable UART HID Service (including Keyboard): "));
  if ( ble_uart.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    if ( !ble_uart.sendCommandCheckOK(F( "AT+BleHIDEn=On" ))) {
      error(F("Could not enable Keyboard"));
    }
  }else
  {
    if (! ble_uart.sendCommandCheckOK(F( "AT+BleKeyboardEn=On"  ))) {
      error(F("Could not enable Keyboard"));
    }
  }

  Serial.println(F("Enable SPI HID Service (including Keyboard): "));
  if ( ble_spi.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    if ( !ble_spi.sendCommandCheckOK(F( "AT+BleHIDEn=On" ))) {
      error(F("Could not enable Keyboard"));
    }
  }else
  {
    if (! ble_spi.sendCommandCheckOK(F( "AT+BleKeyboardEn=On"  ))) {
      error(F("Could not enable Keyboard"));
    }
  }

  /* Add or remove service requires a reset */
  Serial.println(F("Performing a SW reset UART (service changes require a reset): "));
  if (! ble_uart.reset() ) {
    error(F("Couldn't reset??"));
  }
  
  Serial.println(F("Performing a SW reset SPI (service changes require a reset): "));
  if (! ble_spi.reset() ) {
    error(F("Couldn't reset??"));
  }
}

void sendKeysBLE(char keys[]) {
  
  // Send to UART
  ble_uart.print( F("AT+BleKeyboard=") );
  ble_uart.println(keys);

  if( ble_uart.waitForOK() )
  {
    Serial.println( F("OK!") );
  }else
  {
    Serial.println( F("FAILED!") );
  }

  // Send to SPI
  ble_spi.print( F("AT+BleKeyboard=") );
  ble_spi.println(keys);

  if( ble_spi.waitForOK() )
  {
    Serial.println( F("OK!") );
  }else
  {
    Serial.println( F("FAILED!") );
  }
}

char* randomEmoji() {
  return(emojis[random(sizeof(emojis)/sizeof(char*))]);
}


void spinningRainbow() {
  static byte initialHue = 0;
  initialHue = initialHue + 1;
  fill_rainbow(leds, NUM_LEDS, initialHue, 255 / NUM_LEDS);
}

void fillHitColour() {
  // Gold colour
  CHSV hsvval(220,210,255);
  fill_solid(leds, NUM_LEDS, hsvval);
}

void fillIdleColour() {
  CHSV hsvval(0,230,255);
  fill_solid(leds, NUM_LEDS, hsvval);
}

void setup() {

  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  
  delay(500);
  Serial.begin(115200);

  // Bluetooth setup
  setupBLE();

  // LED setup
  FastLED.addLeds<DOTSTAR, LED_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(MAX_BRIGHTNESS);
  set_max_power_in_volts_and_milliamps(MAX_LED_VOLTS, MAX_LED_CURRENT_MA);

  currentPalette  = CRGBPalette16(CRGB::Black);
  targetPalette   = RainbowColors_p;
  currentBlending = LINEARBLEND;

  // SoundFX setup
  setupSoundfx();
  
  // Hit detection setup
  resetTimer();
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), hitHandler, RISING);
}

void loop() {
  
  if (hitInterruptCounter == 2) {
    
    // Run once after interrupt

    fillHitColour();
    show_at_max_brightness_for_power();

    playSoundfx();
    
    sendKeysBLE(randomEmoji());

    resetTimer();
    hitInterruptCounter--;
    
  } else if (hitInterruptCounter == 1) {

    // Run while waiting for dead time to end
    
    if (getTimerElapsed() > WAIT_TIME_MS) {
      
      // Run once after dead time
            
      hitInterruptCounter--;
    }
  } else {

    // Run while waiting for interrupt

    EVERY_N_MILLISECONDS(LED_LOOP_DELAY_MS) {
      spinningRainbow();
      //fillIdleColour();
    }
    
    show_at_max_brightness_for_power();
    
  }
  
}


// Hit interrupt handler
void hitHandler() {
  if (hitInterruptCounter == 0) {
    hitInterruptCounter = 2;
  }
}


