#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include "ESPDMX.h"
#include "FastLED.h"

#define NUM_LEDS    25
#define LED_TYPE    DMXESPSERIAL
#define COLOR_ORDER RGB
#define BRIGHTNESS 255
#define FRAMES_PER_SECOND  60 //maybe too fast


CRGB leds[NUM_LEDS];
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

const char *ssid = "Kiwiburn Temple";
const char *password = "god of light";

// global brightness, speed, hue and the currentLed
unsigned int brightness = 255; // 0 - 255
unsigned int speed = 20;      // 1 - 255 ?
byte hue = 0;
byte currentLed = 0;

const byte DNS_PORT = 53;
IPAddress apIP(10, 10, 10, 10);
DNSServer dnsServer;
ESP8266WebServer server(80);

Ticker coloring;

bool TickerAttached = false;

void DMXinit(){
  FastLED.addLeds<LED_TYPE, RGB>(leds, NUM_LEDS);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);
}

uint16_t sequenceHueShift() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  leds[currentLed] = CHSV(hue, 255, 255);
  hue++;
  currentLed++;
  if (currentLed >= NUM_LEDS) {
    currentLed = 0;
  }
  return 120;
}

void blackout() {
  if (TickerAttached) coloring.detach();
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  server.send(200, "text/html", "<h1>Entered DMX blackout</h1>");
}

void white() {
  if (TickerAttached) coloring.detach();
  fill_solid(leds, NUM_LEDS, CRGB::White);
  server.send(200, "text/html", "<h1>Entered White</h1><p>brightness: " + String(brightness) + "</p>");
}

void rainbow() {
    // default temple rainbow pattern:
    // very slow rotating colour wheel on 12 apex strips
    // very slow rotating colour wheel on 12 square strips opposite direction
    // rainbow colours for apex circle
    // Currently just rainbows though all strips
    fill_rainbow( leds, NUM_LEDS, gHue, 1); // FastLED's built-in rainbow generator
}

void changeBrightness() {
    // grab brightness from GET /brightness/<value> or POST /brightness
    FastLED.setBrightness(brightness);
}

void changeSpeed() {
    // grab brightness from GET /speed/<value> or POST /speed
    // speed = value;
}

void setup() {
  delay(1000);
  Serial.begin(9600);
  Serial.println();
  Serial.println("Configuring access point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);

  dnsServer.start(DNS_PORT, "temple.io", apIP);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("IP address:");
  Serial.println(myIP);
  server.on("/off", blackout);
  server.on("/white", white);
  server.on("/rainbow", rainbow);
  server.on("/speed", changeSpeed);
  server.on("/brightness", changeBrightness);
  server.begin();
  Serial.println("HTTP server started");
  WiFi.printDiag(Serial);

  DMXinit();
}

void loop() {
  uint16_t delay = 0;
  dnsServer.processNextRequest();
  server.handleClient();
  //DMX.update();

  // Call the current pattern function once, updating the 'leds' array
  rainbow();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND);

  // do some periodic updates
  EVERY_N_MILLISECONDS( 1 ) { gHue++; } // slowly cycle the "base color" through the rainbow
}
