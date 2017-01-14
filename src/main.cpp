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
CRGB leds[NUM_LEDS];

const char *ssid = "Kiwiburn Temple";
const char *password = "god of light";

// global brightness, speed, hue and the currentLed
unsigned int brightness = 50; // 0 - 255
unsigned int speed = 20;      // 1 - 255 ?
byte hue = 0;
byte currentLed = 0;

const byte DNS_PORT = 53;
IPAddress apIP(10, 10, 10, 10);
DNSServer dnsServer;
ESP8266WebServer server(80);

Ticker coloring;

String responseHTML = "<h1>WELCOME!</h1><h2>You have entered a new level of experience!</h2>";

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



void colorMe() {
  int Color = random(0, 33);
}

void blackout() {
  if (TickerAttached) coloring.detach();
  server.send(200, "text/html", responseHTML + "<h1> Entered DMX blackout</h1>");
}

void randomize() {
  server.send(200, "text/html", responseHTML + "<h1> Entered Random</h1>");
  TickerAttached = true;
  coloring.attach(.5, colorMe);
}

void white() {
  if (TickerAttached) coloring.detach();
  server.send(200, "text/html", responseHTML + "<h1>Entered White</h1>");
}

void rainbow() {
    // default temple rainbow pattern:
    // very slow rotating colour wheel on 12 apex strips
    // very slow rotating colour wheel on 12 square strips opposite direction
    // rainbow colours for apex circle
}

void changeBrightness() {
    // grab brightness from GET /brightness/<value> or POST /brightness
    // brightness = value;
}

void changeSpeed() {
    // grab brightness from GET /speed/<value> or POST /speed
    // speed = value;
}

void setup() {
  delay(1000);
  Serial.begin(9600);
  Serial.println();
  Serial.print("Configuring access point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);

  dnsServer.start(DNS_PORT, "temple.io", apIP);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address:");
  Serial.println(myIP);
  server.on("/off", blackout);
  server.on("/white", white);
  server.on("/random", randomize);
  server.on("/rainbow", rainbow);
  server.on("/speed", changeSpeed);
  server.on("/brightness", changeBrightness);
  server.begin();
  Serial.println("HTTP server started");




  // DMXinit();
}

void loop() {
  uint16_t delay = 0;
  dnsServer.processNextRequest();
  server.handleClient();
  //DMX.update();

  delay = sequenceHueShift();
  FastLED.show();
  FastLED.delay(delay);
}
