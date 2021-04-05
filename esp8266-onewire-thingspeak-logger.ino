/** The MIT License (MIT)

Copyright (c) 2015 by Daniel Eichhorn

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

See more at http://blog.squix.ch
**/

#include <ESP8266WiFi.h>          // WiFi setup
// #include "DHT.h"                  // Temperature sensor 
#include <OneWire.h>              // Temperature sensor only(DS18B20) 
#include <DallasTemperature.h>    // Temperature sensor only(DS18B20) 

#include <Wire.h>                 // display 
#include <Adafruit_GFX.h>         // display 
#include <Adafruit_SSD1306.h>     // display 
// #include <Adafruit_Sensor.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/***************************
 * Begin Settings
 **************************/
const char* ssid = "FreeWiFi-SKT";
const char* host = "api.thingspeak.com";
const char* THINGSPEAK_API_KEY = "X3VTCVN02TU6WSLH";

// DHT Settings
// #define DHTPIN D6    // what digital pin we're connected to. If you are not using NodeMCU change D6 to real pin
// #define DHTPIN 2     // what digital pin we're connected to. If you are not using NodeMCU change D6 to real pin

// DS18B20 온도 센서의 데이터선인 가운데 핀을 아두이노 3번에 연결합니다.   
#define ONE_WIRE_BUS 2

//1-wire 디바이스와 통신하기 위한 준비  
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
    
// Uncomment whatever type you're using! > As you use One-wire, you don't need all of DHT.
// #define DHTTYPE DHT11   // DHT 11
// #define DHTTYPE DHT22   // DHT 22 (AM2302), AM2321
// #define DHTTYPE DHT21   // DHT 21 (AM2301)

// const boolean IS_METRIC = true;     // ???

// Update every 600 seconds = 10 minutes. Min with Thingspeak is ~20 seconds
// const int UPDATE_INTERVAL_SECONDS = 600;
const int UPDATE_INTERVAL_SECONDS = 3000;

/***************************
 * End Settings
 **************************/
 
// Initialize the temperature/ humidity sensor
// DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  // WiFi.begin(ssid, password);
  WiFi.begin(ssid);
  // WiFi.begin(password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);               // delay 1sec
    Serial.print("connecting...");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // display setup
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    // for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
}

void loop() {      
    Serial.print("connecting to ");
    Serial.println(host);
    
    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;

    if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // read values from the sensor <- DHT sensor use
    // float humidity = dht.readHumidity();   // DS18B20 not support humidity 
    // float temperature = dht.readTemperature(!IS_METRIC);

    // Send the command to get temperatures
    sensors.requestTemperatures(); 
    float temperature = sensors.getTempCByIndex(0);

    //print the temperature in Celsius
    Serial.print("Temperature: ");
    Serial.print(temperature);
    // Serial.print((char)176);//shows degrees character
    Serial.println("`C");

    // We now create a URI for the request
    String url = "/update?api_key=";
    url += THINGSPEAK_API_KEY;
    url += "&field1=";
    url += String(temperature);
    url += "&field2=";
    // url += String(humidity);
    // Serial.print("======= temperature : ");
    // Serial.println(temperature);
    // Serial.print("======= humidity: ");
    // Serial.println(humidity);
    
    Serial.print("Requesting URL: ");
    Serial.println(url);
    
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" 
                + "Host: " + host + "\r\n" 
                + "Connection: close\r\n\r\n");
    
    delay(10);

    while(!client.available()){
      delay(100);
      Serial.print(".");
    }
    // Read all the lines of the reply from server and print them to Serial
    while(client.available()){
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }

    //clear display
    display.clearDisplay();

    // display temperature
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print("Temperature: ");
    display.setTextSize(2);
    display.setCursor(0,10);
    display.print(temperature);
    display.print(" ");
    display.setTextSize(1);
    display.cp437(true);
    display.write(167);
    display.setTextSize(2);
    display.print("C");

    
    display.setTextSize(1);
    display.setCursor(0, 35);
    
    // wifi reconnection code add 2021.03.19
    if( !WiFi.isConnected() ) {
      Serial.println( "WiFi Disconnected!" );
      display.print("WiFi Disconnected !!!");
      WiFi.reconnect();
      WiFi.waitForConnectResult();
    }
    display.print("WiFi connected!");
    display.setCursor(0, 45);
    display.print("https://thingspeak.com/");
    
    display.display(); 
    
    Serial.println();
    Serial.println("============= Requset done! ==============");

  // Go back to sleep. If your sensor is battery powered you might
  // want to use deep sleep here
  delay(10 * UPDATE_INTERVAL_SECONDS);
}
