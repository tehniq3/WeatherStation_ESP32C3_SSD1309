/*
   Project: Internet Weather Station
   Fix: Serial Monitor Date Fix
   Project ported on SSD1309 SPI 2.42"display from ST7920 display
*/

#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <time.h>
/*
// Pin definitions for ESP32-C3 Mini used for ST7920
#define SCLK_PIN 2   
#define MOSI_PIN 3   
#define CS_PIN   4   
*/
#define API_PIN  5

// Define ESP32-C3 Hardware SPI2 pins
#define SPI_CLK  4
#define SPI_MOSI 6
#define SPI_MISO -1 // Not used for display output
#define SPI_CS   7

// Define Display Control Pins
#define OLED_DC   3
#define OLED_RST  10


// some macros
#define ROHTAK -1
#define CREDITS -2
#define THUNDER 0
#define DRIZZLE 1
#define RAIN 2
#define SNOW 3
#define SUN  4
#define FOG 5
#define CLEAR 6
#define CLOUD 7
#define HUMIDITY 8
#define WIND 9
#define PRESSURE 10
#define TEMPMIN 11
#define TEMPMAX 12
#define FEELSLIKE 13
#define CLOUDS 14
#define WINDDEGREE 15
#define WINDGUST 16
#define VISIBILITY 17
#define SUNRISE 18
#define SUNSET 19
#define QUOTE 20
#define UV 21        
#define AQI 22       
#define DATE_DAY_PAGE 23  

#define DELAY1 4000
#define DELAY2 4000

// some global variables
String locationn = "Craiova";
String countryy = "RO";
int temperaturee;
int humidityy;
String weatherr;
String descriptionn;
float pressuree;
int temp_minn;
int temp_maxx;
int visibilityy;
int wind_degreee;
String idstringg;
int feels_likee;
float wind_speedd;
float wind_gustt;
int cloudss;
int sr_hrs;
int sr_mnt;
int st_hrs;
int st_mnt;

// Variabile pentru UV și AQI
float uvIndexx = 0.0;
String uvDescriptionn = "N/A";
int aqiValuee = 0;
String aqiDescriptionn = "N/A";

// Variabile pentru ceas, data și ziua
int current_hrs = 0;
int current_mnt = 0;
int current_sec = 0;
int current_day = 0;   
int current_month = 0; 
int current_year = 0;  
int current_wday = 0;  
int romaniaOffset = 3;

// Coordonate Craiova
float latt = 44.3167;
float lonn = 23.8000;

// wifi credentials
const char* ssid = "bbk2";
const char* password = "internet2";

// u8g2 st7920 construction
//U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R2, SCLK_PIN, MOSI_PIN, CS_PIN, U8X8_PIN_NONE);

// Initialize U8g2 using full-buffer Hardware SPI (Fastest performance)
// Note: If SSD1309 constructor isn't explicitly available, the standard 
// SSD1306 128x64 noname configuration acts as a fully compatible fallback.
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs= */ SPI_CS, /* dc= */ OLED_DC, /* reset= */ OLED_RST);

