# ソーラーパネルデータ送信プログラム 
このプログラムは、ESP-12F WeMos D1 WiFi Arduino Uno互換機に接続されたソーラーパネルの電力データを計測し、[http://solaris.f5.si/に一定間隔で送信します。]() 
## 概要 

- デバイス：ESP-12F WeMos D1 WiFi Arduino Uno互換機
 
- 接続先URL：[solaris.f5.si](http://solaris.f5.si/)

- 計測データ：ワット数 (電圧・電流から計算)

- 測定間隔：15分ごと

## 機能 
 
- **電力計測** ：シャント抵抗を用いて電圧降下を測定し、電力（W）を計算
 
- **データ送信** ：計測した電力データをHTTP POSTリクエストで送信
 
- **ディスプレイ出力** ：ST7735ディスプレイに計測結果を表示
 
- **Webサーバー** ：デバイス上でHTTPサーバーを起動し、データ履歴表示用のWebインターフェースを提供

## 必要なライブラリ 

以下のライブラリが必要です。Arduino IDEの「ライブラリを管理」からインストールしてください。
 
- `ESP8266WiFi.h`
 
- `ESP8266HTTPClient.h`
 
- `ESP8266WebServer.h`
 
- `ESP8266mDNS.h`
 
- `NTPClient.h`
 
- `Adafruit_GFX.h`
 
- `U8g2_for_Adafruit_GFX.h`
 
- `Adafruit_ST7735.h`

## ハードウェア接続 
 
- **ソーラーパネル** ：シャント抵抗を通してA0ピンに接続
 
- **ディスプレイ** ：ST7735ディスプレイ
  - TFT_CS：ピン15

  - TFT_RST：ピン5

  - TFT_DC：ピン4

  - TFT_SCLK：ピン13

  - TFT_MOSI：ピン11
設定ファイル (`config.h`)`config.h`で以下のWi-Fi情報を設定します：

```cpp
#define STASSID "your-SSID"
#define STAPSK "your-PASSWORD"
```

## 使い方 

1. Arduino IDEでソースコードを開き、必要なライブラリをインストールします。
 
2. `config.h`ファイルを編集し、Wi-FiのSSIDとパスワードを設定します。

3. ESP-12F WeMos D1 WiFi Arduino互換機にコードをアップロードします。

4. シリアルモニタ（9600ボーレート）で動作状況を確認できます。

5. 15分ごとに計測した電力データが自動的に送信されます。

## データ表示 
 
- **ディスプレイ** ：電圧、電流、電力が画面に表示されます。
 
- **Webサーバー** ：`http://esp8266.local/`にアクセスすると、電力送信履歴や設定が表示されます。
