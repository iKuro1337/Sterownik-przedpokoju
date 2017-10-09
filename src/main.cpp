//##############################################################################
// SEKCJA WYWOLAN BIBLIOTEK
#include <Arduino.h>
extern "C" {
#include "user_interface.h"
}
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <lib/Adafruit_PWMServoDriver.h>
#include <lib/Adafruit_SHT31.h>
#include <lib/BH1750.h>
#include <lib/EEPROM.h>
#include <Ticker.h>
#include <WiFiUdp.h>
#include <lib/PWMlevels.h>

// KONIEC SEKCJI BIBLIOTEK
//##############################################################################

 //Debugging serial output declaration
#define DBG_OUTPUT_PORT Serial
//
#define NeopixelPIN 2

//Define network connection
const char* ssid = "Batcave";
const char* password = "Backfire_1";
const char* host = "swiatlo_przedpokoj";

//## NTP
unsigned int localPort = 2390;      // local port to listen for UDP packets
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets



int x=0,y_RED=0,y_BLUE=0,y_GREEN=0;
bool CZUJNIK_PIR,TIMER_LIGHT_OFF_FINISHED;
uint16_t CZUJNIK_LUX, LIMIT_LUX;
int TIMER_LIGHT_OFF_SETPOINT, TIMER_LIGHT_OFF_COUNTER;
int GODZINA=0,MINUTA=0,SEKUNDA=0,STREFA=0;
int s_GODZINA,s_MINUTA,s_SEKUNDA,s_STREFA,SAVE_TIME_DATA,TIME_EEPROM_WRITE_DATA,SAVE_LIGHT_DATA;
int CYCLE_TIME, CYCLE_TIME_PREV;
float CZUJNIK_TEM;
float CZUJNIK_WIL;

int s_W_START_HOUR_VAL=0,s_W_START_MIN_VAL=0,s_W_STOP_HOUR_VAL=0,s_W_STOP_MIN_VAL=0,s_RGB_START_HOUR_VAL=0,s_RGB_START_MIN_VAL=0,s_RGB_STOP_HOUR_VAL=0,s_RGB_STOP_MIN_VAL=0,RGBW_EEPROM_WRITE_DATA=0;
int W_START_HOUR_VAL=0,W_START_MIN_VAL=0,W_STOP_HOUR_VAL=0,W_STOP_MIN_VAL=0,RGB_START_HOUR_VAL=0,RGB_START_MIN_VAL=0,RGB_STOP_HOUR_VAL=0,RGB_STOP_MIN_VAL=0;
int LED_W_RELEASE=0,LED_RGB_RELEASE=0;

uint8_t s_RED=0,s_GREEN=0,s_BLUE=0;
uint32_t s_tgtLUX=0;
bool s_LEDON=0,s_SYNC=0,s_CWIPE_ON=0;

String XML;

// KONIEC SEKCJI ZMIENNYCH I STAŁYCH GLOBALNYCH
//##############################################################################


ESP8266WebServer server(80);
//holds the current upload
File fsUploadFile;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(20, NeopixelPIN, NEO_BRG + NEO_KHZ400);

// PWM control for PCA9685
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

Adafruit_SHT31 sht31 = Adafruit_SHT31();

BH1750 lightMeter(0x23);

WiFiUDP udp;

Ticker PWM_CONTROL_ON;
Ticker PWM_CONTROL_OFF;
Ticker RGB_RED_CONTROL_ON;
Ticker RGB_GREEN_CONTROL_ON;
Ticker RGB_BLUE_CONTROL_ON;
Ticker RGB_CONTROL_OFF;
Ticker TIMER_LIGHT_OFF;

Ticker SECOND;


//##############################################################################
//OTA SECTION
//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}

  String path = server.arg("dir");
  DBG_OUTPUT_PORT.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }

  output += "]";
  server.send(200, "text/json", output);
}

