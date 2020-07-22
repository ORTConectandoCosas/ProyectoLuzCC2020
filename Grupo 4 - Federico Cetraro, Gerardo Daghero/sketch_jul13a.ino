#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>

#define WIFI_SSID "Guri's AirPort Express"
#define WIFI_PASSWORD "guriguri"
#define TOKEN "45SKm5WY4RQIsmqVE72P"
#define CLIENT_NAME "eb4989b0-c606-11ea-bf35-41d006a14e9a"

char* thingsboardServer = "demo.thingsboard.io";
char* requestTopic = "v1/devices/me/rpc/request/+";

LiquidCrystal_I2C lcd(0x27, 16, 1);
WiFiClient wifiClient;
PubSubClient client(wifiClient);

int switchPin = 14;       
int ledPin = 2;
int buzzerPin = 16;
              
int switchStatusLast = HIGH;  
int switchStatus;
int ledStatus = LOW;         

void setup() {
  Serial.begin(9600);
  initializeDisplay();
  initializePins();
  digitalWrite(buzzerPin, HIGH);
  connectToWiFi();
  client.setServer(thingsboardServer, 1883);
  reconnect_wifi();
  client.setCallback(on_message);
}

void loop() {
  if (!client.connected()) {
    reconnect_wifi();
  }
  client.loop();
}

void noAlertState()
{
  digitalWrite(buzzerPin, HIGH);
  lcd.clear();
  lcd.noBacklight();
}

void alertState(){
  bool soundState = false;
  int messageLength = 36;
  lcd.on();
  lcd.backlight();
  lcd.home();
  lcd.print("                LIMPIEZA SOLICITADA!");
  while (!isButtonPressed()) {
    if (soundState) {
      digitalWrite(buzzerPin, LOW);
    } else {
      digitalWrite(buzzerPin, HIGH);
    }
    soundState = !soundState;
    lcd.scrollDisplayLeft();
    delay(350);
  }
  noAlertState();
}

bool isButtonPressed() {
  switchStatus = digitalRead(switchPin);
  if (switchStatus != switchStatusLast) {
      delay(50); 
      int switchStatus = digitalRead(switchPin); 
       if (switchStatus != switchStatusLast) {
        ledStatus = !ledStatus;
        digitalWrite(ledPin, ledStatus);
        switchStatus = switchStatusLast;
        return true;
     }
  }
  return false;
}

void on_message(const char* topic, byte* payload, unsigned int length) {
  Serial.print("llega");
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload);
  String message = doc["message"]; 
  if (message.equals("alertState")) {
    alertState();
  }
}

void reconnect_wifi() {
  int statusWifi = WL_IDLE_STATUS;
  while (!client.connected()) {
    statusWifi = WiFi.status();
    connectToWiFi();
    Serial.print("Connecting to ThingsBoard node... ");
    if (client.connect(CLIENT_NAME, TOKEN, NULL)) {
      Serial.println("[DONE]");
      client.subscribe(requestTopic); 
      Serial.println("Subscribed to request's topic, awaiting messages...");
    } else {
      Serial.print("[FAILED] [ rc = ");
      Serial.print(client.state());
      Serial.println("");
      Serial.println(" : retrying in 5 seconds]");
      delay(5000);
    }
  }
}

void initializePins() {
  pinMode(ledPin, OUTPUT);
  pinMode(switchPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
}

void connectToWiFi() {
  if (WiFi.status() != WL_CONNECTED){
    Serial.print("Connecting to WiFi ...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Connected to WiFi");
  }
}

void initializeDisplay() {
  Wire.begin(4, 5);
  lcd.init();
  lcd.noBacklight();
}
