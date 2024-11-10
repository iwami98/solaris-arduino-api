#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClientSecure.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_GFX.h>  // Core graphics library
#include <U8g2_for_Adafruit_GFX.h>
#include <Adafruit_ST7735.h>  // Hardware-specific library
#include <SPI.h>
#include "config.h"

#define TFT_CS 15
#define TFT_RST 5  // you can also connect this to the Arduino reset
#define TFT_DC 4
#define TFT_SCLK 13  // set these to be whatever pins you like!
#define TFT_MOSI 11  // set these to be whatever pins you like!

#ifndef CONFIG_H
#define STASSID STASSID
#define STAPSK STAPSK
#endif

float p = 3.1415926;
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

WiFiUDP udp;
NTPClient timeClient(udp, "ntp.nict.jp", 32400, 60000);  // JST (UTC+9)
const int voltagePin = A0;                               // シャント抵抗の電圧降下を測定するアナログ入力ピン
const long interval = 60000;                             // 1分 (60秒 x 1000ミリ秒)
const char* ssid = STASSID;
const char* password = STAPSK;
const char* url = "http://solaris.f5.si/api/energy";
const char* js = "<script>"
                 "function toggleSidebar() {"
                 "  var sidebar = document.getElementById('sidebar');"
                 "  var container = document.getElementById('container');"
                 "  sidebar.classList.toggle('hidden');"
                 "  container.classList.toggle('expanded');"
                 "}"
                 "</script>";
const char* sideBar = "<div class='header'>"
                      "  <button class='toggle-btn' onclick='toggleSidebar()'>☰</button>"
                      "  <h1 style='color: white; margin: 0 auto;'>Solaris Info</h1>"
                      "</div>"
                      "<div class='sidebar' id='sidebar'>"
                      "  <ul>"
                      "    <li><a href='/'>HOME</a></li>"
                      "    <li><a href='/'>LOGS</a></li>"
                      "    <li><a href='setting'>SETTING</a></li>"
                      "  </ul>"
                      "</div>";
const String cssStyle = "<style>"
                        "body {"
                        "  font-family: Arial, sans-serif;"
                        "  margin: 0;"
                        "  padding: 0;"
                        "  background-color: #2a3d55;"
                        "  display: flex;"
                        "}"
                        ".sidebar {"
                        "  width: 200px;"
                        "  background: #333;"
                        "  color: #fff;"
                        "  height: 100vh;"
                        "  padding: 20px 0;"
                        "  position: fixed;"
                        "  transition: transform 0.3s ease;"
                        "  transform: translateX(0);"
                        "}"
                        ".sidebar.hidden {"
                        "  transform: translateX(-100%);"
                        "}"
                        ".sidebar ul {"
                        "  list-style: none;"
                        "  padding: 0;"
                        "}"
                        ".sidebar ul li {"
                        "  padding: 15px 20px;"
                        "}"
                        ".sidebar ul li a {"
                        "  color: #fff;"
                        "  text-decoration: none;"
                        "  display: block;"
                        "}"
                        ".sidebar ul li a:hover {"
                        "  background: #575757;"
                        "}"
                        ".container {"
                        "  margin-left: 220px;"
                        "  width: calc(100% - 220px);"
                        "  margin-top: 50px;"
                        "  background: #2a3d55;"
                        "  padding: 20px;"
                        "  box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);"
                        "  transition: margin-left 0.3s ease, width 0.3s ease;"
                        "}"
                        ".container.expanded {"
                        "  margin-left: 0;"
                        "  width: 100%;"
                        "  border: none;"
                        "}"
                        ".header {"
                        "  width: 100%;"
                        "  height: 50px;"
                        "  background: #333;"
                        "  color: #fff;"
                        "  display: flex;"
                        "  align-items: center;"
                        "  justify-content: space-between;"
                        "  position: fixed;"
                        "  top: 0;"
                        "  left: 0;"
                        "  z-index: 1000;"
                        "  padding: 0 20px;"
                        "}"
                        ".toggle-btn {"
                        "  background: #333;"
                        "  border: none;"
                        "  color: #fff;"
                        "  font-size: 16px;"
                        "  cursor: pointer;"
                        "  padding: 10px;"
                        "  transition: background 0.3s ease;"
                        "}"
                        ".toggle-btn:hover {"
                        "  background: #575757;"
                        "}"
                        "h1 {"
                        "  text-align: center;"
                        "  color: #333;"
                        "}"
                        "form {"
                        "  display: flex;"
                        "  flex-direction: column;"
                        "}"
                        "label {"
                        "  margin-bottom: 10px;"
                        "  color: #555;"
                        "}"
                        "input, textarea {"
                        "  margin-bottom: 20px;"
                        "  padding: 10px;"
                        "  border: 1px solid #ddd;"
                        "  border-radius: 5px;"
                        "  font-size: 16px;"
                        "}"
                        "input[type='submit'] {"
                        "  background-color: #5cb85c;"
                        "  color: white;"
                        "  border: none;"
                        "  cursor: pointer;"
                        "  transition: background-color 0.3s ease;"
                        "}"
                        "input[type='submit']:hover {"
                        "  background-color: #4cae4c;"
                        "}"
                        "</style>";

