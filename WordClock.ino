#define USE_RTC_CLOCK
#define USE_EXTENDED_DEMO

#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESPNtpClient.h>
#include <time.h>
#include <EEPROM.h>
#include "RTClib.h"
#include <Wire.h>
// if you're using ESP8266 SoC, please download this fix for FastLED library
// https://github.com/FastLED/FastLED/commit/ccd2c7b426179ad50e4e2c0d3f132954fc6aa72c
#include <FastLED.h>
#define DATA_PIN 2
#define NUM_LEDS 114
CRGB strip[NUM_LEDS];

#ifdef USE_EXTENDED_DEMO
#include "ExtDemo.h"
#endif

WiFiManager wifiManager;
ESP8266WebServer server(80);
#define GO_BACK server.sendHeader("Location", "/",true); server.send(302, "text/plane","");

#define HOST_NAME "WordClock"
#define NTP_SERVER "time.google.com"
#define TIME_ZONE TZ_America_New_York

#ifdef USE_RTC_CLOCK
RTC_DS3231 rtc;
#endif

const uint8_t now_word[] = { 112, 111, 110, 109, 108, 107, 255 };       // сегодня
const uint8_t part_mins[] = { 113, 101, 12, 0 };                        // corner minutes LEDs
const uint8_t minutes[11][13] = {
    {  1,   2,   3,   4, 255, 255, 255, 255, 255, 255, 255, 255, 255},  // пять
    { 22,  21,  20,   2,   3,   4, 255, 255, 255, 255, 255, 255, 255},  // десять
    { 24,  25,  26,  27,  28,  29,  30,  31,  32,  33, 255, 255, 255},  // пятнадцать
    { 40,  39,  38,  29,  30,  31,  32,  33, 255, 255, 255, 255, 255},  // двадцать
    { 40,  39,  38,  29,  30,  31,  32,  33,   1,   2,   3,   4, 255},  // двадцать пять
    { 37,  36,  35,  29,  30,  31,  32,  33, 255, 255, 255, 255, 255},  // тридцать
    { 37,  36,  35,  29,  30,  31,  32,  33,   1,   2,   3,   4, 255},  // тридцать пять
    { 17,  16,  15,  14,  13, 255, 255, 255, 255, 255, 255, 255, 255},  // сорок
    { 17,  16,  15,  14,  13,   1,   2,   3,   4, 255, 255, 255, 255},  // сорок пять
    { 24,  25,  26,  23,  22,  21,  20,  19,  18, 255, 255, 255, 255},  // пятьдесят
    { 24,  25,  26,  23,  22,  21,  20,  19,  18,   1,   2,   3,   4}   // пятьдесят пять
};
const uint8_t minutes_word[] = { 7, 8, 9, 10, 11, 255 };                // минут
const uint8_t hours[12][13] = {
    { 89,  88,  87,  86, 255, 255, 255, 255, 255, 255, 255, 255, 255},  // один
    {105, 104, 103, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},  // два
    { 68,  69,  70, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},  // три
    { 90,  91,  92,  93,  94,  95, 255, 255, 255, 255, 255, 255, 255},  // четыре
    { 60,  59,  58,  57, 255, 255, 255, 255, 255, 255, 255, 255, 255},  // пять
    { 46,  47,  48,  49,  50, 255, 255, 255, 255, 255, 255, 255, 255},  // шесть
    { 75,  76,  77,  78, 255, 255, 255, 255, 255, 255, 255, 255, 255},  // семь
    { 73,  74,  75,  76,  77,  78, 255, 255, 255, 255, 255, 255, 255},  // восемь
    { 71,  72,  73,  64,  63,  62, 255, 255, 255, 255, 255, 255, 255},  // девять
    { 67,  66,  65,  64,  63,  62, 255, 255, 255, 255, 255, 255, 255},  // десять
    { 89,  88,  87,  86,  85,  84,  83,  82,  81,  80,  79, 255, 255},  // одиннадцать
    { 96,  97,  98,  85,  84,  83,  82,  81,  80,  79, 255, 255, 255}   // двенадцать
};
const uint8_t hour_word[3][6] = {
    { 45,  44,  43, 255, 255, 255},                                     // час
    { 45,  44,  43,  42, 255, 255},                                     // часа
    { 52,  53,  54,  55,  56, 255},                                     // часов
};
const uint ntp_intervals[] = { 3600, 3600*2, 3600*4, 3600*8, 3600*12, 3600*24 };

CRGB color;
uint8_t mode = 0, ntpInterval = 4, demoColorCnt, demoIndex;
static bool timeSynced = false;
bool rtcIsRunning = false;
time_t demoTime;
int prevHour = -1, prevMinute = -1;
long prevMillis = 0, demoMillis = 0;

void readSettings()
{
    color = CRGB(EEPROM.read(0), EEPROM.read(1), EEPROM.read(2));
    if (color.r==0 && color.g==0 && color.b==0) color = CRGB::Yellow;
    ntpInterval = EEPROM.read(3);
    if (ntpInterval < 1 || ntpInterval > 4) ntpInterval = 4;
}

