/*
 Ejemplo del canal de Youtube "MikroTutoriales"

 https://www.youtube.com/c/Mikrotutoriales16/videos
 
 Utilizando el broker MQTT de mqtt.mikrodash.com
 y la plataforma de https://app.mikrodash.com
 para crear una dashboard personalizada
*/
#include <PubSubClient.h> // Biblioteca para comunicación MQTT

#if defined(ESP8266)
#include <ESP8266WiFi.h>  // Biblioteca WiFi para ESP8266
#elif defined(ESP32)
#include <WiFi.h>         // Biblioteca WiFi para ESP32
#endif

// Configuración de pines
const int button = 5;    // Pin para el botón
const int ledAzul = 22;  // Pin para el LED azul
const int asensorPin = 35; // Pin para el sensor analógico
const int pwmPin = 2;    // Pin para salida PWM

// Estado del botón
bool statusBtn = false;
bool candadoButton = false;

// Lectura del sensor y valor PWM
int sensorValue = 0;
int percentValue = 0;
int lastPercentValue = 0;
int pwmValue = 0;

// Tópicos MQTT (ajustar "your_mikrodash_id" con tu ID real) y el Topic correspondiente
const char* topic_sensor = "your_mikrodash_id/sensor/value";
const char* topic_button = "your_mikrodash_id/button/value";
const char* topic_pwm = "your_mikrodash_id/pwm/percent";
const char* topic_led = "your_mikrodash_id/led/value";

// Configuración WiFi
const char* ssid = "tuwifi";
const char* password = "tucontrasenia";

// Configuración servidor MQTT
const char* mqtt_server = "mqtt.mikrodash.com";
const int mqtt_port = 1883;
const char* usernameMQTT = "tuusername";
const char* passwordMQTT = "tucontraseniamqtt";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Conectado");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (strcmp(topic, topic_led) == 0) {
    int value = message.toInt();
    digitalWrite(ledAzul, value ? HIGH : LOW);
  } else if (strcmp(topic, topic_pwm) == 0) {
    pwmValue = map(message.toInt(), 0, 100, 0, 1023);
    analogWrite(pwmPin, pwmValue);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect("ESPClient", usernameMQTT, passwordMQTT)) {
      Serial.println("Conectado");
      client.subscribe(topic_led);
      client.subscribe(topic_pwm);
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" reintentando en 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(ledAzul, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Leer y publicar el estado del botón
  bool currentStatus = digitalRead(button);
  if (statusBtn != currentStatus) {
    statusBtn = currentStatus;
    client.publish(topic_button, statusBtn ? "1" : "0", true);
  }

  // Leer y publicar el valor del sensor analógico
  sensorValue = analogRead(asensorPin);
  percentValue = map(sensorValue, 0, 1023, 0, 100);
  if (percentValue != lastPercentValue) {
    lastPercentValue = percentValue;
    char msg[50];
    sprintf(msg, "%d", percentValue);
    client.publish(topic_sensor, msg, true);
  }

  delay(100); // Pequeño delay para estabilizar el loop
}
