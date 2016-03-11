/**The MIT License (MIT)

Copyright (c) 2016 by Seokjin Seo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at http://usemodj.com
*/

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "SSD1306.h"
#include "SSD1306Ui.h"
#include "Wire.h"
#include "KMAWeatherClient.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"
#include "NTPTimeClient.h"
#include "sscanf.h"

/***************************
 * Begin Settings
 **************************/
// WIFI
const char* WIFI_SSID = "*****"; 
const char* WIFI_PWD = "******";

// KMA Weather Settings
// *  www.data.go.kr
// *    OPEN API: (신)동네예보정보조회서비스
const String KMA_SERVICE_KEY = "********************";
const String KMA_NX = "70"; //예보지점 X좌표(도사동:순천만)
const String KMA_NY = "69"; //예보지점 Y좌표(도사동:순천만)

// Setup
const int UPDATE_INTERVAL_SECS = 30 * 60; // Update every 30 minutes

// Display Settings
const int I2C_DISPLAY_ADDRESS = 0x3c;
const int SDA_PIN = 4; //GPIO 4
const int SDC_PIN = 5; //GPIO 5

// TimeClient settings
const float UTC_OFFSET = 9;

// Initialize the oled display for address 0x3c
SSD1306   display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
SSD1306Ui ui     ( &display );

/***************************
 * End Settings
 **************************/
bool drawFrame1(SSD1306 *display, SSD1306UiState* state, int x, int y);
bool drawFrame2(SSD1306 *display, SSD1306UiState* state, int x, int y);
bool drawFrame3(SSD1306 *display, SSD1306UiState* state, int x, int y);
bool drawFrame4(SSD1306 *display, SSD1306UiState* state, int x, int y);
bool drawFrame5(SSD1306 *display, SSD1306UiState* state, int x, int y);
bool drawFrame6(SSD1306 *display, SSD1306UiState* state, int x, int y);
void drawProgress(SSD1306 *display, int percentage, String label);
void setReadyForWeatherUpdate();
void updateData(SSD1306 *display);
void drawForecastToday(SSD1306 *display, int x, int y, int fcstIndex);
void drawForecastTomorrow(SSD1306 *display, int x, int y, int fcstIndex);

NTPTimeClient timeClient(UTC_OFFSET);

// Set to false, if you prefere imperial/inches, Fahrenheit
KMAWeatherClient kmaWeather;

// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
bool (*frames[])(SSD1306 *display, SSD1306UiState* state, int x, int y) = { drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5, drawFrame6 };
int numberOfFrames = 6;

// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;
String lastUpdate = "--";
Ticker ticker;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // initialize dispaly
  display.init();
  display.clear();
  display.display();

//  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);

  WiFi.begin(WIFI_SSID, WIFI_PWD);
  
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.clear();
    display.drawString(64, 10, "Connecting to WiFi");
    display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display.display();
    
    counter++;
  }

  ui.setTargetFPS(30);
  ui.setTimePerFrame(10000); //10sec
  ui.setActiveSymbole(activeSymbole);
  ui.setInactiveSymbole(inactiveSymbole);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);
  //ui.setIndicatorPosition(LEFT);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  //ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrameAnimation(SLIDE_UP);

  // Add frames
  ui.setFrames(frames, numberOfFrames);

  // Inital UI takes care of initalising the display too.
  ui.init();
  
//  display.flipScreenVertically();
  
  Serial.println("");

  updateData(&display);

  ticker.attach(UPDATE_INTERVAL_SECS, setReadyForWeatherUpdate);

}

void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println("looping...");
  if(readyForWeatherUpdate && ui.getUiState().frameState == FIXED) {
    updateData(&display);
  }

  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    delay(remainingTimeBudget);
  }
}

void updateData(SSD1306 *display) {
  drawProgress(display, 10, "Updating time...");
  timeClient.updateTime();

  String baseDate = timeClient.getFormattedDate(""); //"20160307";
  String baseTime = "0200";
  int numOfRows = 10;

  drawProgress(display, 50, "Updating forecasts...");
  kmaWeather.updateForecast(KMA_SERVICE_KEY, KMA_NX, KMA_NY, baseDate, baseTime, numOfRows);
  lastUpdate = timeClient.getFormattedTime();
  readyForWeatherUpdate = false;
  drawProgress(display, 100, "Done...");
  delay(1000);
}

void drawProgress(SSD1306 *display, int percentage, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawRect(10, 28, 108, 12);
  display->fillRect(12, 30, 104 * percentage / 100 , 9);
  display->display();
}

bool drawFrame1(SSD1306 *display, SSD1306UiState* state, int x, int y) {
  int textWidth;
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_24);
  String date = timeClient.getMonth() + "/" + timeClient.getDay();
  textWidth = display->getStringWidth(date);
  display->drawString(3 + x, 3 + y, date);
  display->setFont(NanumGothic_Plain_12);
  display->drawString(5 + x + textWidth, 3 + y, "Weather");
  
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  String time = timeClient.getFormattedTime();
  textWidth = display->getStringWidth(time);
  display->drawString(64 + x, 25 + y, time);
  //display->setTextAlignment(TEXT_ALIGN_LEFT);
}

bool drawFrame2(SSD1306 *display, SSD1306UiState* state, int x, int y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(x, y+2, "Today");
  display->setFont(ArialMT_Plain_16);
  display->drawString(x, y + 22, String((int)kmaWeather.getTempMaxMinToday()->tmn) 
  + "/" + String((int)kmaWeather.getTempMaxMinToday()->tmx) + + "°C");
  display->setFont(ArialMT_Plain_10);
  display->drawString(x+1, y + 40, "Humidity:"); 

  drawForecastToday(display, x + 38, y, 0);
  drawForecastToday(display, x + 68, y, 1);
  drawForecastToday(display, x + 98, y, 2);
}