// NTPクライアントの設定
WiFiUDP ntpUDP;

unsigned long previousMillis = 0;  // 直前のミリ秒

ESP8266WebServer server(80);

const int led = 13;

void handleRoot() {
  server.send(200, "text/html",
              "<html>"
              "<head>"
              "<meta charset='UTF-8'>"
                + cssStyle + "</head>"
                             "<body>"
                + sideBar + "<div class='container' id='container'>"
                            "  <form method='post' action='./' enctype='multipart/form-data'>"
                            "    <h2 style='color: white; margin: 0 auto;'>履歴一覧</h2>"
                            "    <h3 style='color: white; margin: 0 auto;'>電力の送信履歴</h3>"
                            // "    <label for='title' style='color: #b8bfce;'>タイトル</label>"
                            // "    <input type='text' id='title' name='title' required>"
                            "    <input type='hidden' value='テスト送信' id='title' name='title'>"
                            "    <label for='message' style='color: #b8bfce;'>一覧</label>"
                            "    <textarea id='message' name='message' rows='4' required value='テスト送信'></textarea>"
                            // "    <label for='date' style='color: #b8bfce;'>通知日</label>"
                            // "    <input type='date' id='date' name='date' required>"
                            "  </form>"
                            "</div>"
                + js + "</body>"
                       "</html>");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) { message += " " + server.argName(i) + ": " + server.arg(i) + "\n"; }
  server.send(404, "text/plain", message);
}

