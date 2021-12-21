
#include "DHT.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Useful links
// See https://github.com/digicosmos86/wiolink
// See https://github.com/Seeed-Studio/Wio_Link 

// 1. See https://www.allthingstalk.com/faq/get-started-with-seeed-studio-wio-link-board 
// 2. Tools->Board->ESP8266 Boards->Seeed Wio Link

// https://wiki.seeedstudio.com/Grove-TemperatureAndHumidity_Sensor/ 
// Perform Steps 1 -5

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302) connect DHT22 to digital port 2 on wiolink
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT10   // DHT 10
//#define DHTTYPE DHT20   // DHT 20

/*Notice: The DHT10 and DHT20 is different from other DHT* sensor ,it uses i2c interface rather than one wire*/
/*So it doesn't require a pin.*/
#define DHTPIN 13     // what pin we're connected to（DHT10 and DHT20 don't need define it）(Digital 2)
DHT dht(DHTPIN, DHTTYPE);   //   DHT11 DHT21 DHT22
//DHT dht(DHTTYPE);         //   DHT10 DHT20 don't need to define Pin
// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

#define LED 14        //connect LED to digital pin14 (Digital 0)
#define RELAY 12      //connect REAY to digital pin 12 (Digital 1)

#if defined(ARDUINO_ARCH_AVR)
    #define debug  Serial

#elif defined(ARDUINO_ARCH_SAMD) ||  defined(ARDUINO_ARCH_SAM)
    #define debug  SerialUSB
#else
    #define debug  Serial
#endif

// ubidots.com
// shahg, usua Ramandan

#define TOKEN "BBFF-753qtqUi9BcvEjsXya55iF4e8cZ12u"  // Put your TOKEN here
#define DEVICE_LABEL "demo"      // Put your device API Label here 
#define VARIABLE_LABEL_1 "hum"  // Put your first variable API Label here
#define VARIABLE_LABEL_2 "temp"   // Put your second variable API Label here

#define SERVER_IP "192.168.1.42"

#ifndef STASSID
#define STASSID "Home Wifi"
#define STAPSK  "41151581"
#endif

WiFiServer server(80);

void setup() {

    debug.begin(74880);
    Serial.println();
    debug.println("IoT WS Demo");    
    Wire.begin();

    /*if using WIO link,must pull up the power pin.*/
    pinMode(PIN_GROVE_POWER, OUTPUT); 
    digitalWrite(PIN_GROVE_POWER, 1);
    pinMode(LED, OUTPUT); 
    pinMode(RELAY, OUTPUT);

    dht.begin();

    Serial.println();
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(STASSID, STAPSK);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    // Start the server
    server.begin();
    Serial.println(F("Server started"));
    
    Serial.println("");
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
}

void loop() {

    int val;
    
    // Check if a client has connected
    WiFiClient client = server.available();
    //if (!client) {
      //return;
    //}
    Serial.println(F("new client"));

    client.setTimeout(1000); // default is 1000

    // Read the first line of the request
    String req = client.readStringUntil('\r');
    Serial.println(F("request: "));
    Serial.println(req);
 
    // Match the request
    // http://192.168.8.125/gpio/0 Relay off
    // http://192.168.8.125/gpio/1 Relay on
    if (req.indexOf(F("/gpio/0")) != -1) {
      val = 0;
      Serial.println(" val set to 0");
      digitalWrite(RELAY, LOW); // set the LAMP (110 V) on 
    }
    if (req.indexOf(F("/gpio/1")) != -1) {
      val = 1;
      Serial.println(" val set to 1");
      digitalWrite(RELAY, HIGH); // set the LAMP (110 V) on 
    }

    // read/ignore the rest of the request
    // do not client.flush(): it is for output only, see below
    while (client.available()) {
      // byte by byte is not very efficient
      client.read();
    }

    // Send the response to the client
    // it is OK for multiple small client.print/write,
    // because nagle algorithm will group them into one single packet
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n"));
    client.print((val) ? F("high") : F("low"));
    client.print(F("<br><br>Click <a href='http://"));
    client.print(WiFi.localIP());
    client.print(F("/gpio/1'>here</a> TO LET THERE LIGHT, or <a href='http://"));
    client.print(WiFi.localIP());
    client.print(F("/gpio/0'>here</a> FOR ANDHERE RAAT.</html>"));

    // The client will actually be *flushed* then disconnected
    // when the function returns and 'client' object is destroyed (out-of-scope)
    // flush = ensure written data are received by 
    
    float temp_hum_val[2] = {0};
    float hum = 0;
    float temp = 0;
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)


    if (!dht.readTempAndHumidity(temp_hum_val)) {
        debug.print("Humidity: ");
        debug.print(temp_hum_val[0]);
        hum = temp_hum_val[0];
        debug.print(" %\t");
        debug.print("Temperature: ");
        debug.print(temp_hum_val[1]* 1.8 + 32);
        temp = temp_hum_val[1] * 1.8 + 32;
        debug.println(" *F");
    } else {
        debug.println("Failed to get temprature and humidity value.");
    }

    if (temp_hum_val[0] > 90) {
      debug.println("Humidity > 90 ");
      digitalWrite(LED, HIGH);   // set the LED on 
      delay(500);               // for 500ms
      
    }else{
      digitalWrite(LED, LOW);   // set the LED off
    }

    // wait for WiFi connection
    
    if ((WiFi.status() == WL_CONNECTED)) {

      //CLIENT

      WiFiClient client;
      HTTPClient http;

      Serial.print("[HTTP] begin...\n");
      // configure traged server and url
      http.begin(client, "http://things.ubidots.com/api/v1.6/devices/demo");
      http.addHeader("Content-Type", "application/json");
      http.addHeader("X-Auth-Token", "BBFF-753qtqUi9BcvEjsXya55iF4e8cZ12u"); 
      

      Serial.print("[HTTP] POST...\n");
      // start connection and send HTTP header and body
      Serial.println(hum);
      Serial.println(temp, 2);
      String myhum = String(hum);
      String mytemp = String(temp);
     
      //Serial.print(myhum);
      //Serial.print(mytemp);

      char JSON[80];
      sprintf(JSON, "{\"hum\": %s, \"temp\": %s}", myhum, mytemp);
      Serial.println(JSON);
      
      // int httpCode = http.POST("{\"hum\": 44, \"temp\": 55}"); //WORKS
      int httpCode = http.POST(JSON);
      
      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] POST... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK) {
          const String& payload = http.getString();
          Serial.println("received payload:\n<<");
          Serial.println(payload);
          Serial.println(">>");
        } else {
          Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
      }

      delay(500);
  }
} 
