#include <ESP8266WiFi.h>
// #include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
// #include <Ticker.h>
#include "ESPDMX.h"
#include "FastLED.h"

#define NUM_LEDS    9
#define NUM_VIRTUAL_LEDS 9
#define LED_TYPE    DMXESPSERIAL
#define COLOR_ORDER RGB
#define BRIGHTNESS  100
#define SPEED       1

#define OFF_PATTERN     0
#define DEFAULT_PATTERN 1
#define RAINBOW_PATTERN 1
#define WHITE_PATTERN   2
#define TEST_PATTERN    3

#define NO_OVERLAY      0
#define PULSE_OVERLAY   1
#define GLITTER_OVERLAY 2

CRGB rawleds[NUM_LEDS];

// diagonal strips are on
// DMX controller A channels 1-12
// DMX controller B channels 31-42
// DMX controller C channels 61-72
const int diagonals[NUM_VIRTUAL_LEDS] {
    0,1,2,3,4,5,6,7,8
    // 0,  // 1
    // 1,  // 2
    // 2,  // 3
    // 3,  // 4
    // 10, // 5
    // 11, // 6
    // 12, // 7
    // 13, // 8
    // 20, // 9
    // 21, // 10
    // 22, // 11
    // 23, // 12
};

// square strips are on
// DMX controller A channels 13-24
// DMX controller B channels 43-54
// DMX controller C channels 73-84
// keep in reverse order to go opposite direction in rainbow pattern
const int squares[NUM_VIRTUAL_LEDS] {
    27, // 12
    26, // 11
    25, // 10
    24, // 9
    17, // 8
    // 16, // 7
    // 15, // 6
    // 14, // 5
    // 7,  // 4
    // 6,  // 3
    // 5,  // 2
    // 4,  // 1
};

// apex strip is on DMX controller A channels 25-27
CRGB apex[1] = rawleds[8];

const char *ssid = "GCSB surveilance WiFi";
const char *password = "god of light";

// global brightness, fps, speed, hueSpan, hue and the currentLed
unsigned int brightness = BRIGHTNESS;  // 0 - 255
unsigned int fps = 60;                 // 1 - 255
unsigned int speed = SPEED;            // 1 - 255
unsigned int hueSpan = NUM_VIRTUAL_LEDS;
unsigned int chanceOfGlitter = 80;
byte hue = 0;
byte currentLed = 0;

const byte DNS_PORT = 53;
IPAddress apIP(10, 10, 10, 10);
DNSServer dnsServer;
ESP8266WebServer server(80);

// Ticker coloring;

// bool TickerAttached = false;

// init functions
void WIFIinit();
void DNSinit();
void HTTPServerinit();
void LEDinit();

// pattern functions
void test();
void rainbow();
void rainbowWithPulse();
void rainbowWithGlitter();
void blackout();
void white();

// overlay pattern functions
void no_overlay();
void pulse();
void glitter();

// HTTP endpoint functions
void on();
void off();

void changeBrightness();
void changeFPS();
void changeSpeed();
void changeHueSpan();
void changeGlitterChance();

void changePattern();
void patternWhite();
void patternRainbow();
void patternTest();
void patternOff();

void changeOverlay();
void overlayPulse();
void overlayGlitter();
void overlayOff();

void fill_rainbow_virtual(
    struct CRGB * pFirstLED,
    int numToFill,
    const int *virtualleds,
    uint8_t initialhue,
    uint8_t deltahue
);

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { blackout, rainbow, white, test };
SimplePatternList gOverlays = { no_overlay, pulse, glitter };
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gCurrentOverlayNumber = 0; // Index number of which overlay is current
uint8_t gHue = 0;   // rotating "base color" used by many of the patterns

void setup() {
  delay(1000);

  Serial.begin(9600);
  Serial.println();

  WIFIinit();
  DNSinit();
  HTTPServerinit();
  LEDinit();
}

void loop() {
  uint16_t delay = 0;
  dnsServer.processNextRequest();
  server.handleClient();

  // Call the current pattern and overlay function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();
  gOverlays[gCurrentOverlayNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/fps);

  // do some periodic updates
  // slowly cycle the "base color" through the rainbow
  EVERY_N_MILLISECONDS( 1 ) { gHue += speed; }
  // return to default pattern and no overlay after a few seconds
  // EVERY_N_SECONDS( 4 ) { gCurrentPatternNumber = DEFAULT_PATTERN; }
  // EVERY_N_SECONDS( 4 ) { gCurrentOverlayNumber = NO_OVERLAY; }
}

/*
    init functions
*/
void WIFIinit() {
    Serial.println("Configuring access point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(ssid, password);

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("IP address:");
    Serial.println(myIP);

    WiFi.printDiag(Serial);
}

void DNSinit() {
    dnsServer.start(DNS_PORT, "temple.io", apIP);
}

void HTTPServerinit() {
    server.on("/on", on);
    server.on("/off", off);
    server.on("/pattern", changePattern);
    server.on("/pattern/off", patternOff);
    server.on("/pattern/white", patternWhite);
    server.on("/pattern/rainbow", patternRainbow);
    server.on("/test", patternTest);
    server.on("/overlay", changeOverlay);
    server.on("/overlay/off", overlayOff);
    server.on("/overlay/pulse", overlayPulse);
    server.on("/overlay/glitter", overlayGlitter);
    server.on("/fps", changeFPS);
    server.on("/speed", changeSpeed);
    server.on("/brightness", changeBrightness);
    server.on("/huespan", changeHueSpan);
    server.on("/glitter/chance", changeGlitterChance);
    server.begin();
    Serial.println("HTTP server started");
}

void LEDinit() {
  FastLED.addLeds<LED_TYPE, RGB>(rawleds, NUM_LEDS);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);
  blackout();
}

