#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

// ===== CONFIGURACIÓN =====
const char* SSID = "GaiaElCaballero";
const char* PASSWORD = "Gaialord40@";

// IP del servidor Spring Boot (cámbiala si cambia tu IP)
const char* SERVER_HOST = "10.33.139.73";
const int SERVER_PORT = 8080;

#define VEHICULO_ID 3  // Cambia según tu vehículo

// Pines LEDs (simulan dirección)
#define LED_IZQUIERDA 18
#define LED_DERECHA 19

// ===== WebSocket =====
WebSocketsClient webSocket;
bool wsConectado = false;

// ===== Heartbeat =====
unsigned long lastHeartbeat = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_IZQUIERDA, OUTPUT);
  pinMode(LED_DERECHA, OUTPUT);
  ledsOff();

  WiFi.begin(SSID, PASSWORD);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado! IP: " + WiFi.localIP().toString());
  Serial.println("Vehículo ID fijo: " + String(VEHICULO_ID));

  // WebSocket - con host, puerto y ruta separados
  webSocket.begin(SERVER_HOST, SERVER_PORT, "/ws/comandos");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop() {
  webSocket.loop();

  if (millis() - lastHeartbeat > 2000) {
    lastHeartbeat = millis();
    enviarHeartbeat();
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi perdido, reconectando...");
    WiFi.reconnect();
  }
}

// ===== EVENTO WEBSOCKET =====
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: {
      wsConectado = false;
      Serial.println("❌ WebSocket desconectado");
      break;
    }

    case WStype_CONNECTED: {
      wsConectado = true;
      Serial.println("✅ WebSocket conectado");
      String identMsg = "{\"tipo\":\"identificar\",\"vehiculoId\":" + String(VEHICULO_ID) + "}";
      webSocket.sendTXT(identMsg);
      Serial.println("📤 Identificando: " + identMsg);
      break;
    }

    case WStype_TEXT: {
      String mensaje = String((char*)payload);
      Serial.println("📩 Comando WebSocket: " + mensaje);

      DynamicJsonDocument doc(256);
      DeserializationError error = deserializeJson(doc, mensaje);
      if (error) {
        Serial.println("❌ Error parseando JSON");
        return;
      }

      String tipo = doc["tipo"].as<String>();
      String valor = doc["valor"].as<String>();

      // ✅ AHORA ACEPTA "direccion" Y "comando"
      if (tipo == "direccion" || tipo == "comando") {
        if (valor == "izquierda") motorIzquierda();
        else if (valor == "derecha") motorDerecha();
        else if (valor == "stop") motorStop();
        else if (valor == "adelante") motorAdelante();
        else if (valor == "atras") motorAtras();
      } else if (tipo == "velocidad") {
        Serial.println("⚡ Velocidad: " + valor);
      }
      break;
    }

    default:
      break;
  }
}

// ===== HEARTBEAT HTTP =====
void enviarHeartbeat() {
  HTTPClient http;
  String url = "http://" + String(SERVER_HOST) + ":" + String(SERVER_PORT) + "/vehiculos/" + String(VEHICULO_ID);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  String json = "{\"estado\":\"CONECTADO\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
  int code = http.PUT(json);
  if (code == 200) Serial.println("💓 Heartbeat OK");
  else Serial.printf("❌ Heartbeat error: %d\n", code);
  http.end();
}

// ===== FUNCIONES LEDs =====
void motorIzquierda() {
  digitalWrite(LED_IZQUIERDA, HIGH);
  digitalWrite(LED_DERECHA, LOW);
  Serial.println("LED: IZQUIERDA");
  delay(500);  // 500 ms visible
  digitalWrite(LED_IZQUIERDA, LOW);
  digitalWrite(LED_DERECHA, LOW);
}

void motorDerecha() {
  digitalWrite(LED_IZQUIERDA, LOW);
  digitalWrite(LED_DERECHA, HIGH);
  Serial.println("LED: DERECHA");
  delay(500);
  digitalWrite(LED_IZQUIERDA, LOW);
  digitalWrite(LED_DERECHA, LOW);
}

void motorAdelante() {
  digitalWrite(LED_IZQUIERDA, HIGH);
  digitalWrite(LED_DERECHA, HIGH);
  Serial.println("LED: ADELANTE (ambos encendidos)");
  delay(500);
  digitalWrite(LED_IZQUIERDA, LOW);
  digitalWrite(LED_DERECHA, LOW);
}

void motorAtras() {
  digitalWrite(LED_IZQUIERDA, LOW);
  digitalWrite(LED_DERECHA, LOW);
  Serial.println("LED: ATRÁS (apagados)");
}

void motorStop() {
  digitalWrite(LED_IZQUIERDA, LOW);
  digitalWrite(LED_DERECHA, LOW);
  Serial.println("LED: STOP");
}

void ledsOff() {
  digitalWrite(LED_IZQUIERDA, LOW);
  digitalWrite(LED_DERECHA, LOW);
}