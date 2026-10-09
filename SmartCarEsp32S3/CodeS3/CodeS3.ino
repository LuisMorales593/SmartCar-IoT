// ===== INCLUDES =====
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Adafruit_NeoPixel.h>

// ===== CONFIGURACIÓN =====
const char* SSID = "GaiaElCaballero";
const char* PASSWORD = "Gaialord40@";
const char* SERVER_HOST = "10.119.85.73";
const int SERVER_PORT = 8080;
#define VEHICULO_ID 3

// ===== PINES =====
// Motores
#define DIR_IN1 18
#define DIR_IN2 21
#define TRAC_IN1 6
#define TRAC_IN2 7
#define TRAC_ENA 4

// ===== LUCES RGB (NeoPixel) =====
#define LED_ATRAS_PIN 5
#define LED_ATRAS_NUM 4

#define LED_ADELANTE_PIN 13
#define LED_ADELANTE_NUM 4

#define LED_MOTOR_PIN 14
#define LED_MOTOR_NUM 16

#define LED_TECHO_PIN 15
#define LED_TECHO_NUM 3

#define LED_TABLERO_PIN 16
#define LED_TABLERO_NUM 6

#define LED_BAJAS_ADELANTE_PIN 10
#define LED_BAJAS_ADELANTE_NUM 4

#define LED_BAJAS_ATRAS_PIN 11
#define LED_BAJAS_ATRAS_NUM 4

// ===== SONIDO (MP3-TF-16P) =====
#define MP3_RX 8
#define MP3_TX 17
#define MP3_BUSY 9

// ===== ENUM =====
enum Direccion {
    IZQUIERDA,
    DERECHA,
    STOP
};

// ===== VARIABLES =====
volatile bool sistemaEncendido = false;
volatile bool motorEncendido = false;

volatile int velocidadActual = 0;
volatile Direccion direccionActual = STOP;
volatile bool direccionActiva = false;
volatile int tipoFreno = 0;
volatile bool atrasPresionado = false;

volatile bool lucesAtras = false;
volatile bool lucesAdelante = false;
volatile bool lucesMotor = false;
volatile bool lucesTecho = false;
volatile bool lucesTablero = false;
volatile bool lucesBajasAdelante = false;
volatile bool lucesBajasAtras = false;

volatile bool sonidoActivo = false;

volatile bool wsConectado = false;
volatile int heartbeatFallos = 0;
const int MAX_HEARTBEAT_FALLOS = 3;
WebSocketsClient webSocket;
unsigned long lastHeartbeat = 0;