//END OF OTA SECTION
//##############################################################################

//##############################################################################
//UDP/NTP SECTION

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address){
  //Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void SYNCHRONIZACJA_NTP(){
  WiFi.hostByName(ntpServerName, timeServerIP);

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);

  int cb = udp.parsePacket();
  int synchro_timeout=0;
  while(!cb) {
    if (synchro_timeout >19) break;
    Serial.println("no packet yet");
    cb = udp.parsePacket();
    delay(500);
    if(s_SYNC){
      strip.setPixelColor(synchro_timeout, strip.Color(0,0,255));
      strip.show();
    }
    ArduinoOTA.handle();
    synchro_timeout++;
  }
  if(cb)
  {
    //Serial.printf("packet received, length=%02d",cb);
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    //Serial.print("Seconds since Jan 1 1900 = " );
    //Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    //Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);


    // print the hour, minute and second:
    //Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    //Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    GODZINA=(epoch  % 86400L) / 3600 + STREFA-10;

    if(GODZINA>=24)GODZINA=GODZINA-24;//jezeli po dodaniu strefy mamy po polnocy, zacznij liczyc do przodu (nast dzien)
    if(GODZINA<0)GODZINA=24+GODZINA;//jezeli po dodaniu strefy mamy ujemna godzine, zacznij liczyc do tylu (poprz dzien)

    //Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      //Serial.print('0');
    }
    //Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    MINUTA=(epoch  % 3600) / 60;
    //Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      //Serial.print('0');
    }
    //Serial.println(epoch % 60); // print the second
    SEKUNDA=epoch % 60;
    cb=0;
  }
  s_SYNC=0;
}

// END OF UDP/NTP SECTION
//##############################################################################

//##############################################################################
//NEOPIXEL SECTION
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void colorClear(){
  for(uint16_t i=0; i<strip.numPixels(); i++)
  {
    strip.setPixelColor(i, 0);
    strip.show();
    ArduinoOTA.handle();
  }
}

void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    ArduinoOTA.handle();
    delay(wait);
  }
}

void colorWipe2w(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels()/2+1; i++) {
    strip.setPixelColor(i, c);
    strip.setPixelColor(strip.numPixels()-i,c);
    strip.show();
    ArduinoOTA.handle();
    delay(wait);
  }
}

void colorFillMiddle(uint32_t c, uint8_t wait) {
  int middle = strip.numPixels();



  for(uint16_t i=0; i<strip.numPixels()/2+1; i++) {
    strip.setPixelColor(i, c);
    strip.setPixelColor(strip.numPixels()-i,c);
    strip.show();
    ArduinoOTA.handle();
    delay(wait);
  }
}

uint16_t i_rainbow, j_rainbow;
// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {

  //for(j=0; j<256; ) { // 1 cycles of all colors on wheel
    for(i_rainbow=0; i_rainbow< strip.numPixels(); i_rainbow++) {
      strip.setPixelColor(i_rainbow, Wheel(((i_rainbow * 256 / strip.numPixels()) + j_rainbow) & 255));
      ArduinoOTA.handle();
      if(!s_LEDON)return;
    }
    if(j_rainbow>255){
      j_rainbow=0;
    }
    else{
      j_rainbow++;
    }
    strip.show();
    delay(wait);
  //}
}

//END OF NEOPIXEL SECTION
//##############################################################################

//##############################################################################
// STEROWANIE OSWIETLENIEM PLYNNE
void PWM_ON(){
  if(CZUJNIK_LUX < s_tgtLUX and CZUJNIK_PIR and x<1023)
  {
    pwm.setPWM(0, 0 , gamma12[x]);
    x++;
  }
}

void PWM_OFF(){
  if((x > 0) and ((CZUJNIK_LUX > s_tgtLUX) or ((!CZUJNIK_PIR) and (TIMER_LIGHT_OFF_FINISHED))))
  {
    pwm.setPWM(0, 0 , gamma12[x]);
    x--;
  }
}

