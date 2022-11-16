#include <ESP8266WebServer.h>
#include <ADXL345_WE.h>
#include "Seeed_BMP280.h"
#include <LittleFS.h>
#include <MPU6050_tockn.h>
#include <Wire.h>
#include <SPI.h>
#include <RH_RF95.h>
// test
MPU6050 mpu6050(Wire);
ESP8266WebServer server(80);
ADXL345_WE accel(0x53);
BMP280 bmp280;
#define RFM95_CS 2
#define RFM95_RST 16
#define RFM95_INT 15
#define RFM95_FREQ 915.0
RH_RF95 rf95(RFM95_CS, RFM95_INT);
int arm_state;
float pressure;
float temp;
float altitude;
float base;
uint32_t ttag;
float angleX;
float angleY;
float angleZ;
File fd;
int count;
char buf[64];
char packet[64];
struct rpacket {int ttag; float x; float y; float z;};
void arm() {
  mpu6050.begin(ACCEL_2G,GYRO_500);
  mpu6050.calcGyroOffsets(true);
  server.send(200,"text/plain","Payload is Armed\n");
  arm_state = 1;
}
void sendpage() {
  String html = "<html><body><h1>Payload Control</h1><form action='/'>";
  html += "<p><input type='submit' name='arm' value='Arm' formaction='/arm'>";
  html += "<p><input type='submit' name='download' value='Download' formaction='/down'>";
  html += "</form></body></html>";
  server.send(200,"text/html",html);
}
void download() {
  fd = LittleFS.open("/landed.csv","r");
  server.streamFile(fd,"text/plain");
  fd.close();
}
void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  Wire.begin(4,5);
  accel.init();
  accel.setDataRate(ADXL345_DATA_RATE_100);
  accel.setRange(ADXL345_RANGE_16G);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Name","Password");
  server.on("/arm",arm);
  server.on("/down",download);
  server.on("/",sendpage);
  server.begin();
  arm_state=0;
  bmp280.init();
  pressure = bmp280.getPressure();
  temp = bmp280.getTemperature();
  base = bmp280.calcAltitude(pressure);
  ttag = millis() + 10;
  count=0;
  Serial.println(arm_state);
  digitalWrite(RFM95_RST,LOW);
  delay(10);
  digitalWrite(RFM95_RST,HIGH);
  delay(10);
  while(!rf95.init()) Serial.println("radio init failed");
  if(!rf95.setFrequency(RFM95_FREQ)) Serial.println("Radio frequency incorrect");
  rf95.setTxPower(23,false);
  Serial.println("Radio ready");
}

void loop() {
  server.handleClient();
  if(millis()>=ttag){
    ttag += 10;
    xyzFloat g=accel.getGValues();
    pressure = bmp280.getPressure();
    temp = bmp280.getTemperature();
    altitude = bmp280.calcAltitude(pressure);
    mpu6050.update();
    angleX = mpu6050.getAngleX();
    angleY = mpu6050.getAngleY();
    angleZ = mpu6050.getAngleZ();
    switch(arm_state) {
      case 1:
        fd=LittleFS.open("data.csv","w");
        base = altitude;
        WiFi.mode(WIFI_OFF);
        arm_state = 2;
        break;
      case 2:
        if(altitude > (base + 5)) arm_state = 3;
        break;
      case 3:
        altitude = altitude - base;
        snprintf(buf,64,"%ld,%.3f,%.3f,%.3f,%.3f,%.3f\n",ttag,altitude,g.z,angleX,angleY,angleZ);
        fd.print(buf);
        snprintf(packet,64,"%ld,%.1f,%.1f,%.1f",ttag,g.x,g.y,g.z);
        rf95.send((uint8_t*)packet,64);
        count ++;
        if(count == 10000) arm_state = 4;
        break;
      case 4:
        fd.close();
        delay(1000);
        LittleFS.rename("data.csv","landed.csv");
        delay(1000);
        Serial.println("file renamed");
        arm_state = 0;
        ESP.deepSleep(20e6);
        break;
    }
  }
}