// ===== OBJETOS NeoPixel (7 tiras) =====
Adafruit_NeoPixel tiraAtras         = Adafruit_NeoPixel(LED_ATRAS_NUM, LED_ATRAS_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tiraAdelante      = Adafruit_NeoPixel(LED_ADELANTE_NUM, LED_ADELANTE_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tiraMotor         = Adafruit_NeoPixel(LED_MOTOR_NUM, LED_MOTOR_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tiraTecho         = Adafruit_NeoPixel(LED_TECHO_NUM, LED_TECHO_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tiraTablero       = Adafruit_NeoPixel(LED_TABLERO_NUM, LED_TABLERO_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tiraBajasAdelante = Adafruit_NeoPixel(LED_BAJAS_ADELANTE_NUM, LED_BAJAS_ADELANTE_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tiraBajasAtras    = Adafruit_NeoPixel(LED_BAJAS_ATRAS_NUM, LED_BAJAS_ATRAS_PIN, NEO_GRB + NEO_KHZ800);

// ===== PROTOTIPOS =====
void resetTodo();
void enviarHeartbeat();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);

void motorDireccionIzquierda();
void motorDireccionDerecha();
void motorStop();
void motorTraccionAdelante(int velocidad);
void motorTraccionAtras(int velocidad);
void motorTraccionDetener();
void aplicarTraccion();
void setVelocidad(int porcentaje);
void frenoActivo();

void apagarTodasLasLuces();
void encenderLucesAtras();
void encenderLucesAdelante();
void encenderLucesMotor();
void encenderLucesTecho();
void encenderLucesTablero();
void encenderLucesBajasAdelante();
void encenderLucesBajasAtras();
void encenderFrenoAtras();

void tareaWebSocket(void *pvParameters);
void tareaHeartbeat(void *pvParameters);
void tareaMotorDireccion(void *pvParameters);
void tareaMotorVelocidad(void *pvParameters);
void tareaLuces(void *pvParameters);

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  pinMode(DIR_IN1, OUTPUT);
  pinMode(DIR_IN2, OUTPUT);
  pinMode(TRAC_IN1, OUTPUT);
  pinMode(TRAC_IN2, OUTPUT);
  pinMode(TRAC_ENA, OUTPUT);

  ledcAttach(TRAC_ENA, 5000, 8);

  tiraAtras.begin();
  tiraAdelante.begin();
  tiraMotor.begin();
  tiraTecho.begin();
  tiraTablero.begin();
  tiraBajasAdelante.begin();
  tiraBajasAtras.begin();
  apagarTodasLasLuces();

  resetTodo();

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

  xTaskCreatePinnedToCore(tareaWebSocket, "WS", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(tareaHeartbeat, "HB", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(tareaMotorDireccion, "DIR", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(tareaMotorVelocidad, "VEL", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(tareaLuces, "LUCES", 2048, NULL, 1, NULL, 0);
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}

// ===== TAREAS =====
void tareaWebSocket(void *pvParameters) {
  while (true) {
    webSocket.loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void tareaHeartbeat(void *pvParameters) {
  while (true) {
    if (WiFi.status() == WL_CONNECTED) {
      enviarHeartbeat();
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void tareaMotorDireccion(void *pvParameters) {
  Direccion ultimaDireccion = STOP;
  while (true) {
    if (direccionActual != ultimaDireccion) {
      ultimaDireccion = direccionActual;
      if (direccionActual == IZQUIERDA) {
        motorDireccionIzquierda();
      } else if (direccionActual == DERECHA) {
        motorDireccionDerecha();
      } else {
        motorStop();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void tareaMotorVelocidad(void *pvParameters) {
  while (true) {
    if (tipoFreno == 1) {
      velocidadActual = velocidadActual - 20;
      if (velocidadActual <= 0) {
        velocidadActual = 0;
        tipoFreno = 0;
      }
    }
    else if (tipoFreno == 2) {
      velocidadActual = velocidadActual - 10;
      if (velocidadActual <= 0) {
        velocidadActual = 0;
        tipoFreno = 0;
      }
    }

    setVelocidad(abs(velocidadActual));
    if (velocidadActual > 0) {
      motorTraccionAdelante(abs(velocidadActual));
    } else if (velocidadActual < 0) {
      motorTraccionAtras(abs(velocidadActual));
    } else {
      motorTraccionDetener();
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void tareaLuces(void *pvParameters) {
  while (true) {
    if (!sistemaEncendido) {
      apagarTodasLasLuces();
    } else {
      // ===== TIRA ATRAS =====
      if (tipoFreno != 0) {
        encenderFrenoAtras();
      } else if (velocidadActual < 0) {
        encenderFrenoAtras();
      } else if (atrasPresionado) {
        encenderFrenoAtras();
      } else if (direccionActiva) {
        encenderLucesAtras();
      } else {
        tiraAtras.clear();
        tiraAtras.show();
      }

      // ===== OTRAS 5 TIRAS (on/off) =====
      if (lucesAdelante) {
        encenderLucesAdelante();
      } else {
        tiraAdelante.clear();
        tiraAdelante.show();
      }

      if (lucesBajasAdelante) {
        encenderLucesBajasAdelante();
      } else {
        tiraBajasAdelante.clear();
        tiraBajasAdelante.show();
      }

      if (lucesMotor) {
        encenderLucesMotor();
      } else {
        tiraMotor.clear();
        tiraMotor.show();
      }

      if (lucesTecho) {
        encenderLucesTecho();
      } else {
        tiraTecho.clear();
        tiraTecho.show();
      }

      if (lucesTablero) {
        encenderLucesTablero();
      } else {
        tiraTablero.clear();
        tiraTablero.show();
      }

      // ===== BAJAS ATRAS: siempre activa cuando sistema ON =====
      encenderLucesBajasAtras();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ===== WEBSOCKET EVENT =====
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: {
      wsConectado = false;
      Serial.println("WebSocket desconectado - Esperando reconexion...");
      apagarTodasLasLuces();
      break;
    }

    case WStype_CONNECTED: {
      wsConectado = true;
      Serial.println("WebSocket conectado");
      String identMsg = "{\"tipo\":\"identificar\",\"vehiculoId\":" + String(VEHICULO_ID) + "}";
      webSocket.sendTXT(identMsg);
      Serial.println("Identificando: " + identMsg);
      break;
    }

    case WStype_TEXT: {
      String mensaje = String((char*)payload);
      Serial.println("Comando WebSocket: " + mensaje);

      DynamicJsonDocument doc(256);
      DeserializationError error = deserializeJson(doc, mensaje);
      if (error) {
        Serial.println("Error parseando JSON");
        return;
      }

      String tipo = doc["tipo"].as<String>();
      String valor = doc["valor"].as<String>();

      if (tipo == "arrancar") {
        if (valor == "on") {
          sistemaEncendido = true;
          Serial.println("Sistema ENCENDIDO");
        } else if (valor == "off") {
          sistemaEncendido = false;
          resetTodo();
          Serial.println("Sistema APAGADO");
        }
      }
      else if (tipo == "motor") {
        if (!sistemaEncendido) {
          Serial.println("Sistema apagado, comando ignorado");
          return;
        }
        if (valor == "on") {
          motorEncendido = true;
          Serial.println("Motor ENCENDIDO");
        } else if (valor == "off") {
          motorEncendido = false;
          motorTraccionDetener();
          Serial.println("Motor APAGADO");
        }
      }
      else if (tipo == "reset") {
        resetTodo();
        sistemaEncendido = false;
        motorEncendido = false;
        Serial.println("Reseteado");
      }
      else if (tipo == "direccion" || tipo == "comando") {
        if (!motorEncendido) {
          Serial.println("Motor apagado, comando ignorado");
          return;
        }
        if (valor == "izquierda") {
          direccionActual = IZQUIERDA;
          direccionActiva = true;
          tipoFreno = 0;
          Serial.println("Direccion: IZQUIERDA");
        } else if (valor == "derecha") {
          direccionActual = DERECHA;
          direccionActiva = true;
          tipoFreno = 0;
          Serial.println("Direccion: DERECHA");
        } else if (valor == "stop") {
          direccionActual = STOP;
          direccionActiva = false;
          Serial.println("Direccion: STOP");
        }
      }
      else if (tipo == "velocidad") {
        if (!motorEncendido) {
          Serial.println("Motor apagado, comando ignorado");
          return;
        }
        int v = valor.toInt();
        if (v < -100) v = -100;
        if (v > 100) v = 100;
        velocidadActual = v;
        tipoFreno = 0;

        Serial.print("Velocidad: ");
        if (v < 0) Serial.print("RETROCESO ");
        else if (v > 0) Serial.print("AVANCE ");
        else Serial.print("DETENIDO ");
        Serial.println(String(abs(v)) + "%");
      }
      else if (tipo == "velocidad_menos") {
        if (!motorEncendido) {
          Serial.println("Motor apagado, comando ignorado");
          return;
        }
        velocidadActual = velocidadActual - 25;
        if (velocidadActual < -100) velocidadActual = -100;
        tipoFreno = 0;
        atrasPresionado = true;

        Serial.print("Velocidad menos: ");
        Serial.println(velocidadActual);
      }
      else if (tipo == "velocidad_stop") {
        atrasPresionado = false;
        Serial.println("Velocidad stop");
      }
      else if (tipo == "bajando") {
        if (valor == "on") {
          atrasPresionado = true;
          Serial.println("Bajando: ON (frenando)");
        } else {
          atrasPresionado = false;
          Serial.println("Bajando: OFF");
        }
      }
      else if (tipo == "subiendo") {
        if (valor == "on") {
          atrasPresionado = false;
          Serial.println("Subiendo: ON");
        }
      }
      else if (tipo == "freno") {
        if (!motorEncendido) {
          Serial.println("Motor apagado, comando ignorado");
          return;
        }
        tipoFreno = 1;
        atrasPresionado = false;
        Serial.println("FRENO ROJO (rapido)");
      }
      else if (tipo == "stop_secuencial") {
        if (!motorEncendido) {
          Serial.println("Motor apagado, comando ignorado");
          return;
        }
        tipoFreno = 2;
        atrasPresionado = false;
        Serial.println("FRENO AMARILLO (lento)");
      }
      else if (tipo == "luces_altas") {
        if (!sistemaEncendido) {
          Serial.println("Sistema apagado, comando ignorado");
          return;
        }
        lucesAdelante = !lucesAdelante;
        Serial.print("Luces altas: ");
        Serial.println(lucesAdelante ? "ON" : "OFF");
      } else if (tipo == "luces_bajas") {
        if (!sistemaEncendido) {
          Serial.println("Sistema apagado, comando ignorado");
          return;
        }
        lucesBajasAdelante = !lucesBajasAdelante;
        Serial.print("Luces bajas: ");
        Serial.println(lucesBajasAdelante ? "ON" : "OFF");
      } else if (tipo == "luces_motor") {
        if (!sistemaEncendido) {
          Serial.println("Sistema apagado, comando ignorado");
          return;
        }
        lucesMotor = !lucesMotor;
        Serial.print("Luces motor: ");
        Serial.println(lucesMotor ? "ON" : "OFF");
      } else if (tipo == "luces_techo") {
        if (!sistemaEncendido) {
          Serial.println("Sistema apagado, comando ignorado");
          return;
        }
        lucesTecho = !lucesTecho;
        Serial.print("Luces techo: ");
        Serial.println(lucesTecho ? "ON" : "OFF");
      } else if (tipo == "luces_tablero") {
        if (!sistemaEncendido) {
          Serial.println("Sistema apagado, comando ignorado");
          return;
        }
        lucesTablero = !lucesTablero;
        Serial.print("Luces tablero: ");
        Serial.println(lucesTablero ? "ON" : "OFF");
      } else if (tipo == "luces_bajas_atras") {
        if (!sistemaEncendido) {
          Serial.println("Sistema apagado, comando ignorado");
          return;
        }
        lucesBajasAtras = !lucesBajasAtras;
        Serial.print("Luces bajas atras: ");
        Serial.println(lucesBajasAtras ? "ON" : "OFF");
      }
      else if (tipo == "sonido") {
        if (!sistemaEncendido) {
          Serial.println("Sistema apagado, comando ignorado");
          return;
        }
        sonidoActivo = !sonidoActivo;
        Serial.print("Bocina: ");
        Serial.println(sonidoActivo ? "pipiiiii ON" : "pipiiiii OFF");
      }
      break;
    }

    default: {
      break;
    }
  }
}

// ===== HEARTBEAT =====
void enviarHeartbeat() {
  HTTPClient http;
  String url = "http://" + String(SERVER_HOST) + ":" + String(SERVER_PORT) + "/vehiculos/" + String(VEHICULO_ID);
  http.begin(url);
  http.setTimeout(500);
  http.addHeader("Content-Type", "application/json");
  String json = "{\"estado\":\"CONECTADO\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
  int code = http.PUT(json);
  if (code == 200) {
    Serial.println("Heartbeat OK");
    heartbeatFallos = 0;
  } else {
    Serial.printf("Heartbeat error: %d\n", code);
    heartbeatFallos++;
    if (heartbeatFallos >= MAX_HEARTBEAT_FALLOS) {
      Serial.println("Demasiados fallos, reseteando...");
      resetTodo();
      sistemaEncendido = false;
      motorEncendido = false;
      heartbeatFallos = 0;
    }
  }
  http.end();
}

// ===== RESET =====
void resetTodo() {
  velocidadActual = 0;
  direccionActual = STOP;
  motorEncendido = false;
  direccionActiva = false;
  tipoFreno = 0;
  atrasPresionado = false;

  digitalWrite(DIR_IN1, LOW);
  digitalWrite(DIR_IN2, LOW);
  digitalWrite(TRAC_IN1, LOW);
  digitalWrite(TRAC_IN2, LOW);
  setVelocidad(0);
  apagarTodasLasLuces();

  lucesAtras = false;
  lucesAdelante = false;
  lucesMotor = false;
  lucesTecho = false;
  lucesTablero = false;
  lucesBajasAdelante = false;
  lucesBajasAtras = false;
  sonidoActivo = false;

  Serial.println("Reset");
}

// ===== DIRECCION =====
void motorDireccionIzquierda() {
  digitalWrite(DIR_IN1, HIGH);
  digitalWrite(DIR_IN2, LOW);
  Serial.println("Direccion: IZQUIERDA");
}

void motorDireccionDerecha() {
  digitalWrite(DIR_IN1, LOW);
  digitalWrite(DIR_IN2, HIGH);
  Serial.println("Direccion: DERECHA");
}

void motorStop() {
  digitalWrite(DIR_IN1, LOW);
  digitalWrite(DIR_IN2, LOW);
  Serial.println("Direccion: STOP");
}

// ===== TRACCION =====
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
}

void motorTraccionAtras(int velocidad) {
  digitalWrite(TRAC_IN1, LOW);
  digitalWrite(TRAC_IN2, HIGH);
  setVelocidad(velocidad);
}

void motorTraccionDetener() {
  digitalWrite(TRAC_IN1, LOW);
  digitalWrite(TRAC_IN2, LOW);
  setVelocidad(0);
}

void frenoActivo() {
  digitalWrite(TRAC_IN1, HIGH);
  digitalWrite(TRAC_IN2, HIGH);
  setVelocidad(0);
  digitalWrite(DIR_IN1, LOW);
  digitalWrite(DIR_IN2, LOW);
  velocidadActual = 0;
  Serial.println("FRENO ACTIVO");
}

void setVelocidad(int porcentaje) {
  if (porcentaje < 0) porcentaje = 0;
  if (porcentaje > 100) porcentaje = 100;
  int pwm = map(porcentaje, 0, 100, 0, 255);
  ledcWrite(TRAC_ENA, pwm);
}

// ===== LUCES WS2812B =====
void apagarTodasLasLuces() {
  tiraAtras.clear();
  tiraAdelante.clear();
  tiraMotor.clear();
  tiraTecho.clear();
  tiraTablero.clear();
  tiraBajasAdelante.clear();
  tiraBajasAtras.clear();

  tiraAtras.show();
  tiraAdelante.show();
  tiraMotor.show();
  tiraTecho.show();
  tiraTablero.show();
  tiraBajasAdelante.show();
  tiraBajasAtras.show();
}

// Tira atras: direccion (amarillo con barrido, se acelera al avanzar)
void encenderLucesAtras() {
  static int pasoBarrido = 0;
  static unsigned long ultimoPaso = 0;

  int intervalo = (velocidadActual > 0) ? 125 : 250;

  if (millis() - ultimoPaso >= intervalo) {
    ultimoPaso = millis();
    pasoBarrido++;
    if (pasoBarrido > 2) pasoBarrido = 0;
  }

  tiraAtras.clear();

  if (direccionActual == DERECHA) {
    if (pasoBarrido >= 1) tiraAtras.setPixelColor(2, 255, 200, 0);
    if (pasoBarrido >= 2) tiraAtras.setPixelColor(3, 255, 200, 0);
  } else if (direccionActual == IZQUIERDA) {
    if (pasoBarrido >= 1) tiraAtras.setPixelColor(1, 255, 200, 0);
    if (pasoBarrido >= 2) tiraAtras.setPixelColor(0, 255, 200, 0);
  }

  tiraAtras.show();
}

// Tira atras: freno (las 4 luces en rojo)
void encenderFrenoAtras() {
  for (int i = 0; i < LED_ATRAS_NUM; i++) {
    tiraAtras.setPixelColor(i, 255, 0, 0);
  }
  tiraAtras.show();
}

// Luces altas adelante (blanco)
void encenderLucesAdelante() {
  for (int i = 0; i < LED_ADELANTE_NUM; i++) {
    tiraAdelante.setPixelColor(i, 255, 255, 255);
  }
  tiraAdelante.show();
}

// Luces bajas adelante (blanco)
void encenderLucesBajasAdelante() {
  for (int i = 0; i < LED_BAJAS_ADELANTE_NUM; i++) {
    tiraBajasAdelante.setPixelColor(i, 255, 255, 255);
  }
  tiraBajasAdelante.show();
}

// Luces motor (secuencia azul verdoso)
void encenderLucesMotor() {
  for (int i = 0; i < LED_MOTOR_NUM; i++) {
    tiraMotor.setPixelColor(i, 0, 150, 100);
  }
  tiraMotor.show();
}

// Luces techo (secuencia azul verdoso)
void encenderLucesTecho() {
  for (int i = 0; i < LED_TECHO_NUM; i++) {
    tiraTecho.setPixelColor(i, 0, 150, 100);
  }
  tiraTecho.show();
}

// Luces tablero (secuencia azul verdoso)
void encenderLucesTablero() {
  for (int i = 0; i < LED_TABLERO_NUM; i++) {
    tiraTablero.setPixelColor(i, 0, 150, 100);
  }
  tiraTablero.show();
}

// Luces bajas atras (4 LEDs: blanco base, amarillo direccional, naranja al acelerar)
void encenderLucesBajasAtras() {
  static bool estadoTitileo = false;
  static unsigned long ultimoTitileo = 0;

  if (millis() - ultimoTitileo >= 125) {
    ultimoTitileo = millis();
    estadoTitileo = !estadoTitileo;
  }

  // LED 1 (indice 0): blanco, amarillo si IZQUIERDA
  if (direccionActiva && direccionActual == IZQUIERDA) {
    tiraBajasAtras.setPixelColor(0, 255, 200, 0);
  } else {
    tiraBajasAtras.setPixelColor(0, 255, 255, 255);
  }

  // LED 2 y 3 (indices 1 y 2): blanco, titilan naranja si velocidad > 0
  uint32_t colorMedio;
  if (velocidadActual > 0 && estadoTitileo) {
    colorMedio = tiraBajasAtras.Color(255, 60, 0);
  } else {
    colorMedio = tiraBajasAtras.Color(255, 255, 255);
  }
  tiraBajasAtras.setPixelColor(1, colorMedio);
  tiraBajasAtras.setPixelColor(2, colorMedio);

  // LED 4 (indice 3): blanco, amarillo si DERECHA
  if (direccionActiva && direccionActual == DERECHA) {
    tiraBajasAtras.setPixelColor(3, 255, 200, 0);
  } else {
    tiraBajasAtras.setPixelColor(3, 255, 255, 255);
  }

  tiraBajasAtras.show();
}