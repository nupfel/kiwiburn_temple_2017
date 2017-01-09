#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include "ESPDMX.h"

const char *ssid = "Kiwiburn Temple";
const char *password = "god of light";

// global brightness and speed level
unsigned int brightness = 50; // 0 - 255
unsigned int speed = 20;      // 1 - 255 ?

const byte DNS_PORT = 53;
IPAddress apIP(10, 10, 10, 10);
DNSServer dnsServer;
ESP8266WebServer server(80);

Ticker coloring;
DMXESPSerial DMX;

String responseHTML = "<h1>WELCOME!</h1><h2>You have entered a new level of experience!</h2>";

uint8_t Red[]   =   {255, 255, 0,   0,  176,173,135,135,0  ,30 ,65 ,25 ,0  ,0  ,60 ,46 ,34 ,0  ,107,245,250,238,255,255,255,255,184,255,178,255,255,208,218,128};
uint8_t Green[] = {255, 0,   255, 0,  224,216,206,206,191,144,105,25 ,250,255,179,139,139,128,142,245,250,232,255,215,165,140,134,160,34 ,0  ,20 ,32 ,112,0};
uint8_t Blue[]  =  {255, 0,   0,   255,230,230,230,235,255,255,225,112,154,127,113,87 ,34 ,0  ,35 ,220,210,170,0  ,0  ,0  ,0  ,11 ,122, 34,255,147,144,214,128};
bool TickerAttached = false;

void colorMe() {
  int Color = random(0, 33);
  DMX.write(1, Red[Color]);
  DMX.write(2, Green[Color]);
  DMX.write(3, Blue[Color]);
}

void blackout() {
  if (TickerAttached) coloring.detach();
  server.send(200, "text/html", responseHTML + "<h1> Entered DMX blackout</h1>");
  DMX.write(1, 0);
  DMX.write(2, 0);
  DMX.write(3, 0);
}

void randomize() {
  server.send(200, "text/html", responseHTML + "<h1> Entered Random</h1>");
  TickerAttached = true;
  coloring.attach(.5, colorMe);
}

void white() {
  if (TickerAttached) coloring.detach();
  server.send(200, "text/html", responseHTML + "<h1>Entered White</h1>");
  DMX.write(1, 255);
  DMX.write(2, 255);
  DMX.write(3, 255);
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
  WiFi.softAP("Kiwiburn Temple");

  dnsServer.start(DNS_PORT, "kiwiburn.temple", apIP);

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

  DMX.init();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  DMX.update();
}