void RGB_RED_ON(){
  if(CZUJNIK_LUX < s_tgtLUX and CZUJNIK_PIR and y_RED<s_RED and CZUJNIK_LUX < LIMIT_LUX and !s_LEDON )
  {
    for(int i=0;i<strip.numPixels();i++)
    {
      strip.setPixelColor(i, strip.Color(gamma8[y_RED], (strip.getPixelColor(i) >> 8),strip.getPixelColor(i)));
    }
    strip.show();
    y_RED++;
  }
}

void RGB_GREEN_ON(){
  if(CZUJNIK_LUX < s_tgtLUX and CZUJNIK_PIR and y_GREEN<s_GREEN and CZUJNIK_LUX < LIMIT_LUX and !s_LEDON )
  {
    for(int i=0;i<strip.numPixels();i++)
    {
      strip.setPixelColor(i, strip.Color(strip.getPixelColor(i) >> 16, gamma8[y_GREEN],strip.getPixelColor(i)));
    }
    strip.show();
    y_GREEN++;
  }
}

void RGB_BLUE_ON(){
  if(CZUJNIK_LUX < s_tgtLUX and CZUJNIK_PIR and y_BLUE<s_BLUE and CZUJNIK_LUX < LIMIT_LUX and !s_LEDON )
  {
    for(int i=0;i<strip.numPixels();i++)
    {
      strip.setPixelColor(i, strip.Color(strip.getPixelColor(i) >> 16, strip.getPixelColor(i) >> 8,gamma8[y_BLUE]));
    }
    strip.show();
    y_BLUE++;
  }
}

void RGB_OFF(){
  if( (y_RED > 0) and ((CZUJNIK_LUX > LIMIT_LUX) or (!CZUJNIK_PIR and TIMER_LIGHT_OFF_FINISHED)) and !s_LEDON )
  {
    for(int i=0;i<strip.numPixels();i++)
    {
      strip.setPixelColor(i, strip.Color(gamma8[y_RED],gamma8[y_GREEN],gamma8[y_BLUE]));
    }
    strip.show();
    if(y_RED>0)y_RED--;
    if(y_GREEN>0)y_GREEN--;
    if(y_BLUE>0)y_BLUE--;
  }
}

void TIMER_LIGHT_OFF_CNT(){
  if(!CZUJNIK_PIR){
    if(TIMER_LIGHT_OFF_COUNTER<TIMER_LIGHT_OFF_SETPOINT)
    {
      TIMER_LIGHT_OFF_COUNTER++;
    }
    else
    {
      TIMER_LIGHT_OFF_FINISHED=true;
    }
  }
  else
  {
    TIMER_LIGHT_OFF_FINISHED=false;
    TIMER_LIGHT_OFF_COUNTER=0;
  }
}

void SECOND_CNT(){
  if(SEKUNDA<59)
  {
    SEKUNDA++;
  }
  else
  {
    SEKUNDA=0;
    if(MINUTA<59)
    {
      MINUTA++;
    }
    else
    {
      MINUTA=0;
      if(GODZINA<23){
        GODZINA++;
        SYNCHRONIZACJA_NTP();
      }
      else
      {
        GODZINA=0;
      }
    }
  }
}