void setup(void) {
  Serial.begin(9600);
  // Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB);  // initialize a ST7735S chip, black tab
  tft.setRotation(1);         // ディスプレイの向きを横向きに設定

  uint16_t time = millis();
  tft.fillScreen(ST7735_BLACK);
  time = millis() - time;

  Serial.println(time, DEC);
  delay(500);

  u8g2_for_adafruit_gfx.begin(tft);

  u8g2_for_adafruit_gfx.setFontMode(0);
  u8g2_for_adafruit_gfx.setFontDirection(0);
  u8g2_for_adafruit_gfx.setForegroundColor(ST7735_WHITE);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_b12_t_japanese1);
  u8g2_for_adafruit_gfx.setCursor(0, 10);

  shortPrintln("プログラム開始");
  shortPrintln("Serial start.");
  delay(1000);
  shortPrintln("ネットワーク接続中...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected to ");
  Serial.println(ssid);
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  if (MDNS.begin("esp8266")) { Serial.println("MDNS responder started"); }

  server.on("/", handleRoot);

  server.on("/setting", []() {
    server.send(200, "text/html",
                "<html>"
                "<head>"
                "<meta charset='UTF-8'>"
                  + cssStyle + "</head>"
                               "<body>"
                  + sideBar + "<div class='container' id='container'>"
                              "  <form method='post' action='./settings' enctype='multipart/form-data'>"
                              "    <h2 style='color: white; margin: 0 auto;'>SETTING</h2>"
                              "    <label for='animetionFlag' style='color: #b8bfce;'>アニメーションフラグ</label>"
                              "    <input type='text' id='animetionFlag' name='animetionFlag' required>"
                              "    <input type='submit' value='保存' style='width: 100px; margin: 0 auto;'>"
                              "  </form>"
                              "</div>"
                  + js + "</body>"
                         "</html>");
  });

  server.on("/gif", []() {
    static const uint8_t gif[] PROGMEM = {
      0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x10, 0x00, 0x10, 0x00, 0x80, 0x01,
      0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x2c, 0x00, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x10, 0x00, 0x00, 0x02, 0x19, 0x8c, 0x8f, 0xa9, 0xcb, 0x9d,
      0x00, 0x5f, 0x74, 0xb4, 0x56, 0xb0, 0xb0, 0xd2, 0xf2, 0x35, 0x1e, 0x4c,
      0x0c, 0x24, 0x5a, 0xe6, 0x89, 0xa6, 0x4d, 0x01, 0x00, 0x3b
    };
    char gif_colored[sizeof(gif)];
    memcpy_P(gif_colored, gif, sizeof(gif));
    // Set the background to a random set of colors
    gif_colored[16] = millis() % 256;
    gif_colored[17] = millis() % 256;
    gif_colored[18] = millis() % 256;
    server.send(200, "image/gif", gif_colored, sizeof(gif_colored));
  });

  server.onNotFound(handleNotFound);

  /////////////////////////////////////////////////////////
  // Hook examples

  server.addHook([](const String& method, const String& url, WiFiClient* client, ESP8266WebServer::ContentTypeFunction contentType) {
    (void)method;       // GET, PUT, ...
    (void)url;          // example: /root/myfile.html
    (void)client;       // the webserver tcp client connection
    (void)contentType;  // contentType(".html") => "text/html"
    Serial.printf("A useless web hook has passed\n");
    Serial.printf("(this hook is in 0x%08x area (401x=IRAM 402x=FLASH))\n", esp_get_program_counter());
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });

  server.addHook([](const String&, const String& url, WiFiClient*, ESP8266WebServer::ContentTypeFunction) {
    if (url.startsWith("/fail")) {
      Serial.printf("An always failing web hook has been triggered\n");
      return ESP8266WebServer::CLIENT_MUST_STOP;
    }
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });

  server.addHook([](const String&, const String& url, WiFiClient* client, ESP8266WebServer::ContentTypeFunction) {
    if (url.startsWith("/dump")) {
      Serial.printf("The dumper web hook is on the run\n");

      // Here the request is not interpreted, so we cannot for sure
      // swallow the exact amount matching the full request+content,
      // hence the tcp connection cannot be handled anymore by the
      // webserver.
#ifdef STREAMSEND_API
      // we are lucky
      client->sendAll(Serial, 500);
#else
      auto last = millis();
      while ((millis() - last) < 500) {
        char buf[32];
        size_t len = client->read((uint8_t*)buf, sizeof(buf));
        if (len > 0) {
          Serial.printf("(<%d> chars)", (int)len);
          Serial.write(buf, len);
          last = millis();
        }
      }
#endif
      // Two choices: return MUST STOP and webserver will close it
      //                       (we already have the example with '/fail' hook)
      // or                  IS GIVEN and webserver will forget it
      // trying with IS GIVEN and storing it on a dumb WiFiClient.
      // check the client connection: it should not immediately be closed
      // (make another '/dump' one to close the first)
      Serial.printf("\nTelling server to forget this connection\n");
      static WiFiClient forgetme = *client;  // stop previous one if present and transfer client refcounter
      return ESP8266WebServer::CLIENT_IS_GIVEN;
    }
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });

  // Hook examples
  /////////////////////////////////////////////////////////

  timeClient.begin();
  server.begin();
  Serial.println("HTTP server started");
  shortPrintln("接続できました!");
  delay(3000);
  tft.initR(INITR_BLACKTAB);  // initialize a ST7735S chip, black tab
}

