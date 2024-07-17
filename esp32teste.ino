#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 32         // Pino onde o sensor DHT está conectado
#define DHTTYPE DHT22     // Definindo o tipo de DHT (DHT22)
#define LDRPIN 33         // Pino onde o sensor LDR está conectado (pino analógico)

const char* ssid = "CLARO_2G287EF5";          // Coloque o nome da sua rede Wi-Fi
const char* password = "AF287EF5";            // Coloque a senha da sua rede Wi-Fi
const char* serverName = "https://esp32-data-api-1.onrender.com/data"; // URL da sua API

DHT dht(DHTPIN, DHTTYPE);
HTTPClient http;

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi...");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("Conectado ao Wi-Fi!");
}

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  
  pinMode(LDRPIN, INPUT);
  // Inicializa o sensor DHT
  dht.begin();
}

void loop() {
  // Lê a umidade e a temperatura do DHT22
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  // Lê a luminosidade do LDR
  int lightLevel = analogRead(LDRPIN);

  // Verifica se as leituras são válidas
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Falha ao ler do sensor DHT!");
    delay(1000);
    return;
  }

  // Printar dados no monitor serial
  Serial.print("Umidade: ");
  Serial.print(humidity);
  Serial.print("%  Temperatura: ");
  Serial.print(temperature);
  Serial.print("°C  Luminosidade: ");
  Serial.println(lightLevel);

    http.begin(serverName); // Especifica o endereço da API
    http.addHeader("Content-Type", "application/json"); // Tipo de conteúdo
    String httpRequestData = "{\"temperatura\": " + String(temperature) 
                          + ", \"umidade\": " + String(humidity) 
                          + ", \"luz\": " + String(lightLevel) + "}";
    int httpResponseCode = http.POST(httpRequestData);
    String response = http.getString();
    if (httpResponseCode) {
      Serial.println(httpResponseCode);
      Serial.print("Dados exportados: ");
      Serial.println(response);         // Resposta do servidor
    } else {
      Serial.print("Erro na requisição HTTP: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  
  // Espera 10 segundos antes de enviar os dados novamente
  delay(10000);
}

