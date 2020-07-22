    #include <PubSubClient.h>
    #include <ESP8266WiFi.h>  
    
    const char *ssid =  "Nombre de la Red";
    const char *pass =  "Contraseña"; 
    WiFiClient client;
    PubSubClient pubsub_client(client);
    void publicar(int hayPersona, int peopleCount,int totalCount);
    
    // definir Pines
    const int trigPin1 = 5;
    const int echoPin1 = 4;
    const int trigPin2 = 0;
    const int echoPin2 = 14;
    const int boton = 12;
    
    // definir variables
      //Sesnsores
      long duration1;
      int distance1;
      long duration2;
      int distance2;

      //Contadores
      int contador = -1;
      int totalCount = -1;
      int hayPersona = 0;

      //Logica
      int estadoBoton = HIGH;    
      int estado = 0;
    void setup() {
      
      pinMode(boton, INPUT);
      pinMode(trigPin2, OUTPUT); 
      pinMode(echoPin2, INPUT); 
      pinMode(trigPin1, OUTPUT); 
      pinMode(echoPin1, INPUT); 
      Serial.begin(9600); 
      
      digitalWrite(trigPin2, LOW);
      digitalWrite(trigPin1, LOW);
      
      //Conexion a Wifi
      Serial.println("Connecting to ");
      Serial.println(ssid); 
      WiFi.begin(ssid, pass); 
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("");
      Serial.println("WiFi connected"); 
      
      //Conexion a Thinsgboard
      pubsub_client.setServer("demo.thingsboard.io", 1883); // This is default if you are using thingsboard

      while (!pubsub_client.connect ("c879e6c0-b745-11ea-b4ad-47e5929eed78","qUZBqPqW1xo5owHG32MC", NULL)) {
            delay(500);
            Serial.print(".");
      }
      Serial.println("");
      Serial.println("Thingsboard connected");   
    }
    void loop() {

      estadoBoton=digitalRead(boton);
      delay(15);
      //Logica de Entrada/Salida
      if(distance1 < 50){
        if(estado == 0){
          estado = 1;
        } else if(estado == 2){
          contador = contador - 1;
          if(contador == 0){
            hayPersona = 0;
            publicar(hayPersona, contador, totalCount);  
          }
          estado = 0;
          publicar(hayPersona, contador, totalCount);
          }
        }
        if(distance2 < 50){ 
          if(estado == 0){
            estado = 2;
          }else if(estado == 1){
            totalCount++;
            contador = contador + 1;
            estado = 0;
            if(contador == 1){
              hayPersona = 1;
              publicar(hayPersona, contador, totalCount);
            }
            publicar(hayPersona, contador, totalCount);
          }
        }
        
      //Boton de Reseteo
      if (estadoBoton==LOW){
        contador =0;
        estado=0;    
        hayPersona = 0;
        publicar(hayPersona, contador, totalCount);  
      }

      //Muestra por Pantalla de los datos
      Serial.print("Gente: ");
      Serial.println(contador); 
      Serial.print("Estado: ");
      Serial.println(estado);
      Serial.print("Gente Total: ");
      Serial.println(totalCount);


      //Sensado
      
      delay(500);  
      //Primer sensor
      digitalWrite(trigPin2, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin2, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin2, LOW);
      duration2 = pulseIn(echoPin2, HIGH);
      distance2= duration2/59;
     
      //Segundo Sensor
      digitalWrite(trigPin1, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin1, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin1, LOW);
      duration1 = pulseIn(echoPin1, HIGH);
      distance1= duration1/59;
     
    }

    //Funcion Para subir a Thngsboard los datos
    void publicar(int hayPersona, int peopleCount,int totalCount){
         String payload = "{";
         payload += "\"Hay_personas\":"; payload += hayPersona; payload += ",";
         payload += "\"peopleCount\":"; payload += peopleCount; payload += ",";
         payload += "\"totalCount\":"; payload += totalCount;
         payload += "}";
        Serial.println(payload);
        if(pubsub_client.publish("v1/devices/me/telemetry",payload.c_str())){
          Serial.println("Published");  
        }    
    }