void writeSettings()
{
    EEPROM.write(0, color.r); EEPROM.write(1, color.g); EEPROM.write(2, color.b);
    EEPROM.write(3, ntpInterval); 
    EEPROM.commit();
}

const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html>
<head>
    <title>Word clock</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <meta name="mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <link rel="icon" href="http://senssoft.com/clock.ico" type="image/x-icon" />
    <link rel="shortcut icon" href="http://senssoft.com/clock.ico" type="image/x-icon" />
    <link rel="apple-touch-icon" href="http://senssoft.com/clock.png" />
    <script>
        function button_click(cmd_id) {
            if (cmd_id == 'set') {
                cmd_id += '&color='+document.getElementById('led_color').value.substring(1);
                cmd_id += '&ntp='+document.getElementById('ntp_interval').value;
                cmd_id += '&mode='+document.getElementById('mode').value;
            }
            window.location.replace('/command?cmd='+cmd_id);
        }
    </script>
    <style>
        table.centeredTable {
            position: absolute;
            height: 75%%;
            width:75%%;
            top: 50%%;
            left: 50%%;
            transform: translate(-50%%, -50%%);
        }
        input, select {
            display: inline-block;
            width: 100%%;
            margin: 0 0.3em 0.3em 0;
        }
        a.colorButton {
            display: inline-block;
            padding: 0.7em 1.7em;
            margin: 0 0.3em 0.3em 0;
            border-radius: 0.2em;
            box-sizing: border-box;
            text-decoration: none;
            font-family: 'Roboto', sans-serif;
            font-weight: normal;
            font-size: 1em;
            box-shadow: inset 0 -0.6em 1em -0.35em rgba(0, 0, 0, 0.17), inset 0 0.6em 2em -0.3em rgba(255, 255, 255, 0.15), inset 0 0 0em 0.05em rgba(255, 255, 255, 0.12);
            text-align: center;
            position: relative;
            vertical-align: middle;
            width: 100%%;
            user-select: none;
            -webkit-user-select: none;
            -khtml-user-select: none;
            -moz-user-select: none;
            -ms-user-select: none;
        }
        a.colorButton:active {
            box-shadow: inset 0 0.6em 2em -0.3em rgba(0, 0, 0, 0.15), inset 0 0 0em 0.05em rgba(255, 255, 255, 0.12);
        }
        @media all and (max-width:30em) {
            a.colorButton {
                display: block;
                margin: 0.4em auto;
            }
        }
        details>summary {
            list-style-type: none;
            outline: none;
            cursor: pointer;
            border: 0px;
        }
        details>summary::before { content: '+ '; }
        details[open]>summary::before { content: '- '; }
    </style>
</head>
<body style="touch-action: pan-x pan-y;">
    <table class="centeredTable">
        <tr style="height: 20%%;">
            <td>Clock color:</td>
            <td>
                <input type="color" id="led_color" value="#%02x%02x%02x"/>
            </td>
        </tr>
        <tr style="height: 20%%;">
            <td>NTP update:</td>
            <td>
                <select id="ntp_interval">
                    <option value="0" %s>Every hour</option>
                    <option value="1" %s>Every two hours</option>
                    <option value="2" %s>Every four hours</option>
                    <option value="3" %s>Every eight hours</option>
                    <option value="4" %s>Twice a day</option>
                    <option value="5" %s>Once a day</option>
                </select>
            </td>
        </tr>
        <tr style="height: 20%%;">
            <td>Mode:</td>
            <td>
                <select id="mode">
                    <option value="0" %s>Normal clock</option>
                    <option value="1" %s>Demo clock</option>
                    <option value="2" %s>All LEDs on</option>
                    <option value="3" %s>Extended demo</option>
                </select>
            </td>
        </tr>
        <tr style="height: 5%%;">
            <td colspan="2">
                <a class="colorButton" onclick="button_click('set');" style="background-color:#E5E7E9">Set</a>
            </td>
            <td/>
        </tr>
        <tr style="height: 5%%;">
            <td colspan="2">
                <details>
                    <summary> </summary>
                    <table style="width: 100%%;">
                        <tr>
                            <td><a class="colorButton" style="background-color: #008000; color: #ffffff;" onclick="button_click('reboot');">Reboot</a></td>
                            <td><a class="colorButton" style="background-color: #0000FF; color: #ffffff;" onclick="button_click('reset');">Reset WiFi</a></td>
                        </tr>
                    </table>
                </details>
            </td>
            <td></td>           
        </tr>
    </table>
</body>
</html>)rawliteral";

#define HTML_SIZE sizeof(index_html)+40
char temp[HTML_SIZE];
void handleRoot() 
{
    String modes[] = {"","","",""};
    modes[mode] = "selected";
#ifndef USE_EXTENDED_DEMO
    modes[3] = "disabled";
#endif    
    String ints[] = {"","","","","",""};
    ints[ntpInterval] = "selected";
    
    snprintf(temp, HTML_SIZE, index_html, 
        color.r, color.g, color.b,                              // format color picker value in hex form
        ints[0], ints[1], ints[2], ints[3], ints[4], ints[5],   // set "selected" for HTML select item
        modes[0], modes[1], modes[2], modes[3]);                // set "selected" for HTML select item
        
    server.send(200, "text/html", temp);
}