void handleESPval() {
  if(server.arg("RED")!="")s_RED = server.arg("RED").toInt();
  if(server.arg("GREEN")!="")s_GREEN = server.arg("GREEN").toInt();
  if(server.arg("BLUE")!="")s_BLUE = server.arg("BLUE").toInt();
  if(server.arg("tgtLUX")!="")s_tgtLUX = server.arg("tgtLUX").toInt();
  if(server.arg("LEDON")!="")s_LEDON = server.arg("LEDON").toInt();
  if(server.arg("SYNC")!="")s_SYNC = server.arg("SYNC").toInt();
  if(server.arg("RAINBOW")!="")s_CWIPE_ON = server.arg("RAINBOW").toInt();
  if(server.arg("SAVE_LIGHT_DATA")!="")SAVE_LIGHT_DATA = server.arg("SAVE_LIGHT_DATA").toInt();
  if(server.arg("SAVE_TIME_DATA")!="")
  {
    s_GODZINA = server.arg("s_GODZINA").toInt();
    s_MINUTA = server.arg("s_MINUTA").toInt();
    s_SEKUNDA = server.arg("s_SEKUNDA").toInt();
    s_STREFA = server.arg("s_STREFA").toInt();
    SAVE_TIME_DATA = server.arg("SAVE_TIME_DATA").toInt();
  }
  if(server.arg("TIME_EEPROM_WRITE_DATA")!="")TIME_EEPROM_WRITE_DATA = server.arg("TIME_EEPROM_WRITE_DATA").toInt();
  if(server.arg("RGBW_EEPROM_WRITE_DATA")!=""){
    W_START_HOUR_VAL = server.arg("s_W_START_HOUR_VAL").toInt();
    W_START_MIN_VAL = server.arg("s_W_START_MIN_VAL").toInt();
    W_STOP_HOUR_VAL = server.arg("s_W_STOP_HOUR_VAL").toInt();
    W_STOP_MIN_VAL = server.arg("s_W_STOP_MIN_VAL").toInt();
    RGB_START_HOUR_VAL = server.arg("s_RGB_START_HOUR_VAL").toInt();
    RGB_START_MIN_VAL = server.arg("s_RGB_START_MIN_VAL").toInt();
    RGB_STOP_HOUR_VAL = server.arg("s_RGB_STOP_HOUR_VAL").toInt();
    RGB_STOP_MIN_VAL = server.arg("s_RGB_STOP_MIN_VAL").toInt();
    RGBW_EEPROM_WRITE_DATA = server.arg("RGBW_EEPROM_WRITE_DATA").toInt();
  }
  if(server.arg("TDEL_AUTOOFF")!=""){
    TIMER_LIGHT_OFF_SETPOINT = server.arg("TDEL_AUTOOFF").toInt()*10;
  }


  server.send(200, "text/xml", XML);
}