// ===== FUNCȚIE PENTRU AFIȘARE DATE ÎN SERIAL MONITOR =====
void printDataToSerial() {
  const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  
  Serial.println("\n========================================");
  Serial.print("Location: "); Serial.print(locationn); Serial.print(", "); Serial.println(countryy);
  Serial.print("Date: "); if(current_day<10) Serial.print("0"); Serial.print(current_day); Serial.print("."); if(current_month<10) Serial.print("0"); Serial.print(current_month); Serial.print("."); Serial.println(current_year);
  Serial.print("Day:  "); Serial.println(days[current_wday]);
  Serial.print("Weather: "); Serial.println(weatherr);
  Serial.println("----------------------------------------");
  Serial.print("Temperature:   "); Serial.print(temperaturee); Serial.println(" °C");
  Serial.print("Feels like:    "); Serial.print(feels_likee); Serial.println(" °C");
  Serial.print("Temp Min:      "); Serial.print(temp_minn); Serial.println(" °C");
  Serial.print("Temp Max:      "); Serial.print(temp_maxx); Serial.println(" °C");
  Serial.println("----------------------------------------");
  Serial.print("Humidity:      "); Serial.print(humidityy); Serial.println(" %");
  Serial.print("Pressure(MSL): "); Serial.print(pressuree, 1); Serial.println(" mmHg");
  Serial.print("Cloud Cover:   "); Serial.print(cloudss); Serial.println(" %");
  Serial.print("Visibility:    "); Serial.print(visibilityy); Serial.println(" km");
  Serial.println("----------------------------------------");
  Serial.print("Wind Speed:    "); Serial.print(wind_speedd, 1); Serial.println(" km/h");
  Serial.print("Wind Degree:   "); Serial.print(wind_degreee); Serial.println("°");
  Serial.print("Wind Gusts:    "); Serial.print(wind_gustt, 1); Serial.println(" km/h");
  Serial.println("----------------------------------------");
  Serial.print("Sunrise:       "); if(sr_hrs<10) Serial.print("0"); Serial.print(sr_hrs); Serial.print(":"); if(sr_mnt<10) Serial.print("0"); Serial.println(sr_mnt);
  Serial.print("Sunset:        "); if(st_hrs<10) Serial.print("0"); Serial.print(st_hrs); Serial.print(":"); if(st_mnt<10) Serial.print("0"); Serial.println(st_mnt);
  Serial.println("----------------------------------------");
  Serial.print("UV Index:      "); Serial.print(uvIndexx, 1); Serial.print(" ("); Serial.print(uvDescriptionn); Serial.println(")");
  Serial.print("AQI European:  "); Serial.print(aqiValuee); Serial.print(" ("); Serial.print(aqiDescriptionn); Serial.println(")");
  Serial.println("========================================\n");
}

// ===== FUNCȚIE PENTRU DETERMINARE OFFSET ROMÂNIA =====
int getRomaniaOffset() {
  time_t now; time(&now);
  struct tm nowTm; gmtime_r(&now, &nowTm);
  int year = nowTm.tm_year + 1900;
  
  struct tm lastSunMarch = {0}; lastSunMarch.tm_year = year - 1900; lastSunMarch.tm_mon = 2; lastSunMarch.tm_mday = 31; mktime(&lastSunMarch);
  while (lastSunMarch.tm_wday != 0) { lastSunMarch.tm_mday--; mktime(&lastSunMarch); }
  lastSunMarch.tm_hour = 3; time_t dstStart = mktime(&lastSunMarch);
  
  struct tm lastSunOct = {0}; lastSunOct.tm_year = year - 1900; lastSunOct.tm_mon = 9; lastSunOct.tm_mday = 31; mktime(&lastSunOct);
  while (lastSunOct.tm_wday != 0) { lastSunOct.tm_mday--; mktime(&lastSunOct); }
  lastSunOct.tm_hour = 4; time_t dstEnd = mktime(&lastSunOct);
  
  return (now >= dstStart && now < dstEnd) ? 3 : 2;
}

void updateClock() {
  time_t now; time(&now);
  time_t localTime = now + (romaniaOffset * 3600);
  struct tm localTm; gmtime_r(&localTime, &localTm);
  
  current_hrs = localTm.tm_hour; 
  current_mnt = localTm.tm_min; 
  current_sec = localTm.tm_sec;
  current_day = localTm.tm_mday;
  current_month = localTm.tm_mon + 1; 
  current_year = localTm.tm_year + 1900;
  current_wday = localTm.tm_wday;     
}

void drawClock() {
  updateClock();
  u8g2.setFont(u8g2_font_10x20_mf);
  if (current_hrs < 10) u8g2.print("0"); u8g2.print(current_hrs); u8g2.print(":");
  if (current_mnt < 10) u8g2.print("0"); u8g2.print(current_mnt); u8g2.print(":");
  if (current_sec < 10) u8g2.print("0"); u8g2.print(current_sec);
}

void drawRohtak() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_8x13B_mr); u8g2.setCursor(6, 25); u8g2.print(F("Weather Station"));
    u8g2.setFont(u8g2_font_tenstamps_mf); u8g2.setCursor(20, 40); u8g2.print(F("Craiova"));
    u8g2.setFont(u8g2_font_nokiafc22_tf); u8g2.setCursor(7, 55); u8g2.print(F("Lat=")); u8g2.print(latt, 2); u8g2.print(" N");
    u8g2.setCursor(68, 55); u8g2.print(F("Lon=")); u8g2.print(lonn, 2); u8g2.print(F(" E"));
  } while (u8g2.nextPage());
}

