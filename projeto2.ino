#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
 
// =============================================
// CONFIGURACOES
// =============================================
 
// Rede Wi-Fi
const char* ssid     = "";
const char* password = "";
 
// Broker HiveMQ Cloud
const char* mqtt_server = "b1bc381827e547a6b1caec499bd42756.s1.eu.hivemq.cloud";
const int   mqtt_port   = 8883;
const char* mqtt_user   = "Esp_Isabela";
const char* mqtt_pass   = "Senha123";
 
// Prefixo do topico — ID do ESP e adicionado automaticamente
// Topico final: sensores/ultrassonico/<ID>/distancia
const char* mqtt_topic_prefix = "sensores/ultrassonico";
 
// Pinos do sensor HC-SR04
#define TRIG_PIN 5
#define ECHO_PIN 18
 
// Intervalo entre leituras (milissegundos)
const unsigned long INTERVALO_LEITURA = 2000; // 2 segundos
 
// =============================================
// VARIAVEIS GLOBAIS (nao precisa mexer)
// =============================================
 
WiFiClientSecure espClient;
PubSubClient client(espClient);
 
unsigned long ultimaLeitura = 0;
 
char device_id[13];  // ex: "A4CF12AB34CD"
char mqtt_topic[64]; // ex: "sensores/ultrassonico/A4CF12AB34CD/distancia"
 
// =============================================
// GERAR ID UNICO A PARTIR DO MAC ADDRESS
// =============================================
void gerarDeviceID() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
 
  snprintf(device_id, sizeof(device_id),
           "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
 
  snprintf(mqtt_topic, sizeof(mqtt_topic),
           "%s/%s/distancia", mqtt_topic_prefix, device_id);
}
 
// =============================================
// CONECTAR AO WI-FI
// =============================================
void conectarWiFi() {
  Serial.print("Conectando ao Wi-Fi: ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);
 
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 40) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
 
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Wi-Fi conectado! IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
  } else {
    Serial.println();
    Serial.println("FALHA ao conectar no Wi-Fi! Verifique SSID e senha.");
  }
}
 
// =============================================
// CONECTAR AO BROKER MQTT (HiveMQ Cloud TLS)
// =============================================
void conectarMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao HiveMQ Cloud (ID: ");
    Serial.print(device_id);
    Serial.print(")...");
 
    char clientId[32];
    snprintf(clientId, sizeof(clientId), "ESP32-%s", device_id);
 
    if (client.connect(clientId, mqtt_user, mqtt_pass)) {
      Serial.println(" conectado!");
      Serial.print("Topico: ");
      Serial.println(mqtt_topic);
    } else {
      Serial.print(" falhou, rc=");
      Serial.print(client.state());
      Serial.println(" - tentando novamente em 3s");
      delay(3000);
    }
  }
}
 
// =============================================
// MEDIR DISTANCIA COM HC-SR04
// =============================================
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
 
  long duracao = pulseIn(ECHO_PIN, HIGH, 30000);
 
  if (duracao == 0) return -1;
 
  float distancia = duracao * 0.0343 / 2.0;
  return distancia;
}
 
// =============================================
// SETUP
// =============================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("=== ESP32 + HC-SR04 + HiveMQ Cloud ===");
 
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
 
  conectarWiFi();
 
  gerarDeviceID();
 
  Serial.print("Device ID: ");
  Serial.println(device_id);
 
  // Desativa verificacao do certificado (mais simples para uso em aula)
  espClient.setInsecure();
 
  client.setServer(mqtt_server, mqtt_port);
}
 
// =============================================
// LOOP
// =============================================
void loop() {
  if (!client.connected()) {
    conectarMQTT();
  }
  client.loop();
 
  unsigned long agora = millis();
  if (agora - ultimaLeitura >= INTERVALO_LEITURA) {
    ultimaLeitura = agora;
 
    float distancia = medirDistancia();
 
    if (distancia > 0 && distancia < 400) {
      char msg[10];
      dtostrf(distancia, 6, 2, msg);
 
      client.publish(mqtt_topic, msg);
 
      Serial.print("[");
      Serial.print(device_id);
      Serial.print("] Distancia: ");
      Serial.print(distancia);
      Serial.println(" cm  -> publicado no HiveMQ");
    } else {
      Serial.println("Leitura invalida do sensor");
    }
  }
}
