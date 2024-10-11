#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
WebServer server(80);

const int ledPin = 2;  // Built-in LED on most ESP32 dev boards
String ssid = "";
String password = "";

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  EEPROM.begin(512);
  loadCredentials();

  if (setupWiFi()) {
    // Connected to WiFi, set up normal operation
    server.on("/", handleRoot);
    server.on("/on", handleOn);
    server.on("/off", handleOff);
  } else {
    // Failed to connect, set up AP mode
    setupAP();
  }

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    dnsServer.processNextRequest();
  }
  server.handleClient();
}

bool setupWiFi() {
  if (ssid == "" || password == "") {
    return false;
  }

  WiFi.begin(ssid.c_str(), password.c_str());
  for (int i = 0; i < 20; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  return false;
}

void setupAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("ESP32-Setup");

  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", handleConfig);
  server.on("/save", handleSave);
}

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>ESP32 LED Control</h1>";
  html += "<p><a href='/on'><button>Turn On</button></a></p>";
  html += "<p><a href='/off'><button>Turn Off</button></a></p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleOn() {
  digitalWrite(ledPin, HIGH);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleOff() {
  digitalWrite(ledPin, LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleConfig() {
  String html = "<html><body>";
  html += "<h1>ESP32 WiFi Setup</h1>";
  html += "<form action='/save' method='post'>";
  html += "SSID: <input type='text' name='ssid'><br>";
  html += "Password: <input type='password' name='password'><br>";
  html += "<input type='submit' value='Save'>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleSave() {
  ssid = server.arg("ssid");
  password = server.arg("password");
  saveCredentials();
  
  server.send(200, "text/plain", "Credentials saved. ESP32 will restart and attempt to connect.");
  delay(2000);
  ESP.restart();
}

void loadCredentials() {
  ssid = EEPROM.readString(0);
  password = EEPROM.readString(50);
}

void saveCredentials() {
  EEPROM.writeString(0, ssid);
  EEPROM.writeString(50, password);
  EEPROM.commit();