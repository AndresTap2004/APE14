#include <WiFi.h>
#include <PubSubClient.h>

const char* WIFI_SSID = "Internet_UNL";        
const char* WIFI_PASSWORD = "UNL1859WiFi";   

const char* MQTT_SERVER = "10.20.136.247";
const int MQTT_PORT = 1883;

const int PIN_LM35 = 34;
const int PIN_LED = 2;

WiFiClient espClient;
PubSubClient client(espClient);

SemaphoreHandle_t xLedSemaphore;
SemaphoreHandle_t xMqttMutex;

String ledState = "OFF";

void callback(char* topic, byte* payload, unsigned int length) {
  String mensaje = "";

  for (int i = 0; i < length; i++) {
    mensaje += (char)payload[i];
  }

  Serial.print("[MQTT] Mensaje recibido en ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(mensaje);

  if (String(topic) == "esp32/led") {
    if (mensaje == "ON" || mensaje == "OFF") {
      ledState = mensaje;
      xSemaphoreGive(xLedSemaphore);
      Serial.println("[MQTT] Estado del LED actualizado via Semaforo.");
    }
  }
}

void conectarWiFi() {
  Serial.println("Conectando WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.print("IP ESP32: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {
  while (!client.connected()) {
    Serial.println("Conectando MQTT...");

    if (client.connect("ESP32_FreeRTOS_MQTT")) {
      Serial.println("[MQTT] Conectado y suscrito.");
      client.subscribe("esp32/led");
    } else {
      Serial.print("Error MQTT: ");
      Serial.println(client.state());
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
  }
}

float leerTemperaturaLM35() {
  int lectura = analogRead(PIN_LM35);
  float voltaje = lectura * (3.3 / 4095.0);
  float temperatura = voltaje * 100.0;

  Serial.print("[ADC] Lectura: ");
  Serial.print(lectura);
  Serial.print(" | Voltaje: ");
  Serial.println(voltaje);

  return temperatura;
}

void vTaskMQTT(void *pvParameters) {
  while (true) {
    if (xSemaphoreTake(xMqttMutex, portMAX_DELAY)) {
      if (!client.connected()) {
        conectarMQTT();
      }

      client.loop();

      xSemaphoreGive(xMqttMutex);
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void vTaskSensor(void *pvParameters) {
  while (true) {
    float temperatura = leerTemperaturaLM35();

    char tempString[10];
    dtostrf(temperatura, 4, 2, tempString);

    if (xSemaphoreTake(xMqttMutex, portMAX_DELAY)) {
      if (client.connected()) {
        client.publish("esp32/temperatura", tempString);

        Serial.print("[SENSOR] Temperatura publicada: ");
        Serial.println(tempString);
      }

      xSemaphoreGive(xMqttMutex);
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void vTaskLed(void *pvParameters) {
  while (true) {
    if (xSemaphoreTake(xLedSemaphore, portMAX_DELAY)) {
      if (ledState == "ON") {
        digitalWrite(PIN_LED, HIGH);
        Serial.println("[LED] Encendido");
      } else if (ledState == "OFF") {
        digitalWrite(PIN_LED, LOW);
        Serial.println("[LED] Apagado");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_LM35, INPUT);

  conectarWiFi();

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  xLedSemaphore = xSemaphoreCreateBinary();
  xMqttMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
    vTaskMQTT,
    "Tarea MQTT",
    4096,
    NULL,
    2,
    NULL,
    1
  );

  xTaskCreatePinnedToCore(
    vTaskSensor,
    "Tarea Sensor",
    4096,
    NULL,
    1,
    NULL,
    1
  );

  xTaskCreatePinnedToCore(
    vTaskLed,
    "Tarea LED",
    2048,
    NULL,
    1,
    NULL,
    1
  );

  Serial.println("[FreeRTOS] Tareas inicializadas en el nucleo 1.");
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}