void handleCommand()
{
    GO_BACK
    if (server.args() > 0)
    {
        String cmd = server.arg(0);
        if (cmd=="reboot") {
            ESP.restart();
        }
        else if (cmd=="reset") { 
            wifiManager.resetSettings();
            ESP.restart(); 
        }
        else if (cmd=="set") {
            color = strtol(server.arg(1).c_str(), NULL, 16);
            ntpInterval = atoi(server.arg(2).c_str());
            mode = atoi(server.arg(3).c_str());

            if (color.r==0 && color.g==0 && color.b==0) color = CRGB::Yellow;
            if (ntpInterval > 5) ntpInterval = 4;
            if (mode > 3) mode = 0;
            writeSettings();
            
            struct tm t = {0, 0, 0, 0, 0, 0};
            demoTime = mktime(&t);
            NTP.setInterval (ntp_intervals[ntpInterval]);
            prevMinute = -1;
            demoColorCnt = 0;
            demoMillis = millis();
            demoIndex = NUM_LEDS;
        }
    }
}

void setLEDs(const uint8_t leds[])
{
    for (int i=0; i<13; i++)
        if (leds[i]>NUM_LEDS) break; else strip[leds[i]] = color;
}

void updateClock(bool forceUpdate = false)
{
    if (mode == 2)
    {
        fill_solid(strip, NUM_LEDS, (demoColorCnt==0 ? CRGB::Red : (demoColorCnt==1 ? CRGB::Green : CRGB::Blue )));
        if (demoColorCnt++ >= 2) demoColorCnt = 0;
        FastLED.show();
    }
    else if (mode < 2)
    {
        time_t timenow;
        if (mode == 1) timenow = demoTime; else time(&timenow);
        struct tm *ti;
        ti = localtime(&timenow);  
        DateTime now = DateTime(ti->tm_year, ti->tm_mon, ti->tm_mday, ti->tm_hour, ti->tm_min, ti->tm_sec);
    
        // If NTP time synced, update RTC clock
        if (mode == 0)
        {
            if (timeSynced) 
            {
                timeSynced = false;
#ifdef USE_RTC_CLOCK            
                if (rtcIsRunning) rtc.adjust(now);
#endif            
            }
#ifdef USE_RTC_CLOCK        
            if (rtcIsRunning) now = rtc.now();
#endif        
        }
    
        int h = now.twelveHour();
        int m = now.minute();
        
        if (h != prevHour || m != prevMinute || forceUpdate)
        {
            prevHour = h;
            prevMinute = m;
    
            FastLED.clear(true);
            
            setLEDs(now_word);
            
            setLEDs(hours[h-1]);
            if (h==1) setLEDs(hour_word[0]);
            else if (h<5) setLEDs(hour_word[1]);
            else setLEDs(hour_word[2]);
    
            if (m/5 > 0) setLEDs(minutes[m/5-1]);
            for (int i=0; i<m%5; i++) strip[part_mins[i]] = color;
            if (m > 4) setLEDs(minutes_word);    
            
            FastLED.show();
        }
    }
}

void setup() 
{
    Serial.begin(115200);

    EEPROM.begin(32);
    readSettings();

    // Setup FastLED    
    FastLED.addLeds<WS2811, DATA_PIN, GRB>(strip, NUM_LEDS);
    FastLED.clear(true);
    FastLED.setBrightness(250);    
    FastLED.show(); 

#ifdef USE_RTC_CLOCK    
    // Start RTC
    rtcIsRunning = rtc.begin();
#endif    

    WiFi.mode(WIFI_STA);
    wifiManager.setHostname(HOST_NAME);
    wifiManager.setConfigPortalBlocking(false);
    wifiManager.autoConnect(HOST_NAME);
    
    // setup NTP client
    NTP.setTimeZone (TIME_ZONE);
    NTP.setInterval (ntp_intervals[ntpInterval]);
    NTP.setNTPTimeout (1000);
    NTP.onNTPSyncEvent ([](NTPEvent_t ntpEvent){ timeSynced = (ntpEvent.event==timeSyncd); });
    NTP.begin(NTP_SERVER);

    // Setup web server
    server.on("/", handleRoot);
    server.on("/command", handleCommand);
    server.on("/favicon.ico", []() { server.sendHeader("Location", "http://senssoft.com/clock.ico",true); server.send(302, "text/plane",""); });
    server.onNotFound( [](){ GO_BACK });
    server.begin();

    ArduinoOTA.setHostname(HOST_NAME);
    ArduinoOTA.begin();
}

void loop() 
{
    wifiManager.process();
    server.handleClient();
    ArduinoOTA.handle();  
      
    if (mode < 3 && millis()-prevMillis >= 1000)
    {
        if (mode == 1) demoTime += 30;
        prevMillis = millis();
        updateClock();
    }
    
    // Switch to normal clock after five minutes of demo mode
    if (mode > 0 && millis() - demoMillis >= 5*60*1000) mode = 0;

#ifdef USE_EXTENDED_DEMO
    if (mode == 3) processDemo();
#endif    
}
