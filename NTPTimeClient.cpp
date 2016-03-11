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

#include "NTPTimeClient.h"

NTPTimeClient::NTPTimeClient(float utcOffset) {
  this->myUtcOffset = utcOffset;
}

void NTPTimeClient::updateTime() {
  udp.begin(localPort);
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    this->epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(this->epoch);
    this->localMillisAtUpdate = millis();
    //set time
    setTime(getCurrentEpochWithUtcOffset());
  }
  
}

// send an NTP request to the time server at the given address
unsigned long NTPTimeClient::sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
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

String NTPTimeClient::getYear(){
  return String(year());
}
String NTPTimeClient::getMonth(){
  int nMonth = month();
  if(nMonth < 10){
    return "0" + String(nMonth);
  }
  return String(nMonth);
}
String NTPTimeClient::getDay(){
  int nDay = day();
  if(nDay < 10){
    return "0" + String(nDay);
  }
  return String(nDay);
}

String NTPTimeClient::getFormattedDate(String seperator) {
  return getYear() + seperator + getMonth() + seperator + getDay();
}

String NTPTimeClient::getHours() {
    if (this->epoch == 0) {
      return "--";
    }
    int hours = hour(); //((getCurrentEpochWithUtcOffset()  % 86400L) / 3600) % 24;
    if (hours < 10) {
      return "0" + String(hours);
    }
    return String(hours); // print the hour (86400 equals secs per day)

}
String NTPTimeClient::getMinutes() {
    if (this->epoch == 0) {
      return "--";
    }
    int minutes = minute(); // ((getCurrentEpochWithUtcOffset() % 3600) / 60);
    if (minutes < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      return "0" + String(minutes);
    }
    return String(minutes);
}
String NTPTimeClient::getSeconds() {
    if (this->epoch == 0) {
      return "--";
    }
    int seconds = second(); //getCurrentEpochWithUtcOffset() % 60;
    if ( seconds < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      return "0" + String(seconds);
    }
    return String(seconds);
}

String NTPTimeClient::getFormattedTime(String seperator) {
  return getHours() + seperator + getMinutes() + seperator + getSeconds();
}

unsigned long NTPTimeClient::getCurrentEpoch() {
  return this->epoch + ((millis() - this->localMillisAtUpdate) / 1000);
}

unsigned long NTPTimeClient::getCurrentEpochWithUtcOffset() {
  return getCurrentEpoch() + round(3600 * this->myUtcOffset);
}


