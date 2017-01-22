#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include "ESPDMX.h"
#include "FastLED.h"

#define NUM_LEDS    28
#define LED_TYPE    DMXESPSERIAL
#define COLOR_ORDER RGB
#define BRIGHTNESS  255


CRGB rawleds[NUM_LEDS];

// diagonal strips are on
// DMX controller A channels 1-12
// DMX controller B channels 31-42
// DMX controller C channels 61-72
const int diagonals[12] {
    0,  // 1
    1,  // 2
    2,  // 3
    3,  // 4
    10, // 5
    11, // 6
    12, // 7
    13, // 8
    20, // 9
    21, // 10
    22, // 11
    23, // 12
};

// square strips are on
// DMX controller A channels 13-24
// DMX controller B channels 43-54
// DMX controller C channels 73-84
// keep in reverse order to go opposite direction in rainbow pattern
const int squares[12] {
    27, // 12
    26, // 11
    25, // 10
    24, // 9
    17, // 8
    16, // 7
    15, // 6
    14, // 5
    7,  // 4
    6,  // 3
    5,  // 2
    4,  // 1
};

// apex strip is on DMX controller A channels 25-27
CRGB apex[1] = rawleds[8];

const char *ssid = "GCSB surveilance WiFi";
const char *password = "god of light";

// global brightness, speed, hue and the currentLed
unsigned int brightness = 255;  // 0 - 255
unsigned int fps = 60;
byte hue = 0;
byte currentLed = 0;

const byte DNS_PORT = 53;
IPAddress apIP(10, 10, 10, 10);
DNSServer dnsServer;
ESP8266WebServer server(80);

Ticker coloring;

bool TickerAttached = false;

void WIFIinit();
void DNSinit();
void HTTPServerinit();
void LEDinit();
void rainbow();
void addGlitter(fract8 chanceOfGlitter);
void rainbowWithGlitter();
// void confetti();
// void sinelon();
// void bpm();
// void juggle();
void blackout();
void white();
void changeFPS();
void changeBrightness();
void changePattern();
void my_fill_rainbow(
    struct CRGB * pFirstLED,
    int numToFill,
    const int *virtualleds,
    uint8_t initialhue,
    uint8_t deltahue
);

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter };
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
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

  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/fps);

  // do some periodic updates
  EVERY_N_MILLISECONDS( 1 ) { gHue++; } // slowly cycle the "base color" through the rainbow
}

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
    server.on("/off", blackout);
    server.on("/white", white);
    server.on("/pattern", changePattern);
    server.on("/fps", changeFPS);
    server.on("/brightness", changeBrightness);
    server.begin();
    Serial.println("HTTP server started");
}

void LEDinit() {
  FastLED.addLeds<LED_TYPE, RGB>(rawleds, NUM_LEDS);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);
  blackout();
}

void blackout() {
  if (TickerAttached) coloring.detach();
  fill_solid(rawleds, NUM_LEDS, CRGB::Black);
  // server.send(200, "text/html", "<h1>Entered DMX blackout</h1>");
}

void white() {
  if (TickerAttached) coloring.detach();
  fill_solid(rawleds, NUM_LEDS, CRGB::White);
  // server.send(200, "text/html", "<h1>Entered White</h1><p>brightness: " + String(brightness) + "</p>");
}

void rainbow() {
    // default temple rainbow pattern:
    // very slow rotating colour wheel on 12 apex strips
    my_fill_rainbow(rawleds, 12, diagonals, gHue, 255/12);

    // very slow rotating colour wheel on 12 square strips opposite direction
    my_fill_rainbow(rawleds, 12, squares, gHue, 255/12);

    // rainbow colours for apex circle with same hue delta as 12 strips
    fill_rainbow(apex, 1, gHue, 255/12);
}

void addGlitter( fract8 chanceOfGlitter) {
  if( random8() < chanceOfGlitter) {
    rawleds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void rainbowWithGlitter() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void changeBrightness() {
    // grab brightness from GET /brightness&value=<0-255>
    brightness = server.arg("value").toInt();
    if (brightness >= 0 || brightness <= 255) {
        FastLED.setBrightness(brightness);
        server.send(200, "text/html", "<h1>Changed Brightness: " + String(brightness) + "</h1>");
    }
}

void changeFPS() {
    // grab brightness from GET /brightness&value=<0-255>
    fps = server.arg("value").toInt();
    server.send(200, "text/html", "<h1>Changed FPS: " + String(fps) + "</h1>");
}

void changePattern() {
    gCurrentPatternNumber = server.arg("value").toInt();
    server.send(200, "text/html", "<h1>Changed Pattern: " + String(gCurrentPatternNumber) + "</h1>");
}

void my_fill_rainbow(
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