void handleESPdata(){
  char json_c [500];
  sprintf(json_c,", \"chid\":\"0x%X\", \"flashchid\":\"0x%X\"",ESP.getChipId(),ESP.getFlashChipId());
  String str(json_c);
  String json = "{";
  json += "\"cpufreq\":\""+String(ESP.getCpuFreqMHz())+"\"";
  json += ", \"flashchrealsize\":\""+String(ESP.getFlashChipRealSize())+"\"";
  json += ", \"flashchsize\":\""+String(ESP.getFlashChipSize())+"\"";
  json += ", \"flashchspeed\":\""+String(ESP.getFlashChipSpeed()/1000000)+"\"";
  json += ", \"flashchmode\":\""+String(ESP.getFlashChipMode())+"\"";
  json += ", \"flashchsizebyid\":\""+String(ESP.getFlashChipSizeByChipId())+"\"";
  json += ", \"bootmode\":\""+String(ESP.getBootMode())+"\"";
  json += ", \"sketchsize\":\""+String(ESP.getSketchSize())+"\"";
  json += ", \"freesketchspace\":\""+String(ESP.getFreeSketchSpace())+"\"";
  json += ", \"freeheap\":\""+String(ESP.getFreeHeap())+"\"";
  json += ", \"godzina\":\""+String(GODZINA)+"\"";
  json += ", \"minuta\":\""+String(MINUTA)+"\"";
  json += ", \"sekunda\":\""+String(SEKUNDA)+"\"";
  json += ", \"strefa\":\""+String(STREFA-10)+"\"";
  json += ", \"cycletime\":\""+String(CYCLE_TIME)+"\"";
  json += ", \"pir\":\""+String(CZUJNIK_PIR)+"\"";
  json += ", \"lux\":\""+String(CZUJNIK_LUX)+"\"";
  json += ", \"cztem\":\""+String(CZUJNIK_TEM)+"\"";
  json += ", \"czhum\":\""+String(CZUJNIK_WIL)+"\"";
  json += ", \"fdbk_tgtLUX\":\""+String(s_tgtLUX)+"\"";
  json += ", \"fdbk_tgtRED\":\""+String(s_RED)+"\"";
  json += ", \"fdbk_tgtGREEN\":\""+String(s_GREEN)+"\"";
  json += ", \"fdbk_tgtBLUE\":\""+String(s_BLUE)+"\"";
  json += ", \"IP\":\""+WiFi.localIP().toString()+"\"";
  json += ", \"s_W_START_HOUR_VAL\":\""+String(W_START_HOUR_VAL)+"\"";
  json += ", \"s_W_START_MIN_VAL\":\""+String(W_START_MIN_VAL)+"\"";
  json += ", \"s_W_STOP_HOUR_VAL\":\""+String(W_STOP_HOUR_VAL)+"\"";
  json += ", \"s_W_STOP_MIN_VAL\":\""+String(W_STOP_MIN_VAL)+"\"";
  json += ", \"s_RGB_START_HOUR_VAL\":\""+String(RGB_START_HOUR_VAL)+"\"";
  json += ", \"s_RGB_START_MIN_VAL\":\""+String(RGB_START_MIN_VAL)+"\"";
  json += ", \"s_RGB_STOP_HOUR_VAL\":\""+String(RGB_STOP_HOUR_VAL)+"\"";
  json += ", \"s_RGB_STOP_MIN_VAL\":\""+String(RGB_STOP_MIN_VAL)+"\"";
  json += ", \"fdbk_TIMER_LIGHT_OFF_SETPOINT\":\""+String(TIMER_LIGHT_OFF_SETPOINT/10)+"\"";
  json += json_c;
  json += "}";
  server.send(200, "text/json", json);
  json = String();
}

