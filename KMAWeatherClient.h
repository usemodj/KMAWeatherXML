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

#pragma once

#include <math.h>
#include <TimeLib.h>
#include "tinyxml2.h"

using namespace tinyxml2;

typedef struct {
  String fcstDate; //예보일자
  String fcstTime; //예보시각
  int pop; // 강수확률(%)
  int pty; // 강수형태(없음:0, 비:1, 비/눈:2, 눈:3)
  int reh; // 습도(%)
  int sky; // 하늘상태(맑음:1, 구름조금:2, 구름많음:3, 흐림:4)
  float t3h; //3시간기온
} ForecastSpace; //동네예보

typedef struct {
  float tmn; //일최저기온(섭씨)
  float tmx; //일최고기온(섭씨) 
} TempMaxMin; //일최고/최저기온(섭씨)

class KMAWeatherClient : public XMLVisitor {
  private:
    // forecast
    boolean isForecast = false;
    int totalCount;
    String resultMsg;
    String baseDate;
    String baseTime;
    time_t baseDateTime;
    ForecastSpace fcstToday[6]; //Today:(6,9,12,15,18,21),Tomorrow:(0,3,6,9,12,15,18,21) 
    ForecastSpace fcstTomorrow[8];
    TempMaxMin tempMaxMin[2]; //Today:0, Tomorrow:1
    
  protected:
    void doUpdate(String url);

  public:

    KMAWeatherClient();
    virtual ~KMAWeatherClient() {}

    inline ForecastSpace* getForecastToday(int index){
      return &fcstToday[index];
    }

    inline ForecastSpace* getForecastTomorrow(int index){
      return &fcstTomorrow[index];
    }
    inline TempMaxMin* getTempMaxMinToday(){
      return &tempMaxMin[0];
    }
    inline TempMaxMin* getTempMaxMinTomorrow(){
      return &tempMaxMin[1];
    }
    
    void updateForecast(String serviceKey, String nx, String ny, String baseDate, String baseTime, int numOfRows);
    String urlEncode(String str);
    String urlDecode(String str);
    unsigned char h2int(char c);
    inline int GetTotalPages(int numOfRows){
      Serial.println(">>totalCount: " + totalCount);
      return (int)ceil((double)this->totalCount/numOfRows);
    }
    
    inline int IndexOfForcastSpace(String fcstDate, String fcstTime){
      if(fcstDate.equals(baseDate)){ //Today
        if(String("0600").equals(fcstTime)){
          return 0;
        } else if(String("0900").equals(fcstTime)){
          return 1;
        } else if(String("1200").equals(fcstTime)){
          return 2;
        } else if(String("1500").equals(fcstTime)){
          return 3;
        } else if(String("1800").equals(fcstTime)){
          return 4;
        } else if(String("2100").equals(fcstTime)){
          return 5;
        }
      } else { //Tomorrow
        if(String("0000").equals(fcstTime)){
          return 0;
        } else if(String("0300").equals(fcstTime)){
          return 1;
        } else if(String("0600").equals(fcstTime)){
          return 2;
        } else if(String("0900").equals(fcstTime)){
          return 3;
        } else if(String("1200").equals(fcstTime)){
          return 4;
        } else if(String("1500").equals(fcstTime)){
          return 5;
        } else if(String("1800").equals(fcstTime)){
          return 6;
        } else if(String("2100").equals(fcstTime)){
          return 7;
        }
      }
    }

    inline time_t MakeTime(const char* date, const char* time){
      return MakeTime(String(date), String(time));
    }
    
    inline time_t MakeTime(const String date, const String time){
      //date: 20160607 (yyyyMMdd, time: 0200 (hhmm)
      tmElements_t tm;
      tm.Year = CalendarYrToTm(date.substring(0,4).toInt());
      tm.Month = date.substring(4, 6).toInt();
      tm.Day = date.substring(6).toInt();
      tm.Hour = time.substring(0, 2).toInt();
      tm.Minute = time.substring(2).toInt();

      return makeTime(tm);
    }
    
    virtual bool VisitEnter( const XMLElement& element, const XMLAttribute* attribute);
    //virtual bool VisitExit( const XMLElement& element);
    //virtual bool Visit( const XMLText& text);

    String getMeteoconIcon(String iconText);
    //sky:하늘상태(맑음:1, 구름조금:2, 구름많음:3, 흐림:4), pty:강수형태(없음:0, 비:1, 비/눈:2, 눈:3)
    String getWeatherIcon(int sky, int pty, int hour);
};