void handleRedirect() {
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void sendEnergy() {
  HTTPClient http;
  WiFiClient wifiClient;
  int statusCode = 0;
  http.begin(wifiClient, url);
  http.addHeader("Content-Type", "application/json");
  String payload = "";

  // 送信するデータをJSON形式で作成
  float energy = calcWatt();
  String postData = "{\"energy\":" + (String)energy + "}";
  int httpCode = http.POST(postData);

  if (httpCode > 0) {
    payload = http.getString();
    server.send(httpCode, "text/plain", payload);
  } else {
    server.send(500, "text/plain", "Failed to connect");
  }
  Serial.println("");
  Serial.println(payload);

  http.end();
  longPrintln(payload.c_str());
  
}

void longPrintln(const char* inputString) {
  int charCount = 0;     // 文字数をカウント
  int bytePosition = 0;  // バイト位置を追跡
  int line = 0;

  tft.fillScreen(ST7735_BLACK);
  u8g2_for_adafruit_gfx.setCursor(0, 10);

  while (inputString[bytePosition] != '\0') {  // 文字列の終わりまでループ
    if ((inputString[bytePosition] & 0x80) == 0) {
      // ASCII文字（1バイト）
      u8g2_for_adafruit_gfx.print(inputString[bytePosition]);
      Serial.println(inputString[bytePosition]);
      bytePosition++;
    } else {
      // 日本語の句読点の確認と置換
      if (inputString[bytePosition] == (char)0xE3 && inputString[bytePosition + 1] == (char)0x80 && inputString[bytePosition + 2] == (char)0x82) {
        // 「、」を「,」に置換
        u8g2_for_adafruit_gfx.print(',');
        Serial.print(',');
        bytePosition += 3;
      } else if (inputString[bytePosition] == (char)0xE3 && inputString[bytePosition + 1] == (char)0x80 && inputString[bytePosition + 2] == (char)0x81) {
        // 「。」を「.」に置換
        u8g2_for_adafruit_gfx.print('.');
        Serial.print('.');
        bytePosition += 3;
      } else {
        // それ以外のマルチバイト文字
        int bytesToWrite = (inputString[bytePosition] & 0xF0) == 0xE0 ? 3 : 4;  // 3または4バイト
        u8g2_for_adafruit_gfx.write(reinterpret_cast<const uint8_t*>(inputString + bytePosition), bytesToWrite);
        Serial.write(reinterpret_cast<const uint8_t*>(inputString + bytePosition), bytesToWrite);
        bytePosition += bytesToWrite;
      }
    }

    charCount++;
    // 13行ごとにリセット
    if (line == 12) {
      delay(500);
      tft.fillScreen(ST7735_BLACK);
      u8g2_for_adafruit_gfx.setCursor(0, 10);
      line = 0;
    }
    // 10文字ごとに改行
    if (charCount % 22 == 0) {
      u8g2_for_adafruit_gfx.println();
      line++;
    }
  }
  delay(1000);
}

void shortPrint(String text) {
  tft.fillScreen(ST7735_BLACK);
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print(text);
  delay(1000);
}

void shortPrintln(String text) {
  tft.fillScreen(ST7735_BLACK);
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.println(text);
  delay(1000);
}

float calcWatt() {
  const float shuntResistance = 6;                       // シャント抵抗の値（単位：オーム）
  int sensorValue = analogRead(voltagePin);          // 電圧降下を取得
  float voltageDrop = sensorValue * (3.2 / 1023.0);  // 電圧に変換（5V基準の場合）
  float current = voltageDrop / shuntResistance;     // オームの法則で電流を計算
  float watt = voltageDrop * current;
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(1);  // ディスプレイの向きを横向きに設定
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  String printV = "電圧:" + (String)voltageDrop + "V";
  String printA = "電流:" + (String)current + "A";
  String printW = "電力:" + (String)watt + "W";

  u8g2_for_adafruit_gfx.println();
  u8g2_for_adafruit_gfx.println();
  u8g2_for_adafruit_gfx.println();
  u8g2_for_adafruit_gfx.println();
  u8g2_for_adafruit_gfx.println();
  u8g2_for_adafruit_gfx.println("          Solaris");
  u8g2_for_adafruit_gfx.println("   http://solaris.f5.si/");
  u8g2_for_adafruit_gfx.println();
  u8g2_for_adafruit_gfx.println();
  u8g2_for_adafruit_gfx.println("         " + printV);
  u8g2_for_adafruit_gfx.println("         " + printA);
  u8g2_for_adafruit_gfx.println("         " + printW);
  return watt;
}

void sunAnimetion() {
  // Set the position of the sun in the top-right corner
  int16_t sunX = tft.width() - 20;
  int16_t sunY = 20;
  tft.fillScreen(ST77XX_BLACK);
  calcWatt();

  // Draw the sun
  //drawSun(sunX, sunY, 10);
  tft.fillCircle(sunX, sunY, 10, ST77XX_YELLOW);
}

void loop(void) {

  // 最後に表示した時刻
  unsigned long lastPrintTime = 0;
  const unsigned long post_interval = 900000;  // 15分をミリ秒に変換
  unsigned long currentMillis = millis();

  timeClient.update();
  server.handleClient();
  MDNS.update();

  // 15分経過したかどうかをチェック
  if (currentMillis - previousMillis >= post_interval) {
    // NTPクライアントの初期化
    timeClient.begin();
    tft.initR(INITR_BLACKTAB);  // initialize a ST7735S chip, black tab
    String currentTime = timeClient.getFormattedTime();
    sendEnergy();
    previousMillis = currentMillis;
  } else {
    calcWatt();
  }
  delay(10000);
}