void setup(void){
  s_SYNC=1;
  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  //START RS232 COMMUNICATION LINE
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.println("RS232 STARTED");
  DBG_OUTPUT_PORT.setDebugOutput(true);
  //RS232 STARTED
  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  //CPU DATA DISPLAY
  Serial.printf("CHIP ID                : 0x%X\n", ESP.getChipId() );   //  returns the ESP8266 chip ID as a 32-bit integer.
  Serial.printf("CPU FREQUENCY          : %d\n\n",   ESP.getCpuFreqMHz() );

  Serial.printf("FLASH CHIP ID          : 0x%X\n", ESP.getFlashChipId() );
  Serial.printf("FLASH CHIP REAL SIZE   : %d\n",   ESP.getFlashChipRealSize() );
  Serial.printf("FLASH CHIP SIZE        : %d\n",   ESP.getFlashChipSize() );  //returns the flash chip size, in bytes, as seen by the SDK (may be less than actual size).
  Serial.printf("FLASH CHIP SPEED       : %d\n",   ESP.getFlashChipSpeed() ); // returns the flash chip frequency, in Hz.
  Serial.printf("FLASH CHIP MODE        : %d\n",   ESP.getFlashChipMode() );
  Serial.printf("FLASH CHIP SIZE BY ID  : 0x%X\n\n", ESP.getFlashChipSizeByChipId() );

  Serial.printf("BOOT MODE              : %d\n",   ESP.getBootMode() );
  Serial.printf("BOOT VERSION           : %d\n\n",   ESP.getBootVersion() );

  Serial.printf("SDK VERSION            : %d\n",   ESP.getSdkVersion() );
  Serial.printf("SKETCH SIZE            : %d\n",   ESP.getSketchSize() );
  Serial.printf("FREE SKETCH SPACE      : %d\n\n",   ESP.getFreeSketchSpace() );

  Serial.printf("FREE HEAP SIZE         : %d\n\n",   ESP.getFreeHeap() );   //  returns the free heap size.
  //CPU DATA DISPLAY
  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  //Neopixel initialization
  strip.setBrightness(255);
  strip.begin();
  colorClear();
  Serial.printf("Neopixel initiatied");
  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  //PWM CONFIGURATION
  Wire.pins(4, 5);   // ESP8266 can use any two pins
  //delay(100);
  pwm.begin();
  //delay(100);
  Serial.println("PWM ON");
  //delay(100);
  pwm.setPWMFreq(1600);  // This is the maximum PWM frequency
  //delay(100);
  pwm.setPWM(0, 0 , 1024);
  delay(500);
  pwm.setPWM(0, 0 , 0);
  delay(500);
  pwm.setPWM(0, 0 , 2048);
  delay(500);
  pwm.setPWM(0, 0 , 0);
  delay(500);
  pwm.setPWM(0, 0 , 3072);
  delay(500);
  pwm.setPWM(0, 0 , 0);
  delay(500);
  pwm.setPWM(0, 0 , 4096);
  delay(500);
  pwm.setPWM(0, 0 , 0);
  delay(500);

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  //INTERNAL EEPROM CONFIGURATION
  EEPROM.begin(512);
  s_RED = (uint8_t)EEPROM.read(0); //RED
  s_GREEN = (uint8_t)EEPROM.read(1); //GREEN
  s_BLUE = (uint8_t)EEPROM.read(2); //BLUE
  s_tgtLUX = (((uint8_t)EEPROM.read(11))<<8) | ((uint8_t)(EEPROM.read(10))); //LUX LIMIT


  W_START_HOUR_VAL = (uint8_t)EEPROM.read(50); //s_W_START_HOUR_VAL
  W_START_MIN_VAL = (uint8_t)EEPROM.read(51); //s_W_START_MIN_VAL
  W_STOP_HOUR_VAL = (uint8_t)EEPROM.read(52); //s_W_STOP_HOUR_VAL
  W_STOP_MIN_VAL = (uint8_t)EEPROM.read(53); //s_W_STOP_MIN_VAL
  RGB_START_HOUR_VAL = (uint8_t)EEPROM.read(54); //s_RGB_START_HOUR_VAL
  RGB_START_MIN_VAL = (uint8_t)EEPROM.read(55); //s_RGB_START_MIN_VAL
  RGB_STOP_HOUR_VAL = (uint8_t)EEPROM.read(56); //s_RGB_STOP_HOUR_VAL
  RGB_STOP_MIN_VAL = (uint8_t)EEPROM.read(57); //s_RGB_STOP_MIN_VAL

  STREFA = (int)EEPROM.read(20); //STREFA CZASOWA
  //EEPROM.write(addr, val);
  Serial.println("EEPROM data imported");
  Serial.printf("RED: %d GREEN: %d BLUE: %d LUX_LIM_HIGH: %d LUX_LIM_LOW: %d\n",(uint8_t)EEPROM.read(0),(uint8_t)EEPROM.read(1),(uint8_t)EEPROM.read(2),((uint8_t)EEPROM.read(11)),(uint8_t)(EEPROM.read(10)));
  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  //LIGHT SENSOR BH1750 CONFIGURATION
  lightMeter.begin(BH1750_CONTINUOUS_HIGH_RES_MODE);
  Serial.println(F("BH1750 Test"));
  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  //TEMPERATURE AND HUMIDITY SENSOR  CONFIGURATION
  Serial.println("SHT31 test");
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  //GENERAL IO PINS Configuration
  pinMode(14, INPUT);

  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    DBG_OUTPUT_PORT.printf("\n");
  }
  //SPIFFS.format();

  //WIFI INIT
  DBG_OUTPUT_PORT.printf("Connecting to %s\n", ssid);
  WiFi.mode(WIFI_STA);
  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.begin(ssid, password);
  }

  int STARTUP_PIXEL_BAR=0;
  //Waiting for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DBG_OUTPUT_PORT.print(".");
    strip.setPixelColor(STARTUP_PIXEL_BAR, strip.Color(255,0,0));
    strip.show();
    STARTUP_PIXEL_BAR++;
  }
  //If not connected, reboot
  if(WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    pwm.setPWM(0, 0 , 4096);
    delay(500);
    pwm.setPWM(0, 0 , 0);
    delay(500);
    pwm.setPWM(0, 0 , 4096);
    delay(500);
    pwm.setPWM(0, 0 , 0);
    delay(3500);
    ESP.restart();
  }
  //if connection successful
  DBG_OUTPUT_PORT.println("");
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());

  //Start OTA functionality and handlers
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  DBG_OUTPUT_PORT.println("OTA Enabled");

  MDNS.begin(host);
  DBG_OUTPUT_PORT.print("Open http://");
  DBG_OUTPUT_PORT.print(host);
  DBG_OUTPUT_PORT.println(".local/edit to see the file browser");


  //SERVER INIT
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []()
  {
    server.send(200, "text/plain", "");
  },
  handleFileUpload
  );

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/esp_data", HTTP_GET, handleESPdata);

  server.on("/setESPval", handleESPval);
