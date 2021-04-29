#include <EEPROM.h>

#include <ESP8266SSDP.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <FS.h>
#include <Adafruit_INA219.h>
#include <WiFiManager.h>  
#include <DNSServer.h>

Adafruit_INA219 ina219;
ESP8266WebServer server ( 80 );

void handleMeasurement() {
    String measurementSite = "<html><header><meta http-equiv=\"refresh\" content=\"5\"></header><body>";
    String json = "{";
    String fmt = server.arg("fmt");
    
    float current_mA = ina219.getCurrent_mA();
    float power_mW = ina219.getPower_mW();
    float supply_v = ina219.getBusVoltage_V();
    float shunt_v = ina219.getShuntVoltage_mV();
    float load_v = supply_v + (shunt_v / 1000);
   
    if (fmt=="json") {
      json += "\"current\":\"" + String(current_mA) + "\",";
      json += "\"power\":\"" + String(power_mW) + "\",";
      json += "\"supply_voltage\":\"" + String(supply_v) + "\",";
      json += "\"load_voltage\":\"" + String(load_v) + "\"}";
      server.send(200, "application/json", json);
    } else {
      measurementSite += "Current: " + String(current_mA) + "mA</br>";
      measurementSite += "Power: " + String(power_mW) + "mW</br>";
      measurementSite += "Supply Voltage: " + String(supply_v) + "V</br>";
      measurementSite += "Load Voltage: " + String(load_v) + "V</br>";
      measurementSite += "</body></html>";
      server.send(200, "text/html", measurementSite);
    }
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}

void setup() {
  WiFiManager wm;
  Serial.begin(115200);
  SPIFFS.begin();

  wm.autoConnect("powerflow");
  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("powerflow")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
  }

  server.on("/measure", handleMeasurement);
  server.on("/", [](){
    File file = SPIFFS.open("/index.htm","r");
    size_t sent = server.streamFile(file,"text/html");
    file.close();
  });
  
  server.on("/style.css", [](){
    File file = SPIFFS.open("/style.css","r");
    size_t sent = server.streamFile(file,"text/css");
    file.close();
  });
  server.on("/chart.js", [](){
    File file = SPIFFS.open("/chart.js","r");
    size_t sent = server.streamFile(file,"text/css");
    file.close();
  });
  
  server.onNotFound(handleNotFound);

  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }

  server.begin();
  Serial.println("HTTP server started");

}

void loop() {
  MDNS.update();
  server.handleClient();
}
