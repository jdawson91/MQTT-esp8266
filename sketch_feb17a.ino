#include <DallasTemperature.h>
#include <OneWire.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/****************************** Heater Pins ******************************************/
int Heater[] = {2, 5, 4};

/****************************** Temp ******************************************/
#define REPORT_INTERVAL 10 // in sec
#define NO_SENSORS 3
#define ONE_WIRE_BUS 13  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "NETGEAR"
#define WLAN_PASS       "def4543aa3"

/************************* MQTT Setup *********************************/

#define AIO_SERVER      "192.168.3.13"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    ""
#define AIO_KEY         ""

/**************** Global State (default settings) **********************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Store the MQTT server, client ID, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
// Set a unique MQTT client ID using the AIO key + the date and time the sketch
// was compiled (so this should be unique across multiple devices for a user,
// alternatively you can manually set this to a GUID or other random value).
const char MQTT_CLIENTID[] PROGMEM  = __TIME__ AIO_USERNAME;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);


/****************************** Feeds ***************************************/

// Setup a feed called 'heaters' for subscribing to changes.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char HEATER_FEED[] PROGMEM = "HEATER_FEED";
Adafruit_MQTT_Subscribe heaters = Adafruit_MQTT_Subscribe(&mqtt, HEATER_FEED);



// function prototypes
void connect(void);
void getTemps();
void sendTeperature(float temp, int i);
void controlHeaters(char *value);

//setup
void setup() {
  for (int i = 0; i < sizeof(Heater) - 1; i++) {
    pinMode(Heater[i], OUTPUT);
  }

  Serial.begin(115200);

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  delay(10);
  Serial.print(F("Connecting to "));
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();

  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());

  // listen for events on the feeds
  mqtt.subscribe(&heaters);

  // connect to adafruit io
  connect();

}



//main
void loop() {
  Adafruit_MQTT_Subscribe *subscription;

  // ping adafruit io a few times to make sure we remain connected
  if (! mqtt.ping(3)) {
    // reconnect to adafruit io
    if (! mqtt.connected())
      connect();
  }

  // this is our 'wait for incoming subscription packets' busy subloop
  while (subscription = mqtt.readSubscription(1000)) {

    if (subscription == &heaters) {
      // convert mqtt ascii payload to int
      char *value = (char *)heaters.lastread;
      Serial.print(F("Received: "));
      Serial.println(value);

      int i = value[1] - 48;
      if (i == 9) {
        getTemps();
      } else {
        controlHeaters(value);
      }
    }
  }
}



void getTemps() {
  float temp;
  for (int i = 0; i < NO_SENSORS; i++) {
    do {
      DS18B20.requestTemperatures();
      temp = DS18B20.getTempCByIndex(i);
    } while (temp == 85.0 || temp == (-127.0));
    sendTeperature(temp, i);
  }


}

void sendTeperature(float temp, int i) {
  Serial.print("Sensor ");
  Serial.print(i);
  Serial.print(" Temperature: ");
  Serial.println(temp);
  String topic = "sensor";
  topic += i;
  char* result;
  //sprintf(result, "%f", temp);
  mqtt.publish(topic.c_str(), String(temp).c_str(), 0);

}

void controlHeaters(char *value) {
  int i = value[0] - 48;
  int current = value[1] - 48;

  // write the current state to the power switch tail
  digitalWrite(Heater[i], current == 1 ? HIGH : LOW);
}

// connect to Pi io via MQTT
void connect() {

  Serial.print(F("Connecting to Pi... "));

  int8_t ret;

  while ((ret = mqtt.connect()) != 0) {

    switch (ret) {
      case 1: Serial.println(F("Wrong protocol")); break;
      case 2: Serial.println(F("ID rejected")); break;
      case 3: Serial.println(F("Server unavail")); break;
      case 4: Serial.println(F("Bad user/pass")); break;
      case 5: Serial.println(F("Not authed")); break;
      case 6: Serial.println(F("Failed to subscribe")); break;
      default: Serial.println(F("Connection failed")); break;
    }

    if (ret >= 0)
      mqtt.disconnect();

    Serial.println(F("Retrying connection..."));
    delay(5000);

  }

  Serial.println(F("Pi Connected!"));

}