//setESPval?cnt='+cnt+'&val='+sliderVal
//http://esp8266.local/setESPval?cnt=12&val=1

  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");



  //PWM_CONTROL.attach_ms(10, PWM_F);


  //Wait for OTA

  Serial.println("OTA TIME!");
  for(int OTA_TIME=0; OTA_TIME < 5000; OTA_TIME++)
  {
    ArduinoOTA.handle();
    delay(1);
    if(OTA_TIME%500 == 0)
    {
      Serial.print(".");
      strip.setPixelColor(STARTUP_PIXEL_BAR, strip.Color(0,255,0));
      strip.show();
      STARTUP_PIXEL_BAR++;
      if(STARTUP_PIXEL_BAR>20)STARTUP_PIXEL_BAR=0;
    }
  }
  Serial.println("");

  pwm.setPWM(0, 0 , 0);
  LIMIT_LUX=10;
  TIMER_LIGHT_OFF_SETPOINT = 100;
  //UDP section
  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

  Serial.println("NTP SYNCHRONIZACJA");
  SYNCHRONIZACJA_NTP();

  PWM_CONTROL_ON.attach_ms(5, PWM_ON);
  PWM_CONTROL_OFF.attach_ms(5, PWM_OFF);
  RGB_RED_CONTROL_ON.attach_ms(5, RGB_RED_ON);
  RGB_GREEN_CONTROL_ON.attach_ms(5, RGB_GREEN_ON);
  RGB_BLUE_CONTROL_ON.attach_ms(5, RGB_BLUE_ON);
  RGB_CONTROL_OFF.attach_ms(5, RGB_OFF);
  TIMER_LIGHT_OFF.attach_ms(100, TIMER_LIGHT_OFF_CNT);

  SECOND.attach(1, SECOND_CNT);


  strip.clear();
  strip.show();
  STARTUP_PIXEL_BAR=0;
}

int LEDON_prev;