String translateWMO(int code) {
  if (code == 0 || code == 1) return "Clear";
  if (code == 2 || code == 3) return "Clouds";
  if (code == 45 || code == 48) return "Fog";
  if (code >= 51 && code <= 57) return "Drizzle";
  if (code >= 61 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Rain";
  if (code >= 85 && code <= 86) return "Snow";
  if (code >= 95) return "Thunderstorm";
  return "Clouds";
}

void parseTime(String isoTime, int &hrs, int &mnt) {
  hrs = isoTime.substring(11, 13).toInt();
  mnt = isoTime.substring(14, 16).toInt();
}

// ===== EXTRAGERE DATE VREME =====
void getOpenMeteoData() {
  HTTPClient http;
  WiFiClient client; 
  
  String url = "http://api.open-meteo.com/v1/forecast?latitude=" + String(latt, 4) + "&longitude=" + String(lonn, 4) + "&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,pressure_msl,wind_speed_10m,wind_direction_10m,wind_gusts_10m,uv_index,visibility,cloud_cover&daily=sunrise,sunset,temperature_2m_max,temperature_2m_min&timezone=auto&wind_speed_unit=kmh";
    
  digitalWrite(API_PIN, HIGH);
  http.begin(client, url);
  http.setConnectTimeout(10000);
  http.useHTTP10(true); 
  int httpCode = http.GET();
  digitalWrite(API_PIN, LOW);
  
  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, payload);

    temperaturee = (int)doc["current"]["temperature_2m"].as<float>();
    humidityy = doc["current"]["relative_humidity_2m"].as<int>();
    feels_likee = (int)doc["current"]["apparent_temperature"].as<float>();
    pressuree = doc["current"]["pressure_msl"].as<float>() * 0.750062; 
    wind_speedd = doc["current"]["wind_speed_10m"].as<float>(); 
    wind_degreee = doc["current"]["wind_direction_10m"].as<int>();
    wind_gustt = doc["current"]["wind_gusts_10m"].as<float>(); 
    uvIndexx = doc["current"]["uv_index"].as<float>();
    visibilityy = doc["current"]["visibility"].as<int>() / 1000;
    cloudss = doc["current"]["cloud_cover"].as<int>(); 
    
    int wmoCode = doc["current"]["weather_code"].as<int>();
    weatherr = translateWMO(wmoCode);

    parseTime(doc["daily"]["sunrise"][0].as<String>(), sr_hrs, sr_mnt);
    parseTime(doc["daily"]["sunset"][0].as<String>(), st_hrs, st_mnt);
    temp_minn = (int)doc["daily"]["temperature_2m_min"][0].as<float>();
    temp_maxx = (int)doc["daily"]["temperature_2m_max"][0].as<float>();
    
    if (uvIndexx <= 2) uvDescriptionn = "Low";
    else if (uvIndexx <= 5) uvDescriptionn = "Moderate";
    else if (uvIndexx <= 7) uvDescriptionn = "High";
    else if (uvIndexx <= 10) uvDescriptionn = "Very High";
    else uvDescriptionn = "Extreme";

    Serial.println("-> Weather+UV data fetched!");
  } else {
    Serial.print("-> HTTP Weather Error: "); Serial.println(httpCode);
  }
  http.end(); 
  client.stop(); 
}

// ===== EXTRAGERE CALITATE AER =====
void getOpenMeteoAQI() {
  HTTPClient http;
  WiFiClientSecure secureClient; 
  secureClient.setInsecure();  
  
  String url = "https://air-quality-api.open-meteo.com/v1/air-quality?latitude=" + String(latt, 4) + "&longitude=" + String(lonn, 4) + "&current=european_aqi&timezone=auto";
  
  digitalWrite(API_PIN, HIGH);
  http.begin(secureClient, url); 
  http.setConnectTimeout(10000);
  http.useHTTP10(true); 
  int httpCode = http.GET();
  digitalWrite(API_PIN, LOW);
  
  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    
    aqiValuee = doc["current"]["european_aqi"].as<int>();
    
    if (aqiValuee <= 20) aqiDescriptionn = "Good";
    else if (aqiValuee <= 40) aqiDescriptionn = "Fair";
    else if (aqiValuee <= 60) aqiDescriptionn = "Moderate";
    else if (aqiValuee <= 80) aqiDescriptionn = "Poor";
    else if (aqiValuee <= 100) aqiDescriptionn = "Very Poor";
    else aqiDescriptionn = "Extremely Poor";

    Serial.println("-> AQI data fetched!");
  } else {
    Serial.print("-> HTTP AQI Error: "); Serial.println(httpCode);
  }
  http.end();
  secureClient.stop(); 
}

