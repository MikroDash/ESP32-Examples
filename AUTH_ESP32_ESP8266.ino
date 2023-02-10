/*
 Ejemplo del canal de Youtube "MikroTutoriales"

 https://www.youtube.com/c/Mikrotutoriales16/videos
 
 Utilizando el broker MQTT de mqtt.mikrodash.com
 y la plataforma de https://app.mikrodash.com
 para crear una dashboard personalizada
*/
#include <PubSubClient.h> //https://github.com/knolleary/pubsubclient
#include <Arduino_JSON.h> //https://github.com/arduino-libraries/Arduino_JSON

#if defined(ESP8266)
#include <ESP8266WiFi.h>  //https://github.com/esp8266/Arduino
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "El programa solo funciona para los modulos ESP32 y ESP8266"
#endif

/**
*  Declaracion de entradas y salidas dependiendo que ESP se seleccione
**/
#if defined(ESP8266)
// Entradas y salidas Digitales
const int button = 5; //Entrada digital
const int ledAzul = 16;    //Salida digital
// Entradas y salidas Analogica
const int asensorPin = A0; //Entrada analogica. Voltaje de entre 0 - 3.2 V
const int pwmPin = 2;       //Salida analogica por PWM 
#elif defined(ESP32)
// Entradas y salidas Digitales
const int button = 5; //Entrada digital
const int ledAzul = 22;    //Salida digital
// Entradas y salidas Analogica
const int asensorPin = 35; //Entrada analogica. Voltaje de entre 0 - 3.2 V
const int pwmPin = 2;       //Salida analogica por PWM 
#endif


//Variables para convertir los datos de entrada digital.
bool statusBtn = false;
bool candadoButton = false; 

//Variables para convertir los datos de entrada analogica.
int sensorValue = 0;
int percentValue = 0;
int lastPercentValue = 0;

//Variables para convertir los datos de salida analogica.
int pwmValue = 0;

//Topics MQTT
const char* topic_sensor = "tutopic";
const char* topic_button = "tutopic";
const char* topic_pwm = "tutopic";
const char* topic_led = "tutopic";

// Constantes de configuracion
const char* ssid = "tuwifi"; // Nombre de la Red Wi-Fi
const char* password = "tucontrasenia"; // Contraseña de la Red Wi-Fi

//Parametros de conexion MQTT con MikroDash
const char* mqtt_server = "mqtt.mikrodash.com"; //Servidor MQTT
const int mqtt_port = 1883;                     //Puerto MQTT
const int qos_level=1;                          //Calidad del servicio MQTT

//Credenciales MQTT MikroDash
const char* usernameMQTT = "tuusername";
const char* passwordMQTT = "tucontraseniamqtt";

//Variables para la conexión y envio de mensajes MQTT
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (115)
char msg[MSG_BUFFER_SIZE];
int value = 0;


//Conexion Wi-Fi
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi Conectado");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
}

//Callback para la recepcion de datos en los topic suscrito
void callback(char* topic, byte* payload, unsigned int length) {
  String myString = ""; 
  for (int i = 0; i < length; i++) {
    myString = myString + (char)payload[i]; //Se convierte el payload de tipo bytes a una cadena de texto (String)
  }
  JSONVar myObject = JSON.parse(myString); // Se convierte la respuesta a tipo Object para acceder a los valores por el nombre.
  if (JSON.typeof(myObject) == "undefined") { // Si el archivo no tiene información o no es JSON se sale del callback
    Serial.println("[JSON] JSON NO VALIDO"); 
    return;
  }else{                                      //Si el archivo es JSON y contiene info entra aqui
    const char* from = myObject["from"];
    Serial.println(myObject["value"]);
    if(strcmp(topic,topic_led)==0 && strcmp(from,"app")==0 ){  //Si el topic es topic_led y lo envia app.mikrodash.com
      if((bool) myObject["value"]){  //Si value es true       
       digitalWrite(ledAzul, HIGH);  //Enciende el LED
       client.publish(topic_led, "{\"from\":\"device\",\"message\":\"Hola desde ESP8266\",\"value\": true}", true); //Actualiza el estado del LED en el topic y lo retiene
      }else{
        digitalWrite(ledAzul, LOW); //Apaga el led
        client.publish(topic_led, "{\"from\":\"device\",\"message\":\"Hola desde ESP8266\",\"value\": false}", true); //Actualiza el estado del LED en el topic y lo retiene
      }
    }else if(strcmp(topic,topic_pwm)==0 && strcmp(from,"app")==0){ //Si el topic es topic_pwm y lo envia app.mikrodash.com

      const bool isActive = (bool) myObject["value"]["on"];   //Se asigna el valor de "Value" en una constante de tipo entero
      const int pwmValue = (int) myObject["value"]["percent"];   //Se asigna el valor de "Value" en una constante de tipo entero

      if(!isActive){
        analogWrite(pwmPin, 0);
        snprintf (msg, MSG_BUFFER_SIZE, "{\"from\":\"device\",\"message\":\"Hola desde ESP8266\",\"value\": { \"on\": false, \"percent\": %ld} }", pwmValue);  //convierte los parametros en un array de bytes
        client.publish(topic_pwm, msg, true); //Se actualiza el valor actual de la salida PWM
        return;
      }
      if(pwmValue >= 0 && pwmValue <= 100){
        const int setValue = (pwmValue * 1024) / 100; //se convierte de un porcentaje a un valor asignable al PWM con una simple regla de 3
        snprintf (msg, MSG_BUFFER_SIZE, "{\"from\":\"device\",\"message\":\"Hola desde ESP8266\",\"value\": { \"on\": true, \"percent\": %ld} }", pwmValue);  //convierte los parametros en un array de bytes
        client.publish(topic_pwm, msg, true); //Se actualiza el valor actual de la salida PWM
        analogWrite(pwmPin, setValue); 
      }
    }
  }

}