void loop(void){
  //############################################################################
  // KALKULOWANIE CZASU CYKLU
  CYCLE_TIME = micros() -  CYCLE_TIME_PREV;
  //############################################################################

  //############################################################################
  // DEKLARACJA ZMIENNYCH LOKALNYCH GLOWNEJ PETLI ORAZ SCZYTANIE WARTOSCI
  CZUJNIK_TEM = sht31.readTemperature();  // CZUJNIK TEMPERATURY ODCZYT
  CZUJNIK_WIL = sht31.readHumidity();     // CZUJNIK WILGOTNOSCI ODCZYT
  CZUJNIK_PIR = digitalRead(14);                // CZUJNIK PIR ODCZYT
  CZUJNIK_LUX = lightMeter.readLightLevel();    // CZUJNIK SWIATLA ODCZYT
  //############################################################################

  LIMIT_LUX = s_tgtLUX;

  //############################################################################
  //OBSLUGA SYSTEMU PLIKOW ORAZ OTA - OBOWIAZKOWE!!!
  ArduinoOTA.handle();
  server.handleClient();

  //############################################################################
  if(s_LEDON){
    if (s_CWIPE_ON){
      rainbowCycle(10);
    }
    else
    {
        for(uint16_t i=0; i<strip.numPixels(); i++) {
          strip.setPixelColor(i,  strip.Color(s_RED,s_GREEN,s_BLUE));
        }
    }
    LEDON_prev=s_LEDON;
    strip.show();
    y_RED=0;
    y_GREEN=0;
    y_BLUE=0;
  }
  if (LEDON_prev!=s_LEDON){
    s_CWIPE_ON=0;
    strip.clear();
    strip.show();
    LEDON_prev=s_LEDON;
  }
  if(s_SYNC){
    SYNCHRONIZACJA_NTP();
    strip.clear();
    strip.show();
  }
  if(SAVE_TIME_DATA){
    GODZINA=s_GODZINA;//+s_STREFA;
    //if(GODZINA>=24)GODZINA=GODZINA-24;
    //if(GODZINA<0)GODZINA=24+GODZINA;
    MINUTA=s_MINUTA;
    SEKUNDA=s_SEKUNDA;
    STREFA=s_STREFA+10;
    SAVE_TIME_DATA=0;
  }
  if(TIME_EEPROM_WRITE_DATA){
    EEPROM.write(20, (uint8_t)(STREFA & 0xff));
    EEPROM.commit();
    TIME_EEPROM_WRITE_DATA=0;
  }
  if(SAVE_LIGHT_DATA){
    EEPROM.write(0, (s_RED & 0xff));
    EEPROM.write(1, (s_GREEN & 0xff));
    EEPROM.write(2, (s_BLUE & 0xff));
    EEPROM.write(10, (uint8_t)(s_tgtLUX & 0x00ff));
    EEPROM.write(11, ((uint8_t)((s_tgtLUX & 0xff00) >> 8)));
    Serial.printf("RED: %d GREEN: %d BLUE: %d LUX_LIM_LOW: %d LUX_LIM_HIGH: %d\n",s_RED,s_GREEN,s_BLUE,(uint8_t)(s_tgtLUX & 0xff),((uint8_t)((s_tgtLUX & 0xff00) >> 8)));
    EEPROM.commit();
    SAVE_LIGHT_DATA=0;
  }
  if(RGBW_EEPROM_WRITE_DATA){
    EEPROM.write(50, (uint8_t)(W_START_HOUR_VAL & 0xff));
    EEPROM.write(51, (uint8_t)(W_START_MIN_VAL & 0xff));
    EEPROM.write(52, (uint8_t)(W_STOP_HOUR_VAL & 0xff));
    EEPROM.write(53, (uint8_t)(W_STOP_MIN_VAL & 0xff));
    EEPROM.write(54, (uint8_t)(RGB_START_HOUR_VAL & 0xff));
    EEPROM.write(55, (uint8_t)(RGB_START_MIN_VAL & 0xff));
    EEPROM.write(56, (uint8_t)(RGB_STOP_HOUR_VAL & 0xff));
    EEPROM.write(57, (uint8_t)(RGB_STOP_MIN_VAL & 0xff));
    EEPROM.commit();
    RGBW_EEPROM_WRITE_DATA=0;
  }
  // int LED_W_RELEASE=0,LED_RGB_RELEASE=0;
  // if((~LED_W_RELEASE) and ())

  CYCLE_TIME_PREV = micros();
}
