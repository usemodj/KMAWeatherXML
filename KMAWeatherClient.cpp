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
#include <WiFiClient.h>

#include "KMAWeatherClient.h"

KMAWeatherClient::KMAWeatherClient(){
  
}

/*
 * http://newsky2.kma.go.kr/service/SecndSrtpdFrcstInfoService2/ForecastSpaceData?ServiceKey=서비스키&base_date=20151021&base_time=0230&nx=1&ny=1
 */

void KMAWeatherClient::updateForecast(String serviceKey, String nx, String ny, String baseDate, String baseTime, int numOfRows) {
  isForecast = true;
  int pageNo = 1;
  String url = "/service/SecndSrtpdFrcstInfoService2/ForecastSpaceData?ServiceKey="
  + serviceKey + "&base_date=" + baseDate + "&base_time=" + baseTime + "&nx=" + nx + "&ny=" + ny
  + "&numOfRows=" + String(numOfRows) + "&pageNo=";
  Serial.println(url);

  this->baseDate = baseDate;
  this->baseTime = baseTime;
  this->baseDateTime = MakeTime(baseDate, baseTime);
  do {
    doUpdate(url + String(pageNo));
    Serial.println(">>Page: " + String(pageNo) + " /TotalPages: " + String(GetTotalPages(numOfRows)));
    pageNo++;
  } while(pageNo <= GetTotalPages(numOfRows));
}

void KMAWeatherClient::doUpdate(String url) {
  XMLDocument xmlDoc;
  
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect("newsky2.kma.go.kr", httpPort)) {
    Serial.println("KMA connection failed");
    return;
  }

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: newsky2.kma.go.kr\r\n" +
               "Connection: close\r\n\r\n");
  while(!client.available()) {
    delay(1000);
  }

  int pos = 0;
  boolean isBody = false;
  String xml = "";
  char c;

  int size = 0;
  client.setNoDelay(false);
  while(client.connected()) {
    while((size = client.available()) > 0) {
      c = client.read();
      if (!isBody && c == '<') {
        isBody = true;
      }
      if (isBody) {
        xml += c;
      }
    }
    xml.trim();
    int end = xml.indexOf("</response>");
    xml = xml.substring(0, end + 11);
    Serial.println(xml);
    if(XML_NO_ERROR == xmlDoc.Parse(xml.c_str())){
      xmlDoc.Accept(this);
    } else {
      Serial.println(">>Error ID: " + String(xmlDoc.ErrorID()));
      Serial.println(" Error Str1: " + String(xmlDoc.GetErrorStr1()));
   }
  }
}

String KMAWeatherClient::urlEncode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
    
}

String KMAWeatherClient::urlDecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}

unsigned char KMAWeatherClient::h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}
bool KMAWeatherClient::VisitEnter( const XMLElement& element, const XMLAttribute* attribute )
{
    String name = String(element.Name());
    String text = String(element.GetText());
    text.trim();
//    Serial.println(name);
//    Serial.println(text);
    if(String("totalCount").equals(name)){
      this->totalCount = text.toInt();
      //element.QueryIntText(&totalCount);
    } else if(String("resultMsg").equals(name)){
      this->resultMsg = text;  
    } else if(String("item").equals(name)){
      String fcstDate = String(element.FirstChildElement("fcstDate")->GetText());
      String fcstTime = String(element.FirstChildElement("fcstTime")->GetText());

      time_t fcstDateTime = MakeTime(fcstDate, fcstTime);
      time_t tomorrow = this->baseDateTime + SECS_PER_DAY;
      
      if(this->baseDate.equals(fcstDate)){ //Today
        String category = String(element.FirstChildElement("category")->GetText());
        
        if(String("POP").equals(category)){//강수확률
          fcstToday[IndexOfForcastSpace(fcstDate, fcstTime)].fcstTime = fcstTime;
          element.FirstChildElement("fcstValue")->QueryIntText(&(fcstToday[IndexOfForcastSpace(fcstDate, fcstTime)].pop));
        } else if(String("PTY").equals(category)){//강수형태
          element.FirstChildElement("fcstValue")->QueryIntText(&(fcstToday[IndexOfForcastSpace(fcstDate, fcstTime)].pty));
        } else if(String("REH").equals(category)){//습도
          element.FirstChildElement("fcstValue")->QueryIntText(&(fcstToday[IndexOfForcastSpace(fcstDate, fcstTime)].reh));
        } else if(String("SKY").equals(category)){//하늘상태
          element.FirstChildElement("fcstValue")->QueryIntText(&(fcstToday[IndexOfForcastSpace(fcstDate, fcstTime)].sky));
        } else if(String("TMN").equals(category)){//일초저기온
          //element.FirstChildElement("fcstValue")->QueryFloatText(&(tempMaxMin[0].tmn));
          tempMaxMin[0].tmn = String(element.FirstChildElement("fcstValue")->GetText()).toFloat();
          //Serial.println(">>TMN: " + String(tempMaxMin[0].tmn));
        } else if(String("TMX").equals(category)){//일최고기온
          //element.FirstChildElement("fcstValue")->QueryFloatText(&(tempMaxMin[0].tmx));
          tempMaxMin[0].tmx = String(element.FirstChildElement("fcstValue")->GetText()).toFloat();
          //Serial.println(">>TMX: " + String(tempMaxMin[0].tmx));
        } else if(String("T3H").equals(category)){//3시간기온
          //element.FirstChildElement("fcstValue")->QueryFloatText(&(fcstToday[IndexOfForcastSpace(fcstDate, fcstTime)].t3h));
          fcstToday[IndexOfForcastSpace(fcstDate, fcstTime)].t3h = String(element.FirstChildElement("fcstValue")->GetText()).toFloat();
        } 

      } else if( day(fcstDateTime) == day(tomorrow)){ //Tomorrow
        String category = String(element.FirstChildElement("category")->GetText());
        if(String("POP").equals(category)){//강수확률
          fcstTomorrow[IndexOfForcastSpace(fcstDate, fcstTime)].fcstTime = fcstTime;
          element.FirstChildElement("fcstValue")->QueryIntText(&(fcstTomorrow[IndexOfForcastSpace(fcstDate, fcstTime)].pop));
        } else if(String("PTY").equals(category)){//강수형태
          element.FirstChildElement("fcstValue")->QueryIntText(&(fcstTomorrow[IndexOfForcastSpace(fcstDate, fcstTime)].pty));
        } else if(String("REH").equals(category)){//습도
          element.FirstChildElement("fcstValue")->QueryIntText(&(fcstTomorrow[IndexOfForcastSpace(fcstDate, fcstTime)].reh));
        } else if(String("SKY").equals(category)){//하늘상태
          element.FirstChildElement("fcstValue")->QueryIntText(&(fcstTomorrow[IndexOfForcastSpace(fcstDate, fcstTime)].sky));
        } else if(String("TMN").equals(category)){//일초저기온
          //element.FirstChildElement("fcstValue")->QueryFloatText(&(tempMaxMin[1].tmn));
          tempMaxMin[1].tmn = String(element.FirstChildElement("fcstValue")->GetText()).toFloat();
        } else if(String("TMX").equals(category)){//일최고기온
          //element.FirstChildElement("fcstValue")->QueryFloatText(&(tempMaxMin[1].tmx));
          tempMaxMin[1].tmx = String(element.FirstChildElement("fcstValue")->GetText()).toFloat();
        } else if(String("T3H").equals(category)){//3시간기온
          //element.FirstChildElement("fcstValue")->QueryFloatText(&(fcstTomorrow[IndexOfForcastSpace(fcstDate, fcstTime)].t3h));
          fcstTomorrow[IndexOfForcastSpace(fcstDate, fcstTime)].t3h = String(element.FirstChildElement("fcstValue")->GetText()).toFloat();
        } 
       
      } else {
        return false;
      }
    }
    
    return true;
}


