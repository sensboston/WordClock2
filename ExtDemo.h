#define FPS 120
#define COOLING 55
#define SPARKING 120
#define BRIGHTNESS 250

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;
// Array of temperature readings at each simulation cell
uint8_t heat[NUM_LEDS];
bool gReverseDirection = false;
CRGBPalette16 currentPalette;
TBlendType currentBlending;
uint8_t gCurrentPatternNumber = 0;  // Index number of which pattern is current
uint8_t gHue = 0;                   // rotating "base color" used by many of the patterns
uint8_t startIndex = 0;
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void rainbow()
{
    // FastLED's built-in rainbow generator
    fill_rainbow(strip, NUM_LEDS, gHue, 7);
}

void addGlitter(fract8 chanceOfGlitter)
{
    if (random8() < chanceOfGlitter)
    {
        strip[random16(NUM_LEDS)] += CRGB::White;
    }
}

void rainbowWithGlitter()
{
    // built-in FastLED rainbow, plus some random sparkly glitter
    rainbow();
    addGlitter(80);
}

void confetti()
{
    // random colored speckles that blink in and fade smoothly
    fadeToBlackBy(strip, NUM_LEDS, 10);
    int pos = random16(NUM_LEDS);
	strip[pos] += CHSV(gHue + random8(64), 200, 255);
}

void sinelon()
{
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy(strip, NUM_LEDS, 20);
    int pos = beatsin16(13, 0, NUM_LEDS - 1);
    strip[pos] += CHSV(gHue, 255, 192);
}

void bpm()
{
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
    for (int i = 0; i < NUM_LEDS; i++)
    {
        strip[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
}

void juggle() 
{
    // eight colored dots, weaving in and out of sync with each other
    fadeToBlackBy(strip, NUM_LEDS, 20);
    uint8_t dothue = 0;
    for (int i = 0; i < 8; i++)
    {
        strip[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, 255);
        dothue += 32;
    }
}

void rainbowSripe()
{
    currentPalette = RainbowStripeColors_p;
    currentBlending = NOBLEND;
}

void rainbowSripeBlend()
{
    currentPalette = RainbowStripeColors_p;
    currentBlending = LINEARBLEND;
}

void purpleAndGreen()
{
    CRGB purple = CHSV(HUE_PURPLE, 255, 255);
    CRGB green = CHSV(HUE_GREEN, 255, 255);
    CRGB black = CRGB::Black;

    currentPalette = CRGBPalette16(green, green, black, black,
        purple, purple, black, black,
        green, green, black, black,
        purple, purple, black, black);
    currentBlending = LINEARBLEND;
}

void blackAndWhiteStriped()
{
    // 'black out' all 16 palette entries...
    fill_solid(currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    currentBlending = NOBLEND;
}

void blackAndWhiteStripedBlend()
{
    blackAndWhiteStriped();
    currentBlending = LINEARBLEND;
}

void clouds()
{
    currentPalette = CloudColors_p;
    currentBlending = LINEARBLEND;
}

void party()
{
    currentPalette = PartyColors_p;
    currentBlending = LINEARBLEND;
}

void redWhiteBlue()
{
    currentPalette = myRedWhiteBluePalette_p;
    currentBlending = NOBLEND;
}

void redWhiteBlueBlend()
{
    currentPalette = myRedWhiteBluePalette_p;
    currentBlending = LINEARBLEND;
}

void FillLEDsFromPaletteColors(uint8_t colorIndex)
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        strip[i] = ColorFromPalette(currentPalette, colorIndex, BRIGHTNESS, currentBlending);
        colorIndex += 3;
    }
}

const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,

    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,

    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};

// List of patterns to cycle through.  Each is defined as a separate function below
typedef void(*SimplePatternList[])();
SimplePatternList gPatterns = { 
    rainbowWithGlitter, confetti, sinelon, juggle, bpm, rainbowSripe, purpleAndGreen,
    blackAndWhiteStriped, rainbowSripeBlend, clouds, party, redWhiteBlueBlend,
};

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
	gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

void processDemo()
{
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();

    if (gCurrentPatternNumber > 5)
    {
        startIndex++;
        FillLEDsFromPaletteColors(startIndex);
    }
    else if (gCurrentPatternNumber == 0) startIndex = 0;
    // send the 'leds' array out to the actual LED strip
    FastLED.show();
    // insert a delay to keep the framerate modest
    FastLED.delay(1000 / FPS);
    // slowly cycle the "base color" through the rainbow
    EVERY_N_MILLISECONDS(20) gHue++;
    // change patterns every 10 seconds
    EVERY_N_SECONDS(10) nextPattern();
}
