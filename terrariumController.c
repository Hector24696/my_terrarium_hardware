#include <SPI.h>
#include <WiFi.h>
#include "ArduinoJson.h"
#include <DHT.h>


#define DHTPIN 13
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

char ssid[] = "dlink-E980";
char pass[] = "qhlqg92724";

DynamicJsonDocument sensorValues(64);


int status = WL_IDLE_STATUS;
IPAddress server(192, 168, 0, 104);
WiFiClient client;

unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 10L * 1000L;

String response = "";

void setup() {
  Serial.begin(9600);
  dht.begin();
  sensorValues["temperature"] = 0;
  sensorValues["humidity"] = 0;
  while (!Serial) {
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

  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }
  if (millis() - lastConnectionTime > postingInterval) {
    sensorGetParameter();
    getParameter();
    
    postParameters();
  }
  delay(2000);
}

void getParameter() {
  client.stop();
  Serial.println("\nStarting connection to server...");
  if (client.connect(server, 8080)) {
    String parameters = getRequest("/get-parameter");
    DynamicJsonDocument doc(128);
    deserializeJson(doc, parameters);
    serializeJson(doc,Serial);
    char* nombre;
    int valor;
    int comparador;
    int sensorValue;
    DynamicJsonDocument actuadores(72);
    for (int i = 0; i <= 1; i++) {
      nombre = doc[i]["name"];
      valor = doc[i]["value"];
      sensorValue = sensorValues[nombre];
      comparador = comparator(valor, sensorValue);
  
      Serial.print("\nValor establecido: ");
      Serial.print(valor);
      if (strcmp(nombre, "temperature") == 0) {
        Serial.print("ºC");
      } else {
        Serial.print("%");
      }
      
      Serial.print(" Valor sensor: ");
      Serial.print(sensorValue);
      if (strcmp(nombre, "temperature") == 0) {
        Serial.print("ºC");
      } else {
        Serial.print("%");
      }
      //POST ACTUATOR STATE
      if (strcmp(nombre, "temperature") == 0) {
        if (comparador == -1) {
          actuadores[0]["val"]="termostato";
          actuadores[0]["st"]=1;
          actuadores[1]["val"]="ventilador";
          actuadores[1]["st"]=0;
          Serial.println(" Apaga ventilador/enciende termostato");
        } else if (comparador == 1) {
          actuadores[0]["val"]="termostato";
          actuadores[0]["st"]=0;
          actuadores[1]["val"]="ventilador";
          actuadores[1]["st"]=1;
          Serial.println(" Enciende ventilador/apaga termostato");
        } else {
          actuadores[0]["val"]="termostato";
          actuadores[0]["st"]=0;
          actuadores[1]["val"]="ventilador";
          actuadores[1]["st"]=0;
          Serial.println("Apaga ventilador/apaga termostato");
        }
      } else {
        if (comparador == -1) {
          actuadores[2]["val"]="aspersor";
          actuadores[2]["st"]=1;
          Serial.println(" Enciende aspersor");
        } else {
          actuadores[2]["val"]="aspersor";
          actuadores[2]["st"]=0;
          Serial.println(" Apaga aspersor");
        }
      }
    }
    serializeJson(sensorValues,Serial);
    String meme;
    serializeJson(actuadores, meme);
    Serial.println(meme);
    postRequest("/update-actuators", meme);     
    delay(1000);
    //lastConnectionTime = millis();
  } else {
    Serial.println("connection failed");
  }
}

int comparator(int valor, int sensorValue) {
  if (valor < sensorValue) {
    return 1;
  } else if (valor > sensorValue) {
    return -1;
  } else {
    return 0;
  }
}

void sensorGetParameter() {
  sensorValues["temperature"] = sensorParameters("temperature");
  sensorValues["humidity"] = sensorParameters("humidity");
}

int sensorParameters(String nombre) {
  if (nombre == "temperature") {
    return round(dht.readTemperature());
  } else {
    return round(dht.readHumidity());
  }
}

void postRequest(String ruta, String body) {
  client.stop();
  Serial.println("Posts...");
  if (client.connect(server, 8080)) {
    client.println("POST " + ruta + " HTTP/1.1");
    client.println("Host: 192.168.0.104:8080 ");
    client.println("Connection: close");
    client.println("Content-Type: application/json;");
    client.print("Content-Length: ");
    client.println(String(body.length()));
    client.println(); 
    client.println(body);
    delay(1000);
    lastConnectionTime = millis();
  } else {
    Serial.println("connection failed");
  }
}

String getRequest(String ruta) {
  client.println("GET " + ruta);
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

void postParameters(){
    String sens;
    serializeJson(sensorValues, Serial);
    serializeJson(sensorValues, sens);
    Serial.println(sens);
    postRequest("/add-to-historic",sens);
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