//bool KMAWeatherClient::VisitExit( const XMLElement& element )
//{
//    //CloseElement( CompactMode(element) );
//    return true;
//}
//
//
//bool KMAWeatherClient::Visit( const XMLText& text )
//{
//    //PushText( text.Value(), text.CData() );
//    Serial.println(">>Text: " + String(text.Value()));
//    return true;
//}
String KMAWeatherClient::getMeteoconIcon(String iconText) {
  if (iconText == "chanceflurries") return "F";
  if (iconText == "chancerain") return "Q";
  if (iconText == "chancesleet") return "W";
  if (iconText == "chancesnow") return "V";
  if (iconText == "chancetstorms") return "O";
  if (iconText == "clear") return "B";
  if (iconText == "cloudy") return "Y";
  if (iconText == "flurries") return "F";
  if (iconText == "fog") return "M";
  if (iconText == "hazy") return "E";
  if (iconText == "mostlycloudy") return "Y";
  if (iconText == "mostlysunny") return "H";
  if (iconText == "partlycloudy") return "H";
  if (iconText == "partlysunny") return "J";
  if (iconText == "sleet") return "W";
  if (iconText == "rain") return "R";
  if (iconText == "snow") return "W";
  if (iconText == "sunny") return "B";
  if (iconText == "tstorms") return "0";

  if (iconText == "nt_chanceflurries") return "F";
  if (iconText == "nt_chancerain") return "7";
  if (iconText == "nt_chancesleet") return "#";
  if (iconText == "nt_chancesnow") return "#";
  if (iconText == "nt_chancetstorms") return "&";
  if (iconText == "nt_clear") return "2";
  if (iconText == "nt_cloudy") return "Y";
  if (iconText == "nt_flurries") return "9";
  if (iconText == "nt_fog") return "M";
  if (iconText == "nt_hazy") return "E";
  if (iconText == "nt_mostlycloudy") return "5";
  if (iconText == "nt_mostlysunny") return "3";
  if (iconText == "nt_partlycloudy") return "4";
  if (iconText == "nt_partlysunny") return "4";
  if (iconText == "nt_sleet") return "9";
  if (iconText == "nt_rain") return "8";
  if (iconText == "nt_snow") return "#";
  if (iconText == "nt_sunny") return "2";
  if (iconText == "nt_tstorms") return "&";

  return ")";
}

//sky:하늘상태(맑음:1, 구름조금:2, 구름많음:3, 흐림:4), pty:강수형태(없음:0, 비:1, 비/눈:2, 눈:3)
String KMAWeatherClient::getWeatherIcon(int sky, int pty, int hour){
  String iconText;
  switch(sky){ //하늘상태
    case 1: //맑음
      iconText = "sunny";
      break;
    case 2: //구름조금
      iconText = "partlycloudy";
      break;
    case 3: //구름많음
      iconText = "mostlycloudy";
      break;
    case 4: {//흐림
      switch(pty){ //강수형태
        case 0: //없음
          iconText = "hazy";
          break;
        case 1: //비
          iconText = "rain";
          break;
        case 2: //비/눈(진눈깨비)
          iconText = "sleet";
          break;
        case 3: //눈
          iconText = "snow";
          break;
        default:
          iconText = ")"; //N/A
          break;
      }
    }
    break;
    default:
      iconText = ")"; //N/A
      break;
  }
  if(hour < 6 || hour >= 18) //before 6am or after 6 pm
    iconText = "nt_" + iconText;
  return getMeteoconIcon(iconText);
}


