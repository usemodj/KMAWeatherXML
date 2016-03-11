
## Required Libraries

Arduino Library Manager:
  Arduino IDE > Sketch > Include Library > Manage Libraries ...

- Json Streaming Parser by Daniel Eichhorn
: https://github.com/squix78/json-streaming-parser
> Arduino library for parsing potentially huge json streams on devices with scarce memory

- ESP8266 Oled Driver for SSD1306 display by Daniel Eichhorn
: https://github.com/squix78/esp8266-oled-ssd1306
> Driver for the SSD1306 based 128x64 pixel OLED display running on the Arduino/ESP8266 platform http://blog.squix.ch

- Ticker Built-In by Ivan Grokhtokov
> Allows to call functions with a given interval

## Required Hardware

- ESP8266 ESP-12E Module + Adapter Plate for PCB board
- 128X64 0.96inch I2C OLED Display SSD1306
- 3.3V 2A(2000mA) DC Power Adapter
 

## 한국(South Korea) 기상청 날씨예보서비스

 공공데이터포털( www.data.go.kr ) 의 오픈 API 서비스 중에서 
 OPEN API: **(신)동네예보정보조회서비스** 
 를 이용한 지역날씨 예보를 위해 ESP8266 모듈과 OLED 디스플레이를 사용해 
오늘과 내일의 지역날씨를 3시간 단위로 보여주는 사물인터넷(IoT) 서비스

OPEN API 서비스를 받기 위해서 먼저 www.data.go.kr 에서 사용자 등록을 하고
[동네예보정보조회서비스](https://www.data.go.kr/subMain.jsp?param=T1BFTkFQSUAxNTAwMDA5OQ==#/L3B1YnIvdXNlL3ByaS9Jcm9zT3BlbkFwaURldGFpbC9vcGVuQXBpTGlzdFBhZ2UkQF4wMTJtMSRAXnB1YmxpY0RhdGFQaz0xNTAwMDA5OSRAXmJybUNkPU9DMDAxMyRAXnJlcXVlc3RDb3VudD0zNjYxJEBeb3JnSW5kZXg9T1BFTkFQSQ==)
화면에서 **(신)동네예보정보조회서비스** 활용신청을 하고 **일반 인증키 발급**을 신청합니다.
발급된 **일반 인증키** 를 KMSWeatherJSON.ino 파일의 KMA_SERVICE_KEY="일반 인증키"로 설정합니다.
예보지점 X좌표, Y좌표는 **예보지점의 X-Y좌표.xlsx** 파일에서 자신이 원하는 좌표값으로 
KMA_NX="X좌표", KMA_NY="Y좌표"로 설정합니다.

[www.usemodj.com](http://www.usemodj.com) 블로그에서 사진과 자료를 확인할 수 있습니다.