bool drawFrame3(SSD1306 *display, SSD1306UiState* state, int x, int y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(x, y+2, "Today");
  display->setFont(ArialMT_Plain_16);
  display->drawString(x, y + 22, String((int)kmaWeather.getTempMaxMinToday()->tmn) 
  + "/" + String((int)kmaWeather.getTempMaxMinToday()->tmx) + + "°C");
  display->setFont(ArialMT_Plain_10);
  display->drawString(x+1, y + 40, "Humidity:");

  drawForecastToday(display, x + 38, y, 3);
  drawForecastToday(display, x + 68, y, 4);
  drawForecastToday(display, x + 98, y, 5);
}

bool drawFrame4(SSD1306 *display, SSD1306UiState* state, int x, int y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(x, y+2, "T");
  int tWidth = display->getStringWidth("T");
  display->setFont(ArialMT_Plain_10);
  display->drawString(x+tWidth, y+2, "omo");
  display->drawString(x+tWidth, y+10, "rrow");
    
  display->setFont(ArialMT_Plain_16);
  display->drawString(x, y + 22, String((int)kmaWeather.getTempMaxMinTomorrow()->tmn) 
  + "/" + String((int)kmaWeather.getTempMaxMinTomorrow()->tmx) + + "°C");
  display->setFont(ArialMT_Plain_10);
  display->drawString(x+1, y + 40, "Humidity:"); 

  drawForecastTomorrow(display, x + 38, y, 0);
  drawForecastTomorrow(display, x + 68, y, 1);
  drawForecastTomorrow(display, x + 98, y, 2);
}

bool drawFrame5(SSD1306 *display, SSD1306UiState* state, int x, int y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(x, y+2, "T");
  int tWidth = display->getStringWidth("T");
  display->setFont(ArialMT_Plain_10);
  display->drawString(x+tWidth, y+2, "omo");
  display->drawString(x+tWidth, y+10, "rrow");

  display->setFont(ArialMT_Plain_16);
  display->drawString(x, y + 22, String((int)kmaWeather.getTempMaxMinTomorrow()->tmn) 
  + "/" + String((int)kmaWeather.getTempMaxMinTomorrow()->tmx) + + "°C");
  display->setFont(ArialMT_Plain_10);
  display->drawString(x+1, y + 40, "Humidity:");

  drawForecastTomorrow(display, x + 38, y, 3);
  drawForecastTomorrow(display, x + 68, y, 4);
  drawForecastTomorrow(display, x + 98, y, 5);
}

bool drawFrame6(SSD1306 *display, SSD1306UiState* state, int x, int y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(x, y+2, "T");
  int tWidth = display->getStringWidth("T");
  display->setFont(ArialMT_Plain_10);
  display->drawString(x+tWidth, y+2, "omo");
  display->drawString(x+tWidth, y+10, "rrow");

  display->setFont(ArialMT_Plain_16);
  display->drawString(x, y + 22, String((int)kmaWeather.getTempMaxMinTomorrow()->tmn) 
  + "/" + String((int)kmaWeather.getTempMaxMinTomorrow()->tmx) + + "°C");
  display->setFont(ArialMT_Plain_10);
  display->drawString(x+1, y + 40, "Humidity:");

  drawForecastTomorrow(display, x + 38, y, 6);
  drawForecastTomorrow(display, x + 68, y, 7);
  //drawForecastTomorrow(display, x + 98, y, 8);
}

void setReadyForWeatherUpdate() {
  Serial.println("Setting readyForUpdate to true");
  readyForWeatherUpdate = true;
}

void drawForecastToday(SSD1306 *display, int x, int y, int fcstIndex) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y, kmaWeather.getForecastToday(fcstIndex)->fcstTime.substring(0,2) + "h");
  
  display->setFont(Meteocons_0_21);
  display->drawString(x + 20, y + 10, kmaWeather.getWeatherIcon(kmaWeather.getForecastToday(fcstIndex)->sky, 
                                        kmaWeather.getForecastToday(fcstIndex)->pty, kmaWeather.getForecastToday(fcstIndex)->fcstTime.substring(0,2).toInt()));
  display->setFont(ArialMT_Plain_10);
  //display->drawString(x + 20, y + 20, String(kmaWeather.getForecastToday(fcstIndex)->pop) + "%"); // 강수확률(%)
  display->drawString(x + 20, y + 30, String((int)kmaWeather.getForecastToday(fcstIndex)->t3h) + "°C"); //3시간기온
  display->drawString(x + 20, y + 40, String(kmaWeather.getForecastToday(fcstIndex)->reh) + "%"); // 습도(%)
  
}

void drawForecastTomorrow(SSD1306 *display, int x, int y, int fcstIndex) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y, kmaWeather.getForecastTomorrow(fcstIndex)->fcstTime.substring(0,2) + "h");
  
  display->setFont(Meteocons_0_21);
  display->drawString(x + 20, y + 10, kmaWeather.getWeatherIcon(kmaWeather.getForecastToday(fcstIndex)->sky, 
                                        kmaWeather.getForecastTomorrow(fcstIndex)->pty, kmaWeather.getForecastTomorrow(fcstIndex)->fcstTime.substring(0,2).toInt()));
  display->setFont(ArialMT_Plain_10);
  //display->drawString(x + 20, y + 20, String(kmaWeather.getForecastTomorrow(fcstIndex)->pop) + "%"); // 강수확률(%)
  display->drawString(x + 20, y + 30, String((int)kmaWeather.getForecastTomorrow(fcstIndex)->t3h) + "°C"); //3시간기온
  display->drawString(x + 20, y + 40, String(kmaWeather.getForecastTomorrow(fcstIndex)->reh) + "%"); // 습도(%)
  
}


