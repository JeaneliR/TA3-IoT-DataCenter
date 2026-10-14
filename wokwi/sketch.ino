#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>

#define DHT_PIN 15
#define LDR_PIN 34

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

const char* MQTT_SERVER = "test.mosquitto.org";
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC =
    "jeaneli-u22219824/datacenter/sala1/lecturas";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
DHTesp dhtSensor;

unsigned long ultimaPublicacion = 0;
const unsigned long INTERVALO = 5000;

void conectarWiFi() {
  Serial.print("Conectando a Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi conectado");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando a MQTT... ");

    String clientId = "esp32-jeaneli-";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("conectado");
    } else {
      Serial.print("error, código=");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

String clasificarTemperatura(float temperatura) {
  if (temperatura < 28.0) {
    return "normal";
  }

  if (temperatura < 30.0) {
    return "advertencia";
  }

  return "critica";
}

int calcularIluminacion(int lecturaAnalogica) {
  int porcentaje = map(lecturaAnalogica, 4095, 0, 0, 100);
  return constrain(porcentaje, 0, 100);
}

void publicarLectura() {
  TempAndHumidity datos = dhtSensor.getTempAndHumidity();

  float temperatura = datos.temperature;
  int lecturaLDR = analogRead(LDR_PIN);
  int iluminacion = calcularIluminacion(lecturaLDR);

  if (isnan(temperatura)) {
    Serial.println("Error leyendo el DHT22");
    return;
  }

  String estado = clasificarTemperatura(temperatura);
  char mensaje[220];

  snprintf(
    mensaje,
    sizeof(mensaje),
    "{\"temperatura\":%.1f,"
    "\"iluminacion\":%d,"
    "\"dispositivo\":\"esp32-sala1\","
    "\"estado_temperatura\":\"%s\"}",
    temperatura,
    iluminacion,
    estado.c_str()
  );

  bool publicado = mqttClient.publish(MQTT_TOPIC, mensaje);

  Serial.println("--------------------------------");
  Serial.print("Temperatura: ");
  Serial.print(temperatura, 1);
  Serial.println(" °C");

  Serial.print("Iluminación: ");
  Serial.print(iluminacion);
  Serial.println(" %");

  Serial.print("Estado: ");
  Serial.println(estado);

  Serial.print("Mensaje MQTT: ");
  Serial.println(mensaje);

  Serial.print("Publicado: ");
  Serial.println(publicado ? "Sí" : "No");
}

void setup() {
  Serial.begin(115200);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  conectarWiFi();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  randomSeed(micros());

  Serial.println("Sistema IoT del centro de datos iniciado");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();

  if (millis() - ultimaPublicacion >= INTERVALO) {
    ultimaPublicacion = millis();
    publicarLectura();
  }
}