#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>  // Inclua a biblioteca ArduinoJson

#define DHTPIN 32
#define DHTTYPE DHT22
#define LDRPIN 33
#define LEDPIN 2

const char* mqtt_server = "test.mosquitto.org";

// Dados das credenciais Wi-Fi no formato JSON
const char* wifi_credentials_str = "[{\"CLARO_2G287EF5\":\"AF287EF5\"},{\"CLARO_5G287EF5\":\"AF287EF5\"},{\"Getulio\":\"100200300\"}]";

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(DHTPIN, DHTTYPE);

bool tempError = false;
bool humidityError = false;
bool lightError = false;

struct WiFiCredentials {
  String ssid;
  String password;
};

WiFiCredentials wifi_credentials[10];
int credentials_count = 0;

void setup_wifi() {
  DynamicJsonDocument doc(2048);

  // Processa o JSON
  DeserializationError error = deserializeJson(doc, wifi_credentials_str);
  if (error) {
    Serial.print("Falha ao processar JSON: ");
    Serial.println(error.c_str());
    return;
  }

  int index = 0;
  for (JsonObject obj : doc.as<JsonArray>()) {
    for (JsonPair kv : obj) {
      if (index < 10) {
        wifi_credentials[index].ssid = kv.key().c_str();
        wifi_credentials[index].password = kv.value().as<String>();
        index++;
      }
    }
  }

  credentials_count = index;

  bool connected = false;
  while (!connected) {
    for (int i = 0; i < credentials_count; i++) {
      Serial.print("Tentando conectar ao Wi-Fi ");
      Serial.println(wifi_credentials[i].ssid);

      WiFi.begin(wifi_credentials[i].ssid.c_str(), wifi_credentials[i].password.c_str());

      int retry_count = 0;
      while (WiFi.status() != WL_CONNECTED && retry_count < 20) {
        delay(500);
        Serial.print(".");
        retry_count++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi conectado: " + String(wifi_credentials[i].ssid.c_str()));
        Serial.println("Endereço IP: ");
        Serial.println(WiFi.localIP());
        connected = true;
        break;
      } else {
        Serial.println("Falha ao conectar");
      }
      // Se for o último índice e ainda não está conectado, reinicie i para 0
        if (i == credentials_count - 1) {
            i = -1; // -1 porque o próximo incremento de i será 0
        }

    }

   
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado");
    }
    else {
      setup_wifi();
    }
      // Inscrever-se em tópicos se necessário
      // client.subscribe("sua/assinatura");
    // } else {
    //   Serial.print("Falhou, rc=");
    //   Serial.print(client.state());
    //   Serial.println(" Tentando novamente em 5 segundos");
    //   delay(5000);
    // }
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
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // Exemplo de uso:
    float temperatura = NAN;
    float umidade = NAN;
    int luz = 1714;

    String message = generateJson(temperatura, umidade, luz);
    Serial.println(message);

    // Enviar a mensagem via MQTT
    client.publish("esp32/sensores", message.c_str());

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

  tempError = false;
  humidityError = false;
  lightError = false;

  if (isnan(humidity)) {
    Serial.println("Falha ao ler do sensor DHT!: Umidade!");
    humidityError = true;
    // humidity = 0;
  }

  if (isnan(temperature)) {
    Serial.println("Falha ao ler do sensor DHT!: Temperatura!");
    tempError = true;
    // temperature = 0;
  }

  if (isnan(lightLevel)) {
    Serial.println("Falha ao ler do sensor de Iluminação!");
    lightError = true;
    // lightLevel = 0;
  }

  Serial.print("Umidade: ");
  Serial.print(humidity);
  Serial.print("%  Temperatura: ");
  Serial.print(temperature);
  Serial.print("°C  Luminosidade: ");
  Serial.println(lightLevel);

  String payload = "{\"temperatura\":" + String(temperature) 
                 + ", \"umidade\":" + String(humidity) 
                 + ", \"luz\":" + String(lightLevel) + "}";

                 // Exemplo de uso:
  // float temperatura = NAN;
  // float umidade = NAN;
  // int luz = 1714;

  String message = generateJson(temperature, humidity, lightLevel);
  // Serial.println(message);

  // Enviar a mensagem via MQTT
  client.publish("esp32/sensores", message.c_str());

  // client.publish("esp32/sensores", payload.c_str());

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
    blinkLED(blinkCount);
  } else {
    digitalWrite(LEDPIN, HIGH);
    delay(10000);
  }
}
