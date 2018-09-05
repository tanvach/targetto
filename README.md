# Targetto
Extreme pair programming target!

# System
- Adafruit M0 Feather Bluefruit LE (controller + SPI bluetooth module)
- Adafruit Bluefruit LE UART Friend
- Sparkfun sound detector board (with potentiometer to adjust gain)
- Adafruit Sound FX Mini sound board
- Adafruit DotStar LED strip

# How to install firmware
- Install Arduino environment
- Install Adafruit Feather board support
- Install Bluefruit LE support from Adafruit
- Install FastLED library
- Open firmware.ino in Arduino environment
- Compile and upload via USB

# How to upload sound fx
- Connect Sound FX mini sound board to computer via USB
- Copy up to 10 wav or ogg files. Make sure they are named T00RANDn.wav or T00RANDn.ogg, where n = 0 to 9
- Make sure to eject the drive before disconnecting. Otherwise the sound files may not be recognized by the Sound FX mini

# Layout

![](img/layout.png?raw=true "Layout")
