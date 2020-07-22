// includes de bibliotecas para comunicación
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>


//  configuración datos wifi
// decomentar el define y poner los valores de su red y de su dispositivo
#define WIFI_AP "iPhone de Mauri"
#define WIFI_PASSWORD "Mauri1234"

//  configuración datos thingsboard
#define NODE_NAME "MZ$RG"   //nombre que le pusieron al dispositivo cuando lo crearon
#define NODE_TOKEN "1hrjCkn8JMui0DIyYH7s"   //Token que genera Thingboard para dispositivo cuando lo crearon

int DServo = D2;
int ExtractorState = D4;

char thingsboardServer[] = "demo.thingsboard.io";

/*definir topicos.
   telemetry - para enviar datos de los sensores
   request - para recibir una solicitud y enviar datos
   attributes - para recibir comandos en baes a atributtos shared definidos en el dispositivo
*/
char telemetryTopic[] = "v1/devices/me/telemetry";
char requestTopic[] = "v1/devices/me/rpc/request/+";  //RPC - El Servidor usa este topico para enviar rquests, cliente response
char attributesTopic[] = "v1/devices/me/attributes";  // Permite recibir o enviar mensajes dependindo de atributos compartidos


// declarar cliente Wifi y PubSus
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// declarar variables control loop (para no usar delay() en loop
unsigned long lastSend;
unsigned long lastSpray;
unsigned long sprayTime = 10000;
const int elapsedTime = 3000; // tiempo transcurrido entre envios al servidor


void setup() {

  pinMode(ExtractorState, OUTPUT);
  pinMode(DServo, OUTPUT);
  Serial.begin(9600);

  // inicializar wifi y pubsus
  connectToWiFi();
  client.setServer( thingsboardServer, 1883 );
  reconnect();

  // agregado para recibir callbacks
  client.setCallback(on_message);

  lastSend = 0; // para controlar cada cuanto tiempo se envian datos

}

void loop()
{
  if ( !client.connected() ) {
    reconnect();
  }

  if ( millis() - lastSend > elapsedTime ) { // Update and send only after 1 seconds

    // FUNCION DE TELEMETRIA para enviar datos a thingsboard
    getAndSendData();   // FUNCION QUE ENVIA INFORMACIÓN DE TELEMETRIA

    lastSend = millis();
  }

  if (millis() - lastSpray > sprayTime) {
    lastSpray = millis();
    Serial.println("Se aromatizó");
    digitalWrite(DServo, HIGH);
    delay(10);
    digitalWrite(DServo, LOW);

  }


  client.loop();
}

void getAndSendData() {

  // Lectura de sensores
  Serial.println("Verificando Estados.");

  String ServoState = String(digitalRead(DServo));
  String Extractor = String(digitalRead(ExtractorState));


  String payload = "{";
  payload += "\"Estado Servo\":";
  payload += ServoState;
  payload += ",";
  payload += "\"Extractor\":";
  payload += Extractor;
  payload += "}";

  // Enviar el payload al servidor usando el topico para telemetria
  char attributes[100];
  payload.toCharArray( attributes, 100 );
  if (client.publish( telemetryTopic, attributes ) == true)
    Serial.println("publicado ok");
  Serial.println( attributes );

}


void on_message(const char* topic, byte* payload, unsigned int length)
{
  // Mostrar datos recibidos del servidor
  Serial.println("On message");

  char json[length + 1];
  strncpy (json, (char*)payload, length);
  json[length] = '\0';

  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(json);

  // Decode JSON request
  // Notar que a modo de ejemplo este mensaje se arma utilizando la librería ArduinoJson en lugar de desarmar el string a "mano"
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.parseObject((char*)json);

  if (!data.success())
  {
    Serial.println("parseObject() failed");
    return;
  }

  // Obtener el nombre del método invocado, esto lo envia el switch de la puerta y el knob del motor que están en el dashboard
  String methodName = String((const char*)data["method"]);
  String paramName = String((const char*)data["params"]);
  String timeSpray = String((const char*)data["tiempo_spray"]);
  int timeSprayInt = timeSpray.toInt();
  Serial.print("Tiempo Spray:");
  Serial.println(timeSpray);
  Serial.print("Nombre metodo:");
  Serial.println(methodName);
  Serial.print("Nombre param:");
  Serial.println(paramName);

  // Responder segun el método
  processData(methodName, paramName, timeSprayInt);

}

void processData(String methodName, String paramName, int timeSprayInt) {


  if (methodName.equals("spray")) {
    sprayTime = paramName.toInt();
    
  } else if (methodName.equals("forcedFan")) {
    if (paramName.equals("true")) {
      Serial.println("Se encendio el Extractor");
      digitalWrite(ExtractorState, LOW);
    } else {
      Serial.println("Se apago el Extractor");
      digitalWrite(ExtractorState, HIGH);
    }

  } else {
    if (methodName.equals("fan")) {
      if (paramName.equals("true")) {
        Serial.println("Se encendio el Extractor");
        digitalWrite(ExtractorState, LOW);
      } else if (paramName.equals("false")) {
        Serial.println("Se apago el Extractor");
        digitalWrite(ExtractorState, HIGH);
      }
    }

  }
  if(timeSprayInt > 0){
    sprayTime = timeSprayInt*1000;
  }
  if(methodName.equals("tiempo_spray")){
    int manualSpray = paramName.toInt();
    if(manualSpray > 0){
      sprayTime = manualSpray*1000;
    }
  }
}


  /*
     funcion para reconectarse al servidor de thingsboard y suscribirse a los topicos de RPC y Atributos
  */
  void reconnect() {
    int statusWifi = WL_IDLE_STATUS;
    // Loop until we're reconnected
    while (!client.connected()) {
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
     función para conectarse a wifi
  */
  void connectToWiFi()
  {
    if (WiFi.status() != WL_CONNECTED) {
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
