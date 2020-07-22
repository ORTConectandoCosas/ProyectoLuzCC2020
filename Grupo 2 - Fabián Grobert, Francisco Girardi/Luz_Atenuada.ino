// includes de bibliotecas para comunicación
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>


//  configuración datos wifi 
// decomentar el define y poner los valores de su red y de su dispositivo
#define WIFI_AP "user"
#define WIFI_PASSWORD "password"

//  configuración datos thingsboard
#define NODE_NAME "ESP8266 Luz Atenuante"   //nombre que le pusieron al dispositivo cuando lo crearon
#define NODE_TOKEN "4oTUWSYB2L4b3CySw4nB"   //Token que genera Thingboard para dispositivo cuando lo crearon


char thingsboardServer[] = "demo.thingsboard.io";

/*definir topicos.
 * telemetry - para enviar datos de los sensores
 * request - para recibir una solicitud y enviar datos 
 * attributes - para recibir comandos en baes a atributtos shared definidos en el dispositivo
 */
char telemetryTopic[] = "v1/devices/me/telemetry";
char requestTopic[] = "v1/devices/me/rpc/request/+";  //RPC - El Servidor usa este topico para enviar rquests, cliente response
char attributesTopic[] = "v1/devices/me/attributes";  // Permite recibir o enviar mensajes dependindo de atributos compartidos


// declarar cliente Wifi y PubSus
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// declarar variables control loop (para no usar delay() en loop
int peopleCount = 0;
unsigned long lastSend;
const int elapsedTime = 1000; // tiempo transcurrido entre envios al servidor


void setup()
{
  Serial.begin(9600);
  pinMode(D1, OUTPUT); 

  connectToWiFi();
  client.setServer( thingsboardServer, 1883 );
  reconnect_wifi();

  client.setCallback(on_message);
   
  lastSend = 0;
}

void loop()
{
  if ( client.connected() != 1 ) {
    reconnect_wifi();
  }
  checkLightLevel();
  client.loop();
  delay(500); 
}

void checkLightLevel(){
  if(peopleCount > 0){
     int lightLevel = analogRead(A0); 
     Serial.println(lightLevel);    
    if(lightLevel >= 800){
     analogWrite(D1, lightLevel); 
    } else {
    analogWrite(D1, LOW);
    }
  } else {
    analogWrite(D1,LOW);
  }
}

void on_message(const char* topic, byte* payload, unsigned int length) 
{
  Serial.println("On message");

  char json[length + 1];
  strncpy (json, (char*)payload, length);
  json[length] = '\0';

  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(json);

  
  StaticJsonDocument<256> doc;
     deserializeJson(doc, payload);

  String methodName = String((const char*)doc["method"]);

 processData(methodName);
 
}

void processData(String methodName){

  if(methodName.equals("apagar")){
    peopleCount = 0;
  } else if(methodName.equals("prender")){
    peopleCount = 1;
  }

  Serial.println(peopleCount);
}

/*
 * funcion para reconectarse al servidor de thingsboard y suscribirse a los topicos de RPC y Atributos
 */
void reconnect_wifi() {
  int statusWifi = WL_IDLE_STATUS;
  // Loop until we're reconnected
  while ( client.connected() != 1 ) {
    statusWifi = WiFi.status();
    connectToWiFi();
    
    Serial.print("Connecting to ThingsBoard node ...");
    // Attempt to connect (clientId, username, password)
    if ( client.connect(NODE_NAME, NODE_TOKEN, NULL) ) {
      Serial.println( "[DONE]" );
      
      // Suscribir al Topico de request
      client.subscribe(requestTopic); 
      client.subscribe(attributesTopic);
      
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}

/*
 * función para conectarse a wifi
 */
void connectToWiFi()
{
  if(WiFi.status() != WL_CONNECTED){
  Serial.println("Connecting to WiFi ...");
  // attempt to connect to WiFi network

  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  }
}
