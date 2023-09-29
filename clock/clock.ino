#include <NTPClient.h>
#include <WiFi.h>
#include <FastLED.h>

#define LED_PIN 23   // 定義燈帶資料腳位
#define NUM_LEDS 88 // LED燈數量


CRGBArray<NUM_LEDS> leds;

const int strip = 2;  //單條LED數量
const int utc = 8; //設定時區

const char *ssid =  "";     // replace with your wifi ssid and wpa2 key
const char *pass =  "";

const char *ntpServer = "time.stdtime.gov.tw"; // NTP站，可自行更改
const long utcOffsetInSeconds = utc * 3600; // 設置時區偏移，以毫秒為單位
const int updateTime = 15; // 請求間隔(秒)
const int mistake = 0; //誤差調整(非必要)

WiFiUDP udp;

NTPClient timeClient(udp, ntpServer, utcOffsetInSeconds);

void setup() {
  Serial.begin(115200);
  // 如果您的燈帶不是WS2812B，請自行更改您所使用的燈帶型號(備註: WS2815 須將 GRB 改成 RGB)
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2815, LED_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(50); // 設置亮度，根據需要調整

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  timeClient.begin();
  timeClient.update();
}

void number(int site, int number) { 
  int from = site * ((strip * 7) - 1);
  if (site != 0) {
    from = from + site;
  }

  const char numberMap[10][6] = { //定義熄滅區域
    "G",        // 0
    "ADEFG",    // 1
    "CF",       // 2
    "EF",       // 3
    "ADE",      // 4
    "BE",       // 5
    "B",        // 6
    "DEFG",     // 7
    "",         // 8
    "E"         // 9
  };

  light(from, numberMap[number]);
}

void light(int from, const char let[]) {
  int ledMap[7] = {
    strip * 0,
    strip * 1,
    strip * 2,
    strip * 3,
    strip * 4,
    strip * 5,
    strip * 6
  };

  for (int i = 0; i < strlen(let); i++) {
    int ledIndex = ledMap[let[i] - 'A'];
    for (int i = 0; i < strip; i++) {
      leds[from + ledIndex + i] = CRGB(0, 0, 0);
    }
  }
}

String formatToTwoDigits(int number) {
  if (number < 10) {
    return "0" + String(number);
  } else {
    return String(number);
  }
}

void displayTime(String h, String m, String s) { //顯示位置
  String timeArray[] = {h, m, s};

  for (int i = 0; i < 3; i++) {
    number(i * 2, timeArray[i].charAt(0) - '0');
    number(i * 2 + 1, timeArray[i].charAt(1) - '0');
  }
}


//時鐘延遲
const unsigned long clockDelayTime = 1000;
unsigned long clockPreviousDelayTime = 0;

//燈條延遲
unsigned long stripDelayTime = 50; //特效延遲
unsigned long stripPreviousDelayTime = 0;

static uint8_t beginHue = 0;
int deltaHue = 1; //控制流速


int updateDelay = 15;
bool clockTrigger = false;
bool stripTrigger = false;

int currentHour = timeClient.getHours();
int currentMinute = timeClient.getMinutes();
int currentSecond = timeClient.getSeconds();

int blinking = (6 * ((strip * 7) - 1)) + 6;

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - clockPreviousDelayTime >= clockDelayTime) {//時鐘緩衝器
    clockTrigger = true;
    clockPreviousDelayTime = currentTime;
  }

  if (currentTime - stripPreviousDelayTime >= stripDelayTime) {// 特效緩衝器
    stripTrigger = true;
    stripPreviousDelayTime = currentTime;
  }

  if (clockTrigger == true) {
    currentSecond++;
    updateDelay++;

    if (updateDelay >= updateTime) { // 校時
      timeClient.update();
      currentHour = timeClient.getHours();
      currentMinute = timeClient.getMinutes();
      currentSecond = timeClient.getSeconds();

      currentSecond += mistake; // 誤差調整

      updateDelay = 0;
    }

    if (currentSecond >= 60) {
      currentSecond = 0;
      currentMinute++;

      if (currentMinute >= 60) {
        currentMinute = 0;
        currentHour++;

        if (currentHour >= 24) {
          currentMinute = 0;
        }
      }
    }
    //如果註解最下方的Seria，則須在此定義clockTrigger = false
    //clockTrigger = false;
  }

  String formattedHour = formatToTwoDigits(currentHour);
  String formattedMinute = formatToTwoDigits(currentMinute);
  String formattedSecond = formatToTwoDigits(currentSecond);
  /*-------------------------------------------------------*/
  //特效，此處可自行更改數字的顯示效果

  //leds(0, NUM_LEDS) = CRGB(255, 255, 255);
  fill_rainbow(leds, NUM_LEDS, beginHue, deltaHue);
  
  if (stripTrigger == true) {
    beginHue += 1;
    if (beginHue == 256) {
      beginHue = 0;
    }
    stripTrigger = false;
  }
  /*-------------------------------------------------------*/
  displayTime(formattedHour, formattedMinute, formattedSecond); // 顯示時、分、秒（帶補零）

  if (currentSecond % 2 == 0) { // 閃爍
    leds[blinking+0] = CRGB(0, 0, 0);
    leds[blinking+1] = CRGB(0, 0, 0);
    leds[blinking+2] = CRGB(0, 0, 0);
    leds[blinking+3] = CRGB(0, 0, 0);
  }

  FastLED.show();

  if (clockTrigger == true) { // Serial
    Serial.print("Current time: ");
    Serial.print(formattedHour);
    Serial.print(":");
    Serial.print(formattedMinute);
    Serial.print(":");
    Serial.println(formattedSecond);
    clockTrigger = false;
  }
}
