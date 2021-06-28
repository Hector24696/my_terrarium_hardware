#include <SPI.h>
#include <WiFi.h>
#include "ArduinoJson.h"
#include <DHT.h>


#define DHTPIN 13     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

char ssid[] = "dlink-E980";
char pass[] = "qhlqg92724";

int status = WL_IDLE_STATUS;
IPAddress server(192, 168, 0, 104);
//char server[] = "http://my_terrarium.test";
WiFiClient client;


unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 10L * 1000L;

String response = "";

void setup() {
  Serial.begin(9600);
  dht.begin();
  while (!Serial) { // Sólo necesario para Arduino Leonardo
    ;
  }
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true);
  }
  String fv = WiFi.firmwareVersion();
  if (fv != "1.1.0") {
    Serial.println("Please upgrade the firmware");
  }
  Serial.print(("Firmware version: "));
  Serial.println(WiFi.firmwareVersion());

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }
  Serial.println("Connected to WiFi");
  printWifiStatus();
}

void loop() {

  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }
  // if ten seconds have passed since your last connection,
  // then connect again and send data:
  if (millis() - lastConnectionTime > postingInterval) {
    getParameter();
  }
}

void getParameter() {
  char result [20];
  client.stop();
  Serial.println("\nStarting connection to server...");
  if (client.connect(server, 8080)) {
    String parameters= getRequest("/get-parameter");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, parameters);
    char* nombre;
    int valor;
    int comparador;
    int sensorValue;

    for(int i=0; i<=1; i++){
      nombre = doc[i]["name"];
      valor = doc[i]["value"];
      sensorValue=sensorParameters(nombre);
      comparador=comparator(valor,sensorValue);
      Serial.print("Valor establecido: ");
      Serial.print(valor);
      if(strcmp(nombre, "temperature")==0){
        Serial.print("ºC");
      }else{
        Serial.print("%");
      }
      Serial.print(" Valor sensor: ");
      Serial.print(sensorValue);
      if(strcmp(nombre, "temperature")==0){
        Serial.print("ºC");
      }else{
        Serial.print("%");
      }
      if(strcmp(nombre, "temperature")==0){
        if(comparador == 1){
          Serial.println(" Enciende ventilador/apaga termostato");
        }else if (comparador == -1){
          Serial.println(" Apaga ventilador/enciende termostato");
        }else{
          Serial.println(" Apaga ventilador/apaga termostato");
        }
      }else{
        if(comparador == 1){
          Serial.println(" Apaga aspersor");
        }else if (comparador == -1){
          Serial.println(" Enciende aspersor");
        }else{
          Serial.println(" Apaga aspersor");
        }
      }
    }
    //sensores{"temperature": 13,"humidity": 13}
    //sensorData(sensores[doc[i]["name"]],doc[i]["name"]); -->> compare(SensV,doc[i]["value"])
    //llamar a comparador con los valores
    //if actuator state change postStateActuator(); if(c==1){activar X, desactivar Y
    client.println("Connection: close");
    //client.println();
    lastConnectionTime = millis();
  } else {
    Serial.println("connection failed");
  }
}
int comparator(int valor, int sensorValue){
  if(valor<sensorValue){
    return 1;
  }else if(valor>sensorValue){
    return -1;
  }else{
    return 0;
  }
}
int sensorParameters(String nombre){
    //devolver map con elementos
    //POSTEAR PARAMETROS EN DDBB
    if(nombre == "temperature"){
      return round(dht.readTemperature());
    }else{
      return round(dht.readHumidity());
    }
}

String getRequest(String ruta) {
  client.println("GET "+ ruta);
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      delay(60000);
      return;
    }
  }
  String line = "";
  while (client.available())
  {
    line = client.readStringUntil('\r');
  }
  return line;
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
