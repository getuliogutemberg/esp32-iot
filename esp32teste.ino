#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "esp_system.h"

#define DHTPIN 32         // Pino onde o sensor DHT está conectado
#define DHTTYPE DHT22     // Definindo o tipo de DHT (DHT22)
#define LDRPIN 33         // Pino onde o sensor LDR está conectado (pino analógico)
#define LEDPIN 2          // Pino onde o LED está conectado

const char* ssid = "CLARO_2G287EF5";          // Coloque o nome da sua rede Wi-Fi
const char* password = "AF287EF5";            // Coloque a senha da sua rede Wi-Fi
const char* mqtt_server = "test.mosquitto.org"; // Endereço do servidor MQTT

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(DHTPIN, DHTTYPE);

bool tempError = false;
bool humidityError = false;
bool lightError = false;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando ao Wi-Fi ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop até reconectar
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    // Tentando se conectar
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado");
      // Você pode se inscrever em tópicos aqui se necessário
      // client.subscribe("sua/assinatura");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
      Serial.println("Restarting now...");
      esp_restart();  // Reinicia a placa
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);

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
  delay(3000); // Espera 3 segundos com o LED apagado
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int lightLevel = analogRead(LDRPIN);

  tempError = false;
  humidityError = false;
  lightError = false;

  if (isnan(humidity)) {
    Serial.println("Falha ao ler do sensor DHT!: Umidade!");
    humidityError = true;
  }

  if (isnan(temperature)) {
    Serial.println("Falha ao ler do sensor DHT!: Temperatura!");
    tempError = true;
  }

  if (isnan(lightLevel)) {
    Serial.println("Falha ao ler do sensor de Iluminação!");
    lightError = true;
  }

  Serial.print("Umidade: ");
  Serial.print(humidity);
  Serial.print("%  Temperatura: ");
  Serial.print(temperature);
  Serial.print("°C  Luminosidade: ");
  Serial.println(lightLevel);

  // Cria uma string JSON para enviar
  String payload = "{\"temperatura\":" + String(temperature) 
                 + ", \"umidade\":" + String(humidity) 
                 + ", \"luz\":" + String(lightLevel) + "}";

  // Publica a mensagem no tópico "esp32/sensores"
  client.publish("esp32/sensores", payload.c_str());

  // Controle do LED
  int blinkCount = 0;

  if (tempError) {
    blinkCount += 3;
  }

  if (humidityError) {
    blinkCount += 2;
  }

  if (lightError) {
    blinkCount += 1;
  }

  if (blinkCount > 0) {
    blinkLED(blinkCount);  // LED piscando
  } else {
    digitalWrite(LEDPIN, HIGH);  // LED estático
    delay(10000); // Espera 10 segundos antes de enviar os dados novamente
  }
}
