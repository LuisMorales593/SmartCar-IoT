#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

// ===== CONFIGURACIÓN =====
const char* SSID = "GaiaElCaballero";
const char* PASSWORD = "Gaialord40@";
const char* SERVER_HOST = "10.152.135.73";
const int SERVER_PORT = 8080;
#define VEHICULO_ID 3

// ===== PINES =====
#define DIR_IN1 18
#define DIR_IN2 19
#define TRAC_IN1 21
#define TRAC_IN2 22
#define TRAC_ENA 4
#define LUZ_DEL_ROJO  23
#define LUZ_DEL_AMAR  25
#define LUZ_TRAS_ROJO 26
#define LUZ_TRAS_AMAR 27
#define LED_STRIP 5
#define DF_RX 16
#define DF_TX 17

// ===== VARIABLES =====
int velocidadActual = 0;
String direccionActual = "stop";
bool lucesEncendidas = false;

WebSocketsClient webSocket;
bool wsConectado = false;
unsigned long lastHeartbeat = 0;

void setup() {
  Serial.begin(115200);

  pinMode(DIR_IN1, OUTPUT);
  pinMode(DIR_IN2, OUTPUT);
  pinMode(TRAC_IN1, OUTPUT);
  pinMode(TRAC_IN2, OUTPUT);
  pinMode(TRAC_ENA, OUTPUT);
  pinMode(LUZ_DEL_ROJO, OUTPUT);
  pinMode(LUZ_DEL_AMAR, OUTPUT);
  pinMode(LUZ_TRAS_ROJO, OUTPUT);
  pinMode(LUZ_TRAS_AMAR, OUTPUT);

  ledcAttach(TRAC_ENA, 5000, 8);

  resetTodo();
  lucesOff();

  WiFi.begin(SSID, PASSWORD);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado! IP: " + WiFi.localIP().toString());
  Serial.println("Vehículo ID fijo: " + String(VEHICULO_ID));

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

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: {
      wsConectado = false;
      Serial.println("❌ WebSocket desconectado - ¡SEGURIDAD ACTIVADA! Velocidad a 0");
      // SEGURIDAD INSTANTÁNEA: detener todo inmediatamente
      velocidadActual = 0;
      setVelocidad(0);
      digitalWrite(TRAC_IN1, LOW);
      digitalWrite(TRAC_IN2, LOW);
      digitalWrite(DIR_IN1, LOW);
      digitalWrite(DIR_IN2, LOW);
      digitalWrite(18, LOW);
      digitalWrite(19, LOW);
      lucesOff();
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

      if (tipo == "reset") {
        resetTodo();
        Serial.println("🔄 ESP32 reseteado (todo a cero)");
      } else if (tipo == "direccion" || tipo == "comando") {
        if (valor == "izquierda") {
          direccionActual = "izquierda";
          motorDireccionIzquierda();
        } else if (valor == "derecha") {
          direccionActual = "derecha";
          motorDireccionDerecha();
        } else if (valor == "stop") {
          direccionActual = "stop";
          motorStop();
        }
      } else if (tipo == "velocidad") {
        int v = valor.toInt();
        if (v < -100) v = -100;
        if (v > 100) v = 100;
        velocidadActual = v;
        setVelocidad(abs(v));
        aplicarTraccion();
        Serial.print("⚡ Velocidad: ");
        if (v < 0) Serial.print("RETROCESO ");
        else if (v > 0) Serial.print("AVANCE ");
        else Serial.print("DETENIDO ");
        Serial.println(String(abs(v)) + "%");
      } else if (tipo == "freno") {
        frenoActivo();
      } else if (tipo == "luces") {
        if (valor == "on") {
          lucesOn();
          lucesEncendidas = true;
        } else if (valor == "off") {
          lucesOff();
          lucesEncendidas = false;
        }
      } else if (tipo == "bocina") {
        Serial.println("📢 Bocina activada");
      } else if (tipo == "animacion") {
        Serial.println("✨ Animación activada");
      }
      break;
    }

    default: {
      break;
    }
  }
}

