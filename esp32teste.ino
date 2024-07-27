#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define DHTPIN 32
#define DHTTYPE DHT11
#define LDRPIN 33
#define LEDPIN 2

const char* default_ssid = "CLARO_2G287EF5";
const char* default_password = "AF287EF5";

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(DHTPIN, DHTTYPE);

struct WiFiCredentials {
  String ssid;
  String password;
};

WiFiCredentials wifi_credentials[10];
int credentials_count = 0;
String mqtt_server = "test.mosquitto.org";
int mqtt_port = 1883;
String mqtt_client = "ESP32Client";
String mqtt_topic = "esp32/sensores";
String esp_id = "ESP007";

void setup_wifi(const char* ssid, const char* password) {
  delay(10);
  WiFi.begin(ssid, password);

  int retry_count = 0;
  while (WiFi.status() != WL_CONNECTED && retry_count < 20) {
    delay(500);
    Serial.print(".");
    retry_count++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi conectado: " + String(ssid));
    Serial.println("Endereço IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Falha ao conectar");
  }
}

void fetchSetupData() {
  HTTPClient http;
  http.begin("https://esp32-data-api-1.onrender.com/setup");
  int httpCode = http.GET();

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("Resposta JSON recebida:");
      Serial.println(payload);

      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        // Atualiza as credenciais Wi-Fi
        credentials_count = 0;
        for (JsonObject obj : doc["wifi_credentials_str"].as<JsonArray>()) {
          for (JsonPair kv : obj) {
            if (credentials_count < 10) {
              wifi_credentials[credentials_count].ssid = kv.key().c_str();
              wifi_credentials[credentials_count].password = kv.value().as<String>();
              credentials_count++;
            }
          }
        }

        mqtt_server = doc["mqtt_server"].as<String>();
        mqtt_port = doc["mqtt_port"].as<int>();
        mqtt_client = doc["mqtt_client"].as<String>();
        mqtt_topic = doc["mqtt_topic"].as<String>();
        esp_id = doc["id"].as<String>();

        Serial.println("Configurações atualizadas:");
        Serial.print("MQTT Server: ");
        Serial.println(mqtt_server);
        Serial.print("MQTT Port: ");
        Serial.println(mqtt_port);
        Serial.print("MQTT Client: ");
        Serial.println(mqtt_client);
        Serial.print("MQTT Topic: ");
        Serial.println(mqtt_topic);
        Serial.print("ESP ID: ");
        Serial.println(esp_id);
      } else {
        Serial.println("Falha ao processar JSON de configuração");
      }
    }
  } else {
    Serial.println("Falha ao buscar configuração");
  }

  http.end();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    if (client.connect(mqtt_client.c_str())) {
      Serial.println("Conectado");
      client.subscribe(mqtt_topic.c_str());
    } else {
      setup_wifi(default_ssid, default_password);
      delay(5000);
    }
  }
}

String generateJson(float temperatura, float umidade, int luz) {
  String json = "{";

  json += "\"temperatura\":";
  if (isnan(temperatura)) {
    json += "null";
  } else {
    json += String(temperatura);
  }

  json += ", \"umidade\":";
  if (isnan(umidade)) {
    json += "null";
  } else {
    json += String(umidade);
  }

  json += ", \"luz\":";
  json += String(luz);

  json += "}";
  
  return json;
}

void setup() {
  Serial.begin(115200);
  setup_wifi(default_ssid, default_password);
  fetchSetupData();
  client.setServer(mqtt_server.c_str(), mqtt_port);
  pinMode(LDRPIN, INPUT);
  pinMode(LEDPIN, OUTPUT);
  dht.begin();
}

void blinkLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LEDPIN, HIGH);
    delay(500);
    digitalWrite(LEDPIN, LOW);
    delay(500);
  }
  delay(3000);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int lightLevel = analogRead(LDRPIN);

  if (isnan(humidity) || isnan(temperature) || isnan(lightLevel)) {
    Serial.println("Falha ao ler do sensor");
  } else {
    Serial.print("Umidade: ");
    Serial.print(humidity);
    Serial.print("%  Temperatura: ");
    Serial.print(temperature);
    Serial.print("°C  Luminosidade: ");
    Serial.println(lightLevel);

    String payload = generateJson(temperature, humidity, lightLevel);
    client.publish(mqtt_topic.c_str(), payload.c_str());
  }

  delay(10000);
}