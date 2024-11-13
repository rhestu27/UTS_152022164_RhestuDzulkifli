#include <WiFi.h>
#include "PubSubClient.h"
#include "DHTesp.h"

// Membuat objek WiFi dan MQTT client
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Informasi jaringan WiFi
const char* ssid = "Wokwi-GUEST"; // Nama WiFi (SSID)
const char* password = ""; // Kata sandi WiFi

// Informasi broker MQTT
char* mqttServer = "broker.hivemq.com"; // Alamat server MQTT
int mqttPort = 1883; // Port MQTT

// Pin untuk sensor DHT dan relay
const int DHT_PIN = 32; // Pin DHT
const int relayPin = 13; // Pin relay

// Pin untuk LED dan buzzer
#define LED_RED 18     // Pin LED merah
#define LED_YELLOW 5   // Pin LED kuning
#define LED_GREEN 17   // Pin LED hijau
#define BUZZER 8       // Pin buzzer

// Membuat objek untuk sensor DHT
DHTesp dhtSensor;

// Fungsi untuk menghubungkan kembali ke broker MQTT jika terputus
void reconnect() {
  Serial.println("Connecting to MQTT Broker...");
  while (!mqttClient.connected()) {
    Serial.println("Reconnecting to MQTT Broker..");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Connected.");
    }
  }
}

// Fungsi untuk mengatur server MQTT
void setupMQTT() {
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
}

// Fungsi callback untuk menerima pesan dari MQTT
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Callback - Message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
  }
  Serial.println();
}

// Fungsi setup yang dijalankan sekali saat memulai
void setup() {
  Serial.begin(115200);
  Serial.println("Hello, ESP32!");
  
  // Inisialisasi sensor DHT
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  
  // Mengatur pin untuk relay, LED, dan buzzer
  pinMode(relayPin, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Menghubungkan ke WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected to Wi-Fi");

  // Mengatur koneksi MQTT
  setupMQTT();
}

// Fungsi loop yang dijalankan berulang kali
void loop() {
  // Memeriksa dan menghubungkan kembali ke MQTT jika terputus
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  // Membaca suhu dan kelembaban dari sensor DHT
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  float temperature = data.temperature;
  float humidity = data.humidity;

  // Variabel untuk mengontrol LED dan buzzer
  bool isRedActive = false;
  bool isYellowActive = false;
  bool isGreenActive = false;
  bool isBuzzerActive = false;

  // Kontrol LED dan buzzer berdasarkan suhu
  if (temperature > 35) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(BUZZER, HIGH);
    isRedActive = true;
    isBuzzerActive = true;
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
  } else if (temperature >= 30 && temperature <= 35) {
    digitalWrite(LED_YELLOW, HIGH);
    isYellowActive = true;
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(BUZZER, LOW);
  } else {
    digitalWrite(LED_GREEN, HIGH);
    isGreenActive = true;
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(BUZZER, LOW);
  }

  // Kontrol relay berdasarkan suhu
  if (temperature >= 35) {
    digitalWrite(relayPin, HIGH);  // Mengaktifkan relay
  } else {
    digitalWrite(relayPin, LOW);   // Mematikan relay
  }

  // Mengirim data suhu, kelembaban, dan status ke MQTT
  char tempString[8];
  char humString[8];
  dtostrf(temperature, 1, 2, tempString); // Konversi suhu ke string
  dtostrf(humidity, 1, 2, humString);     // Konversi kelembaban ke string

  mqttClient.publish("monitoring/temperature", tempString);
  mqttClient.publish("monitoring/humidity", humString);

  // Mengirim status LED dan buzzer
  String statusMessage = String("Red LED: ") + (isRedActive ? "ON" : "OFF") +
                         ", Yellow LED: " + (isYellowActive ? "ON" : "OFF") +
                         ", Green LED: " + (isGreenActive ? "ON" : "OFF") +
                         ", Buzzer: " + (isBuzzerActive ? "ON" : "OFF") +
                         ", Relay: " + ((temperature >= 35) ? "ON" : "OFF");
  mqttClient.publish("monitoring/status", statusMessage.c_str());

  delay(2000);  // Jeda antara setiap loop
}