/*
    pattern functions
*/
void rainbow() {
    // default temple rainbow pattern:
    // very slow rotating colour wheel on 12 apex strips
    fill_rainbow_virtual(rawleds, NUM_VIRTUAL_LEDS, diagonals, gHue, 255/hueSpan);

    // very slow rotating colour wheel on 12 square strips opposite direction
    // fill_rainbow_virtual(rawleds, NUM_VIRTUAL_LEDS, squares, gHue, 255/12);

    // rainbow colours for apex circle with same hue delta as 12 strips
    // fill_rainbow(apex, 1, gHue, 255/12);
}

void blackout() {
  fill_solid(rawleds, NUM_LEDS, CRGB::Black);
}

void white() {
  fill_solid(rawleds, NUM_LEDS, CRGB::White);
}

void test() {
    static uint8_t led = 0;
    static uint8_t colour = 0;

    switch(colour) {
        case 0: { rawleds[led] = CRGB::Red; break; }
        case 1: { rawleds[led] = CRGB::Green; break; }
        case 2: { rawleds[led] = CRGB::Blue; break; }
    }

    colour++;
    if (colour % 3 == 0) { led++; colour = 0; }
    if (led >= NUM_LEDS) { led = 0; }

    FastLED.delay(1000 - (1000 * speed/255));
}

/*
    overlay pattern functions
*/
void no_overlay() {}

void pulse() {

}

void glitter() {
    if (random8() < chanceOfGlitter) {
        rawleds[ random16(NUM_LEDS) ] += CRGB::White;
    }
}

/*
    HTTP endpoint functions
*/
void on() {
    gCurrentPatternNumber = DEFAULT_PATTERN;
    gCurrentOverlayNumber = NO_OVERLAY;
    speed = 1;
    hueSpan = NUM_VIRTUAL_LEDS;
    gHue = 0;
    server.send(200, "text/html", "<h1>ON</h1>");
}

void off() {
    gCurrentPatternNumber = OFF_PATTERN;
    gCurrentOverlayNumber = NO_OVERLAY;
    server.send(200, "text/html", "<h1>OFF</h1>");
}

void changeBrightness() {
    // grab brightness from GET /brightness&value=<0-255>
    brightness = server.arg("value").toInt();

    if (brightness >= 0 || brightness <= 255) {
        FastLED.setBrightness(brightness);
        server.send(200, "text/html", "<h1>brightness: " + String(brightness) + "</h1>");
    }
}

void changeFPS() {
    // grab fps from GET /fps&value=<1-255>
    fps = server.arg("value").toInt();
    if (fps == 0) { fps = 1; }
    server.send(200, "text/html", "<h1>fps: " + String(fps) + "</h1>");
}

void changeSpeed() {
    // grab speed from GET /speed&value=<1-255>
    speed = server.arg("value").toInt();
    if (speed == 0) { speed = 1; }
    server.send(200, "text/html", "<h1>speed: " + String(speed) + "</h1>");
}

void changeHueSpan() {
    // grab huespan from GET /huespan&value=<1-255>
    hueSpan = server.arg("value").toInt();
    if (hueSpan == 0) { hueSpan = 1; }
    server.send(200, "text/html", "<h1>hueSpan: " + String(hueSpan) + "</h1>");
}

void changeGlitterChance() {
    // grab value from GET /glitter/chance&value=<1-255>
    chanceOfGlitter = server.arg("value").toInt();
    server.send(200, "text/html", "<h1>chanceOfGlitter: " + String(chanceOfGlitter) + "</h1>");
}

void changePattern() {
    gCurrentPatternNumber = server.arg("value").toInt();
    server.send(200, "text/html", "<h1>pattern: " + String(gCurrentPatternNumber) + "</h1>");
}

void patternOff() {
    gCurrentPatternNumber = OFF_PATTERN;
    server.send(200, "text/html", "<h1>pattern: OFF</h1>");
}

void patternWhite() {
    gCurrentPatternNumber = WHITE_PATTERN;
    server.send(200, "text/html", "<h1>pattern: white</h1>");
}

void patternRainbow() {
    gCurrentPatternNumber = RAINBOW_PATTERN;
    server.send(200, "text/html", "<h1>pattern: rainbow</h1>");
}

void patternTest() {
    gCurrentPatternNumber = TEST_PATTERN;
    server.send(200, "text/html", "<h1>pattern: test</h1>");
}

void changeOverlay() {
    gCurrentOverlayNumber = server.arg("value").toInt();
    server.send(200, "text/html", "<h1>overlay: " + String(gCurrentOverlayNumber) + "</h1>");
}

void overlayOff() {
    gCurrentOverlayNumber = NO_OVERLAY;
    server.send(200, "text/html", "<h1>overlay: OFF</h1>");
}

void overlayPulse() {
    gCurrentOverlayNumber = PULSE_OVERLAY;
    server.send(200, "text/html", "<h1>overlay: pulse</h1>");
}

void overlayGlitter() {
    gCurrentOverlayNumber = GLITTER_OVERLAY;
    server.send(200, "text/html", "<h1>overlay: glitter</h1>");
}

/*
    helper functions
*/
void fill_rainbow_virtual(
    struct CRGB * pFirstLED,
    int numToFill,
    const int *virtualleds,
    uint8_t initialhue,
    uint8_t deltahue
) {
    CHSV hsv;
    hsv.hue = initialhue;
    hsv.val = 255;
    hsv.sat = 240;
    for( int i = 0; i < numToFill; i++) {
        int virtualled=virtualleds[i];
        pFirstLED[virtualled] = hsv;
        hsv.hue += deltahue;
    }
}