// ================= PARTEA DE DESENARE (GRAPHICS) =================
void drawWeatherSymbol_t(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) {
  if (symbol <= 7) {
    switch (symbol) {
      case THUNDER: u8g2.setFont(u8g2_font_open_iconic_embedded_6x_t); u8g2.drawGlyph(x, y, 67); break;
      case DRIZZLE: u8g2.setFont(u8g2_font_open_iconic_weather_6x_t); u8g2.drawGlyph(x, y, 65); break;
      case RAIN: u8g2.setFont(u8g2_font_open_iconic_weather_6x_t); u8g2.drawGlyph(x, y, 67); break;
      case SNOW: u8g2.setFont(u8g2_font_open_iconic_text_6x_t); u8g2.drawGlyph(x, y, 69); break;
      case SUN: u8g2.setFont(u8g2_font_open_iconic_weather_6x_t); u8g2.drawGlyph(x, y, 69); break;
      case FOG: u8g2.setFont(u8g2_font_open_iconic_mime_6x_t); u8g2.drawGlyph(x, y, 64); break;
      case CLEAR: u8g2.setFont(u8g2_font_open_iconic_weather_6x_t); u8g2.drawGlyph(x, y, 69); break;
      case CLOUD: u8g2.setFont(u8g2_font_open_iconic_weather_6x_t); u8g2.drawGlyph(x, y, 64); break;
    }
  }
}
void drawWeatherSymbol_h(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_thing_6x_t); u8g2.drawGlyph(x, y, 72); }
void drawWeatherSymbol_w(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_all_6x_t); u8g2.drawGlyph(x, y, 254); }
void drawWeatherSymbol_p(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_app_6x_t); u8g2.drawGlyph(x, y, 72); }
void drawWeatherSymbol_tn(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_arrow_6x_t); u8g2.drawGlyph(x, y, 72); }
void drawWeatherSymbol_tm(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_arrow_6x_t); u8g2.drawGlyph(x, y, 75); }
void drawWeatherSymbol_fl(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_text_6x_t); u8g2.drawGlyph(x, y, 69); }
void drawWeatherSymbol_wd(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_arrow_6x_t); u8g2.drawGlyph(x, y, 87); }
void drawWeatherSymbol_wg(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_arrow_4x_t); u8g2.drawGlyph(x, y, 90); }
void drawWeatherSymbol_v(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_human_6x_t); u8g2.drawGlyph(x, y, 64); }
void drawWeatherSymbol_c(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_weather_6x_t); u8g2.drawGlyph(x, y, 64); }
void drawWeatherSymbol_sr(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_weather_6x_t); u8g2.drawGlyph(0, 48, 69); }
void drawWeatherSymbol_ss(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_weather_6x_t); u8g2.drawGlyph(0, 48, 66); }
void drawWeatherSymbol_wq(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_mime_6x_t); u8g2.drawGlyph(0, 48, 66); }
void drawWeatherSymbol_uv(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_embedded_6x_t); u8g2.drawGlyph(x, y, 80); }
void drawWeatherSymbol_aqi(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { u8g2.setFont(u8g2_font_open_iconic_embedded_6x_t); u8g2.drawGlyph(x, y, 72); }

void drawWeatherSymbol_date(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol) { 
  u8g2.setFont(u8g2_font_open_iconic_mime_6x_t); 
  u8g2.drawGlyph(x, y, 66); // niq_ro choose  other icon
}

#define ICON_Y   48
#define DATA_Y   51
#define SCROLL_Y 62

void drawWeather(uint8_t symbol, int degree) {
  if (symbol <= 7) { drawWeatherSymbol_t(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso32_tf); u8g2.setCursor(48 + 3, DATA_Y); u8g2.print(temperaturee); u8g2.print("°C"); }
  if (symbol == 8) { drawWeatherSymbol_h(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso32_tf); u8g2.setCursor(48, DATA_Y); u8g2.print(humidityy); u8g2.print("%"); }
  if (symbol == 9) { drawWeatherSymbol_w(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso32_tf); u8g2.setCursor(40, DATA_Y); u8g2.print(wind_speedd, 0); u8g2.setFont(u8g2_font_t0_17b_tf); u8g2.print("km/h"); }
  if (symbol == 10) { drawWeatherSymbol_p(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso24_tf); u8g2.setCursor(46, DATA_Y-2); u8g2.print(pressuree, 0); u8g2.setFont(u8g2_font_7x14B_tf); u8g2.print("mmHg"); }
  if (symbol == 11) { drawWeatherSymbol_tn(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso32_tf); u8g2.setCursor(48 + 3, DATA_Y); u8g2.print(temp_minn); u8g2.print("°C"); }
  if (symbol == 12) { drawWeatherSymbol_tm(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso32_tf); u8g2.setCursor(48 + 3, DATA_Y); u8g2.print(temp_maxx); u8g2.print("°C"); }
  if (symbol == 13) { drawWeatherSymbol_fl(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso32_tf); u8g2.setCursor(48 + 3, DATA_Y); u8g2.print(feels_likee); u8g2.print("°C"); }
  if (symbol == 14) { drawWeatherSymbol_c(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso32_tf); u8g2.setCursor(48, DATA_Y); u8g2.print(cloudss); u8g2.print("%"); }
  if (symbol == 15) { drawWeatherSymbol_wd(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso32_tf); u8g2.setCursor(48 + 3, DATA_Y); u8g2.print(wind_degreee); u8g2.print("°"); }
  if (symbol == 16) { drawWeatherSymbol_wg(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso28_tf); u8g2.setCursor(35, DATA_Y); u8g2.print(wind_gustt, 0); u8g2.setFont(u8g2_font_t0_17b_tf); u8g2.print("km/h"); }
  if (symbol == 17) { drawWeatherSymbol_v(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso32_tf); u8g2.setCursor(48 + 3, DATA_Y); u8g2.print(visibilityy); u8g2.setFont(u8g2_font_lubB14_tr); u8g2.print("km"); }
  if (symbol == 18) { drawWeatherSymbol_sr(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso22_tf); u8g2.setCursor(48 + 3, DATA_Y); u8g2.print(sr_hrs); u8g2.print(":"); if (sr_mnt < 10) u8g2.print("0"); u8g2.print(sr_mnt); }
  if (symbol == 19) { drawWeatherSymbol_ss(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso22_tf); u8g2.setCursor(48 + 3, DATA_Y); u8g2.print(st_hrs); u8g2.print(":"); if (st_mnt < 10) u8g2.print("0"); u8g2.print(st_mnt); }
  if (symbol == 20) { drawWeatherSymbol_wq(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_adventurer_tf); u8g2.setCursor(48 + 3, 25); u8g2.print("I live for"); u8g2.setCursor(43, DATA_Y); u8g2.print("this Weather"); }
  if (symbol == 21) { drawWeatherSymbol_uv(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso24_tf); u8g2.setCursor(46, DATA_Y); u8g2.print(uvIndexx, 1); }
  if (symbol == 22) { drawWeatherSymbol_aqi(0, ICON_Y, symbol); u8g2.setFont(u8g2_font_logisoso32_tf); u8g2.setCursor(48, DATA_Y); u8g2.print(aqiValuee); }
  
  if (symbol == 23) { 
    drawWeatherSymbol_date(0, ICON_Y, symbol); 
    //u8g2.setFont(u8g2_font_nokiafc22_tf); 
    u8g2.setFont(u8g2_font_8x13B_mr); // niq_ro's font choosed
    
    u8g2.setCursor(48, 32); 
    if (current_day < 10) u8g2.print("0"); u8g2.print(current_day); u8g2.print(".");
    if (current_month < 10) u8g2.print("0"); u8g2.print(current_month); u8g2.print(".");
    u8g2.print(current_year); 
    
    const char* days_en[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    u8g2.setCursor(48, 45); 
    u8g2.print(days_en[current_wday]); 
  }
}

void drawScrollString(int16_t offset, const char *s) {
  static char buf[36]; size_t len; size_t char_offset = 0; u8g2_uint_t dx = 0; size_t visible = 0;
  len = strlen(s);
  if (offset < 0) {
    char_offset = (-offset) / 8; dx = offset + char_offset * 8;
    if (char_offset >= u8g2.getDisplayWidth() / 8) return;
    visible = u8g2.getDisplayWidth() / 8 - char_offset + 1;
    strncpy(buf, s, visible); buf[visible] = '\0';
    u8g2.setFont(u8g2_font_8x13_mf); u8g2.drawStr(char_offset * 8 - dx, SCROLL_Y, buf);
  } else {
    char_offset = offset / 8;
    if (char_offset >= len) return;
    dx = offset - char_offset * 8; visible = len - char_offset;
    if (visible > u8g2.getDisplayWidth() / 8 + 1) visible = u8g2.getDisplayWidth() / 8 + 1;
    strncpy(buf, s + char_offset, visible); buf[visible] = '\0';
    u8g2.setFont(u8g2_font_8x13_mf); u8g2.drawStr(-dx, SCROLL_Y, buf);
  }
}

void draw(const char *s, uint8_t symbol, float degree) {
  int16_t offset = -(int16_t)u8g2.getDisplayWidth();
  int16_t len = strlen(s);
  for (;;) {
    u8g2.firstPage();
    do {
      u8g2.setCursor(49, 13); drawClock();
      drawWeather(symbol, degree);
      drawScrollString(offset, s);
    } while (u8g2.nextPage());
    delay(20); offset += 2;
    if (offset > len * 8 + 1) break;
  }
}

void setup(void) {
  Serial.begin(115200);
  Serial.setDebugOutput(false); 
  
  pinMode(API_PIN, OUTPUT); digitalWrite(API_PIN, LOW);

  WiFi.begin(ssid, password);
  // Reduce puterea semnalului pentru a preveni crash-ul plăcuței mini
  WiFi.setTxPower(WIFI_POWER_8_5dBm); 
  
  Serial.println("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected!");

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Syncing NTP...");
  int timeout = 0; struct tm timeinfo;
  while (!getLocalTime(&timeinfo) && timeout < 20) { delay(500); timeout++; }
  
  romaniaOffset = getRomaniaOffset();
  Serial.println("Weather Station starting...");
  u8g2.begin(); u8g2.enableUTF8Print();
  drawRohtak(); delay(DELAY1);
}

void loop(void) {
  drawRohtak();
  
  getOpenMeteoData();
  delay(100); 
  getOpenMeteoAQI();

  updateClock(); // <--- FIX: Forțează actualizarea datei înainte de Serial Print
  printDataToSerial(); 

  if (weatherr == "Thunderstorm") draw("That sounds like thunder.", THUNDER, temperaturee);
  else if (weatherr == "Drizzle") draw("It's quite drizzle outside!", DRIZZLE, temperaturee);
  else if (weatherr == "Rain") draw("It's raining cats and dogs.", RAIN, temperaturee);
  else if (weatherr == "Snow") draw("It's snowing outside!", SNOW, temperaturee);
  else if (weatherr == "Fog") draw("It's foggy out there!", FOG, temperaturee);
  else if (weatherr == "Clear") draw("The sun's come out!", CLEAR, temperaturee);
  else if (weatherr == "Clouds") draw("It's cloudy outside!", CLOUD, temperaturee);

  draw("Today is...", DATE_DAY_PAGE, 0.0);

  draw("Minimum temperature!!", TEMPMIN, temp_minn);
  draw("Maximum temperature!!", TEMPMAX, temp_maxx);
  draw("It feels like!!", FEELSLIKE, feels_likee);
  draw("There are clouds!!!", CLOUDS, cloudss);
  draw("Hello! new sun!!", SUNRISE, 0.0);     
  draw("Bye bye sun!!", SUNSET, 0.0);         
  draw("Humidity in the air!", HUMIDITY, humidityy);
  draw("See visibility!!", VISIBILITY, visibilityy);
  draw("Wind travelling speed!", WIND, wind_speedd);
  draw("Wind travelling angle!", WINDDEGREE, wind_degreee);
  draw("Wind gust there!", WINDGUST, wind_gustt);
  draw("Atmospheric pressure!!", PRESSURE, pressuree);
  
  draw(("UV Index: " + String(uvIndexx, 1) + " - " + uvDescriptionn).c_str(), UV, uvIndexx);
  draw(("Air Quality: " + aqiDescriptionn).c_str(), AQI, aqiValuee);
}
