#include <ESP8266WebServer.h>
#include <Wire.h>
#include <SPI.h>
#include <RH_RF95.h>
ESP8266WebServer server(80);
#define RFM95_CS 2
#define RFM95_RST 16
#define RFM95_INT 15
#define RFM95_FREQ 915.0
RH_RF95 rf95(RFM95_CS, RFM95_INT);
int ttag;
float xaccel;
float yaccel;
float zaccel;
float xvel;
float yvel;
float zvel;
float xpos;
float ypos;
float zpos;
float tilta;
float pana;
float pad_range;
int packetnum;
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("GroundCamera","MSU2026");
  server.begin();
  packetnum = 0;
  digitalWrite(RFM95_RST,LOW);
  delay(10);
  digitalWrite(RFM95_RST,HIGH);
  delay(10);
  while(!rf95.init()) Serial.println("radio init failed!");
  if(!rf.setFrequency(RFM_95)) Serial.println("Wrong radio freq!");
  rf95.setTxPower(23,false);
  Serial.println("Radio Ready");
}
void loop() {
  server.handleClient();
  if(rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if(rf95.recv(buf, &len)) {
      
    }
  }
}
