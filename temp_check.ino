#include <Adafruit_GFX.h>       // OLED Screen
#include <Adafruit_SSD1306.h>   // OLED Screen
#include <OneWire.h>            // DS18820
#include <ESP8266WiFi.h>        // WiFi
#include "DHT.h"                // DHT22 tempriture

#define OLED_RESET  0           // GPIO 0

Adafruit_SSD1306  OLED(OLED_RESET);
OneWire ds(D4);                 // on pin D4(a 4.7K resistor is necessary)

const char* ssid = "FreeWifi-SKT";
const char* host = "api.thingspeak.com";
const char* THINGSPEAK_API_KEY = "X3VTCVN02TU6WSLH";

// DHT Settings
// #define DHTPIN D6
#define DHTPIN 2                // What digital pin we're connected to. If you are not using NodeMCU change D6 to real pin

// Uncomment whatever type you're using!
// #define DHTTYPE DHT11         // DHT11
// #define DHTTYPE DHT21         // DHT21 (AM2301)
#define DHTTYPE DHT22            // DHT22 (AM2302), AM2321

const boolean IS_METRIC = true;
const int UPDATE_INTERVAL_SECONDS = 20;     // Update every 600 seconds = 10 minutes. Min with Thingspeak is ~20 seconds

DHT dht(DHTPIN, DHTTYPE);        // Initailze the Temperature/Humidity senso

void setup(){
  Serial.begin(9600);
  
  OLED.begin();
  OLED.clearDisplay();

  delay(10);

  // We start by connecting to a WiFi network
  Serial.println("");
  Serial.println("");
  Serial.println("Connecting to ");
  Serial.println(ssid);

  // WiFi.begin(ssid, password);      // if you need password. 
  WiFi.begin(ssid);                   // free wifi

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("[a].");
  }
  
  Serial.println("");
  Serial.println("============= WiFi connected ============");
  Serial.println("== IP address: ");
  Serial.println(WiFi.localIP());
}

void loop(void){
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;

  if(!ds.search(addr)){
    ds.reset_search();
    delay(250);
    return;
  }

  if(OneWire::crc8(addr, 7) != addr[7]){
    Serial.println("CRC is not valid");
    return;
  }
  
  Serial.println();

  // the first ROM byte indicates which chip
  switch(addr[0]){
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18*20 family device.");
      return;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);       // start conversion, with parasite power on at the end
  delay(1000);
  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad

  for(i=0; i<9; i++){
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  int16_t raw = (data[1] << 8) | data[0];

  if(type_s){
    raw = raw << 3;       // 9 bit resolution default
  
    if(data[7] == 0x10){
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    if(cfg == 0x00) raw = raw & ~7;       // 9 bit resolution, 93.75 ms
    else if(cfg == 0x20) raw = raw & ~3;  // 10 bit res, 187.5 ms
    else if(cfg == 0x40) raw = raw & ~1;  // 11 bit res, 375 ms
  }

  celsius = (float)raw/16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  Serial.print(" Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.println(fahrenheit);
  Serial.println(" Fahrenheit");

  // OLED Print
  OLED.begin();
  OLED.clearDisplay();

  // Add stuff into the 'display buffer'
  OLED.setTextWrap(false);
  OLED.setTextSize(1);
  OLED.setTextColor(WHITE);
  OLED.setCursor(0, 0);
  OLED.println("Temperature");
  OLED.setTextSize(2);
  OLED.print(celsius);
  OLED.println("`C");
  OLED.display();                       // output 'display buffer' to screen
  OLED.startscrollleft(0x00, 0x0F);     // make display scroll

  // WiFi start
  Serial.print("== connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;

  if(!client.connect(host, httpPort)){
    Serial.println("---------- connection failed! -------------");
    return;
  }

  // Read values from the sensor for DHT22
  // dealy(200);
  // float humidity = dht.readHumidity();
  // float temperature = dht.readTemperature();
  // float fahrenheit = dht.readTemperature(IS_METRIC);
  // Serial.print("== ");
  // Serial.print(humidity);
  // Serial.print(",");
  // Serial.print(temperature);
  // Serial.print(",");
  // Serial.print(fahrenheit);
  
  // Create a URI for the request.
  String url = "/update?api_key=";
  url += THINGSPEAK_API_KEY;
  url += "&field1=";
  url += String(celsius);
  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will s ned the request to the server.
  client.print(String("GET ") + url + "HTTP/1/1\r\n" + 
                      "Host:" + host + "\r\n" +
                      "Connetion: close\r\n\r\n");
  delay(10);

  // wifi reconnection code add 2021.03.19
  if( !WiFi.isConnected() ) {
    Serial.println( "Disconnected!" );
    WiFi.reconnect();
    WiFi.waitForConnectResult();
  }

  while(!client.available()){
    delay(100);
    Serial.println("[b].");
  }

  // Read all the lines of the replay from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println("");
  Serial.println("closing connection");

  // Go back to sleep. If your sensor is battery powered you mithg want to use deep sleep here
  delay(1000 * UPDATE_INTERVAL_SECONDS);
}
