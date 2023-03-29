#include "DHT.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//Wifi configuration
const char *ssid =  " <ssid> ";  					//replace <ssid>  with your value
const char *password =  " <password> ";				//replace <password> with your value

//MQTT settings
const char *mqtt_server = "mqtt3.thingspeak.com";		//replace with your value
const char *mqttclientId = " <mqttclientId> ";			//replace <mqttclientId> with your value
const char *mqttUser = "  <mqttUser> ";				//replace <mqttUser> with your value
const char *mqttPassword = "  <mqttPassword> ";			//replace <mqttPassword> with your value

/*
  PIN - VALUE
  D0   = 16;
  D1   = 5;
  D2   = 4;
  D3   = 0;
  D4   = 2;
  D5   = 14;
  D6   = 12;
  D7   = 13;
  D8   = 15;
  D9   = 3;
  D10  = 1;
*/

#define DHTPIN 5      //D1
#define DHTTYPE DHT11 

#define ENGINE 15     //D8

#define VIN 3.3 // V power voltage
#define R 10000 //ohm resistance value

unsigned long lastMsg = 0;
int sensorLight;

//dht is library for read from humidity and temperature sensor
DHT dht(DHTPIN, DHTTYPE);

//WiFiClient is library for connect to wifi
WiFiClient espclient;

//PubSubClient is library for mqtt connection
PubSubClient client(espclient);

//set wifi connection
void setup_wifi() {
  Serial.println();
  Serial.print("Wifi connecting to ");
  Serial.println( ssid );

  WiFi.begin(ssid,password);

  Serial.println();
  Serial.print("Connecting");

  while( WiFi.status() != WL_CONNECTED ){
      delay(500);
      Serial.print(".");        
  }

  Serial.println("Wifi Connected Success!");
  Serial.print("Node IP Address : ");
  Serial.println(WiFi.localIP() );

}

//used for SUBSCRIBE
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  //If received 1 activate engine, otherwise I turn it off
  if ((char)payload[0] == '1') {
      analogWrite(ENGINE,255);
  } else {
      analogWrite(ENGINE,0);
  }

}

//read the values from the sensors, send to thingspeak and print on the screen
void read_and_send_message(){
  sensorLight = analogRead(A0); // read analog input pin 0

  // Read humidity as percentage  
  float h = dht.readHumidity();

  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t) || isnan(sensorLight) ) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  //convert value in lumen
  int lux = sensorRawToPhys(sensorLight);
  
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("C  "));
  Serial.print(F("Light: "));
  Serial.print(lux); 
  Serial.println(" \n"); 

  //prepare string message for send with MQTT publish
  String postStr = "field1="+String(t)+"&field2="+String(h)+"&field3="+String(lux)+"&status=MQTTPUBLISH";

  //send message and print on Serial 
  if(client.publish("channels/1974770/publish", postStr.c_str())){
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" degrees Celcius, Humidity: ");
    Serial.print(h);
    Serial.print("%, Light: ");
    Serial.print(lux); // prints the value read
    Serial.println(". Send to Thingspeak.");
  }
  else{
    Serial.println("Error sending values");
  }
 
  Serial.println("Waiting...");
}

//used for connection to MQTT
void connect_to_mqtt(){

  while (!client.connected()) {
      Serial.println("Connecting to MQTT...");
  
      if (client.connect(mqttclientId, mqttUser, mqttPassword )) {
  
        Serial.println("connected");  
        client.subscribe("channels/1974770/subscribe/fields/field4");

      } else {
  
        Serial.print("failed with state ");
        Serial.print(client.state());
        delay(2000);
  
      }
  }
}

//convert raw value from light sensor to lumen
int sensorRawToPhys(int raw){
  float Vout = float(raw) * (VIN / float(1023));  // Conversion analog to voltage
  float RLDR = (R * (VIN - Vout))/Vout;   // Conversion voltage to resistance
  int phys=500/(RLDR/1000);  // Conversion resitance to lumen
  return phys;
}


void setup() {
  
  Serial.begin(9600);
  pinMode(ENGINE, OUTPUT);
  Serial.println(F("Starting setup.."));

  dht.begin();
  //setup wifi connection
  setup_wifi();

  //configure MQTT comunication
  client.setServer(mqtt_server, 1883);

  //set subscribe function
  client.setCallback(callback);

}


void loop() {

  if (!client.connected()) {
    connect_to_mqtt();
  }
  client.loop();

  //send message each 30 seconds
  unsigned long now = millis();                              
  if (now - lastMsg > 30000) {
    lastMsg = now;
    read_and_send_message();
  }
  

}