// Funcion para reconectar
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando al broker MQTT...");
    // Se genera un ID aleatorio con
    String clientId = "MikroDashWiFiClient-";
    clientId += String(random(0xffff), HEX);

    //Cuando se conecta
    if (client.connect(clientId.c_str(), usernameMQTT, passwordMQTT) ) {
      Serial.println("Conectado");

      //Topics a los que se suscribira para recibir cambios
      client.subscribe(topic_pwm,qos_level);
      client.subscribe(topic_led,qos_level);
    } else {
      Serial.print("Error al conectar: ");
      Serial.print(client.state());
      Serial.println(" Reintentando en 5 segundos...");
      delay(5000);
    }
  }
}

//Funcion principal para configura el modulo
void setup() {
  pinMode(ledAzul, OUTPUT);  //Se declara el pin como salida
  Serial.begin(115200);      //Comunicación serial para monitorear
  setup_wifi();               //Se llama la funcición para conectarse al Wi-Fi
  client.setServer(mqtt_server, mqtt_port);  //Se indica el servidor y puerto de MQTT para conectarse
  client.setCallback(callback);  //Se establece el callback, donde se reciben los cambios de las suscripciones MQTT
}

//Bucle infinito
void loop() {

  if (!client.connected()) { //Si se detecta que no hay conexión con el broker 
    reconnect();              //Reintenta la conexión
  }
  client.loop();            //De lo contrario crea un bucle para las suscripciones MQTT

  statusBtn = digitalRead(button); //Se lee el estado del boton o entrada digital
  if(candadoButton != statusBtn){  //Condiciones para entrar solo en cada cambio de estado del boton
    candadoButton = statusBtn;
    if(candadoButton){
      client.publish(topic_button, "{\"from\":\"device\",\"message\":\"Hola desde ESP8266\",\"value\": true}", true); //Se actualiza el nuevo estado en el topic
    }else{
      client.publish(topic_button, "{\"from\":\"device\",\"message\":\"Hola desde ESP8266\",\"value\": false}", true); //Se actualiza el nuevo estado en el topic
    }
  }
  
  sensorValue = analogRead(asensorPin); //Se lee la entrada analógica
  #if defined(ESP8266)    
    percentValue = (sensorValue * 100) / 1024;  //Se convierte de un resolucion de 0 a 1024 a un porcentaje de 0 a 100 con una regla de 3  
  #elif defined(ESP32)
    percentValue = (sensorValue * 100) / 4095;  //Se convierte de un resolucion de 0 a 1024 a un porcentaje de 0 a 100 con una regla de 3  
  #endif
  
  if(lastPercentValue != percentValue){  //Solo si hay un cambio en el sensor entra
    lastPercentValue = percentValue;    
    snprintf (msg, MSG_BUFFER_SIZE, "{\"from\":\"device\",\"message\":\"Hola desde ESP8266\",\"value\":%ld}", lastPercentValue); //Se convierte a array bits y se concatena el porcentaje del sensor
    client.publish(topic_sensor, msg, true); //Se actualiza el nuevo estado en el topic
  }
  delay(100); //Un delay de 100ms para darle un respiro al modulo de wifi ESP8266
}