void enviarHeartbeat() {
  HTTPClient http;
  String url = "http://" + String(SERVER_HOST) + ":" + String(SERVER_PORT) + "/vehiculos/" + String(VEHICULO_ID);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  String json = "{\"estado\":\"CONECTADO\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
  int code = http.PUT(json);
  if (code == 200) {
    Serial.println("💓 Heartbeat OK");
  } else {
    Serial.printf("❌ Heartbeat error: %d\n", code);
    // Si el heartbeat falla, también detener todo por seguridad
    velocidadActual = 0;
    setVelocidad(0);
    digitalWrite(TRAC_IN1, LOW);
    digitalWrite(TRAC_IN2, LOW);
    digitalWrite(DIR_IN1, LOW);
    digitalWrite(DIR_IN2, LOW);
    digitalWrite(18, LOW);
    digitalWrite(19, LOW);
    lucesOff();
  }
  http.end();
}

// ===== FUNCIÓN RESET =====
void resetTodo() {
  velocidadActual = 0;
  direccionActual = "stop";
  lucesEncendidas = false;

  digitalWrite(DIR_IN1, LOW);
  digitalWrite(DIR_IN2, LOW);
  digitalWrite(18, LOW);
  digitalWrite(19, LOW);
  digitalWrite(TRAC_IN1, LOW);
  digitalWrite(TRAC_IN2, LOW);
  setVelocidad(0);
  lucesOff();
  Serial.println("🔄 Todos los sistemas reseteados a cero");
}

// ===== DIRECCIÓN =====
void motorDireccionIzquierda() {
  digitalWrite(DIR_IN1, HIGH);
  digitalWrite(DIR_IN2, LOW);
  digitalWrite(18, HIGH);
  digitalWrite(19, LOW);
  Serial.println("Dirección: IZQUIERDA");
}

void motorDireccionDerecha() {
  digitalWrite(DIR_IN1, LOW);
  digitalWrite(DIR_IN2, HIGH);
  digitalWrite(18, LOW);
  digitalWrite(19, HIGH);
  Serial.println("Dirección: DERECHA");
}

void motorStop() {
  digitalWrite(DIR_IN1, LOW);
  digitalWrite(DIR_IN2, LOW);
  digitalWrite(18, LOW);
  digitalWrite(19, LOW);
  Serial.println("Dirección: STOP");
}

// ===== TRACCIÓN =====
void aplicarTraccion() {
  if (velocidadActual > 0) {
    motorTraccionAdelante(abs(velocidadActual));
  } else if (velocidadActual < 0) {
    motorTraccionAtras(abs(velocidadActual));
  } else {
    motorTraccionDetener();
  }
}

void motorTraccionAdelante(int velocidad) {
  digitalWrite(TRAC_IN1, HIGH);
  digitalWrite(TRAC_IN2, LOW);
  setVelocidad(velocidad);
  Serial.println("Tracción: ADELANTE a " + String(velocidad) + "%");
}

void motorTraccionAtras(int velocidad) {
  digitalWrite(TRAC_IN1, LOW);
  digitalWrite(TRAC_IN2, HIGH);
  setVelocidad(velocidad);
  Serial.println("Tracción: ATRÁS a " + String(velocidad) + "%");
}

void motorTraccionDetener() {
  digitalWrite(TRAC_IN1, LOW);
  digitalWrite(TRAC_IN2, LOW);
  setVelocidad(0);
  Serial.println("Tracción: DETENIDA");
}

void frenoActivo() {
  digitalWrite(TRAC_IN1, HIGH);
  digitalWrite(TRAC_IN2, HIGH);
  setVelocidad(0);
  digitalWrite(DIR_IN1, LOW);
  digitalWrite(DIR_IN2, LOW);
  digitalWrite(18, LOW);
  digitalWrite(19, LOW);
  velocidadActual = 0;
  Serial.println("🔴 FRENO ACTIVO");
}

void setVelocidad(int porcentaje) {
  if (porcentaje < 0) porcentaje = 0;
  if (porcentaje > 100) porcentaje = 100;
  int pwm = map(porcentaje, 0, 100, 0, 255);
  ledcWrite(TRAC_ENA, pwm);
}

void lucesOn() {
  digitalWrite(LUZ_DEL_ROJO, HIGH);
  digitalWrite(LUZ_DEL_AMAR, HIGH);
  digitalWrite(LUZ_TRAS_ROJO, HIGH);
  digitalWrite(LUZ_TRAS_AMAR, HIGH);
  Serial.println("💡 Luces ON");
}

void lucesOff() {
  digitalWrite(LUZ_DEL_ROJO, LOW);
  digitalWrite(LUZ_DEL_AMAR, LOW);
  digitalWrite(LUZ_TRAS_ROJO, LOW);
  digitalWrite(LUZ_TRAS_AMAR, LOW);
  Serial.println("💡 Luces OFF");
}