// ===== INCLUDES =====
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiServer.h>       
#include "USB_STREAM.h"      

// ===== CONFIGURACIÓN =====
const char* SSID = "GaiaElCaballero";
const char* PASSWORD = "Gaialord40@";
const char* SERVER_HOST = "10.209.8.73"; // IP actual de tu PC
const int SERVER_PORT = 8080;
#define VEHICULO_ID 3

// ===== PINES =====
#define DIR_IN1 18
#define DIR_IN2 21
#define TRAC_IN1 6
#define TRAC_IN2 7

// NeoPixels
#define LED_ATRAS_PIN 5
#define LED_ATRAS_NUM 4
#define LED_BAJAS_ATRAS_PIN 11
#define LED_BAJAS_ATRAS_NUM 2  
#define LED_TURBO_PIN 12       
#define LED_TURBO_NUM 2        
#define LED_TECHO_PIN 15
#define LED_TECHO_NUM 3
#define LED_MOTOR_PIN 14
#define LED_MOTOR_NUM 10 
#define LED_ALTAS_PIN 13
#define LED_ALTAS_NUM 2  
#define LED_TABLERO_PIN 16
#define LED_TABLERO_NUM 4      
#define LED_NORMALES_DEL_PIN 10 
#define PIN_BATERIA 1

// ===== ENUM =====
enum Direccion { IZQUIERDA, DERECHA, STOP };

// ===== VARIABLES =====
volatile bool sistemaEncendido = false;        // Sistema encendido
volatile bool motorEncendido = false;          // Motor encendido
volatile int velocidadActual = 0;              // Velocidad actual
volatile Direccion direccionActual = STOP;     // Dirección actual
volatile bool direccionActiva = false;         // Direccional activa
volatile int tipoFreno = 0;                    // Tipo de freno
volatile bool atrasPresionado = false;         // Botón atrás presionado

volatile bool lucesAtras = false;              // Luces traseras
volatile bool lucesAdelante = false;           // Luces altas delanteras
volatile bool lucesBajasAdelante = false;      // Luces bajas delanteras
volatile bool lucesMotor = false;              // Luces del motor
volatile bool lucesTecho = false;              // Luces del techo
volatile bool lucesTablero = false;            // Luces del tablero

// CÁMARA (Inicia apagada)
bool camaraEncendida = false;                  // Cámara encendida

volatile bool wsConectado = false;             // WebSocket conectado
volatile int heartbeatFallos = 0;              // Fallos de heartbeat
const int MAX_HEARTBEAT_FALLOS = 5;            // Máximo de fallos
WebSocketsClient webSocket;                    // Cliente WebSocket

// ===== OBJETOS NeoPixel =====
Adafruit_NeoPixel tiraAtras         = Adafruit_NeoPixel(LED_ATRAS_NUM, LED_ATRAS_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tiraTurbo         = Adafruit_NeoPixel(LED_TURBO_NUM, LED_TURBO_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tiraMotor         = Adafruit_NeoPixel(LED_MOTOR_NUM, LED_MOTOR_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tiraTecho         = Adafruit_NeoPixel(LED_TECHO_NUM, LED_TECHO_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tiraTablero       = Adafruit_NeoPixel(LED_TABLERO_NUM, LED_TABLERO_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tiraAltas         = Adafruit_NeoPixel(LED_ALTAS_NUM, LED_ALTAS_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tiraBajasAtras    = Adafruit_NeoPixel(LED_BAJAS_ATRAS_NUM, LED_BAJAS_ATRAS_PIN, NEO_GRB + NEO_KHZ800);

// ===== OBJETOS CÁMARA Y MEMORIA =====
WiFiServer server(8081);
WiFiClient streamClient;
USB_STREAM *usbCam;

uint8_t *_xferBufferA = NULL;
uint8_t *_xferBufferB = NULL;
uint8_t *_frameBuffer = NULL;

static void onCameraFrameCallback(uvc_frame *frame, void *user_ptr) {
    if (frame->data_bytes > 0 && streamClient.connected()) {
        streamClient.print("--frame\r\n");
        streamClient.print("Content-Type: image/jpeg\r\n");
        streamClient.print("Content-Length: ");
        streamClient.print(frame->data_bytes);
        streamClient.print("\r\n\r\n");
        uint8_t* ptr = (uint8_t*)frame->data;
        int bytes_restantes = frame->data_bytes;
        while (bytes_restantes > 0 && streamClient.connected()) {
            int chunk_size = (bytes_restantes > 1024) ? 1024 : bytes_restantes;
            streamClient.write(ptr, chunk_size);
            ptr += chunk_size;
            bytes_restantes -= chunk_size;
            vTaskDelay(1); 
        }
        streamClient.print("\r\n");
    }
}

// ===== PROTOTIPOS =====
void resetTodo();
void enviarHeartbeat(float voltajeBateria);
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void motorDireccionIzquierda();
void motorDireccionDerecha();
void motorStop();
void motorTraccionAdelante(int velocidad);
void motorTraccionAtras(int velocidad);
void motorTraccionDetener();
void apagarTodasLasLuces();
void encenderLucesAtras();
void encenderFrenoAtras();
void tareaWebSocket(void *pvParameters);
void tareaHeartbeat(void *pvParameters);
void tareaMotorDireccion(void *pvParameters);
void tareaMotorVelocidad(void *pvParameters);
void tareaLuces(void *pvParameters);
void tareaCamara(void *pvParameters);

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  _xferBufferA = (uint8_t *)malloc(55 * 1024);
  _xferBufferB = (uint8_t *)malloc(55 * 1024);
  _frameBuffer = (uint8_t *)malloc(55 * 1024);

  pinMode(DIR_IN1, OUTPUT);
  pinMode(DIR_IN2, OUTPUT);
  pinMode(TRAC_IN1, OUTPUT);
  pinMode(TRAC_IN2, OUTPUT);
  
  ledcAttach(TRAC_IN1, 20000, 8); 
  ledcAttach(TRAC_IN2, 20000, 8); 

  // Asegurar que ambos pines de tracción arranquen en 0
  ledcWrite(TRAC_IN1, 0);
  ledcWrite(TRAC_IN2, 0);

  pinMode(LED_NORMALES_DEL_PIN, OUTPUT);
  digitalWrite(LED_NORMALES_DEL_PIN, LOW);

  tiraAtras.begin(); tiraTurbo.begin(); tiraMotor.begin();
  tiraTecho.begin(); tiraTablero.begin(); tiraAltas.begin(); tiraBajasAtras.begin();

  // Brillo al 50% en todas las tiras (ahorrar corriente)
  tiraAtras.setBrightness(128);
  tiraTurbo.setBrightness(128);
  tiraMotor.setBrightness(128);
  tiraTecho.setBrightness(128);
  tiraTablero.setBrightness(128);
  tiraAltas.setBrightness(128);
  tiraBajasAtras.setBrightness(128);

  apagarTodasLasLuces();
  resetTodo();

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado! IP: " + WiFi.localIP().toString());

  webSocket.begin(SERVER_HOST, SERVER_PORT, "/ws/comandos");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);

  xTaskCreatePinnedToCore(tareaWebSocket, "WS", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(tareaHeartbeat, "HB", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(tareaMotorDireccion, "DIR", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(tareaMotorVelocidad, "VEL", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(tareaLuces, "LUCES", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(tareaCamara, "CAM", 8192, NULL, 1, NULL, 1); 
}

void loop() { vTaskDelay(portMAX_DELAY); }

// ===== TAREA DE LA CÁMARA =====
// Iniciar la cámara y gestionar el servidor de stream
void tareaCamara(void *pvParameters) {
  usbCam = new USB_STREAM();
  
  usbCam->uvcConfiguration(480, 320, FRAME_INTERVAL_FPS_10, 55 * 1024, _xferBufferA, _xferBufferB, 55 * 1024, _frameBuffer);
  usbCam->uvcCamRegisterCb(&onCameraFrameCallback, NULL);
  
  usbCam->start();
  usbCam->connectWait(1000);
  
  usbCam->uvcCamSuspend(NULL);
  Serial.println("Cámara iniciada en modo SLEEP (apagada)");

  server.begin();

  while (true) {
    if (camaraEncendida) {
        WiFiClient newClient = server.available();
        if (newClient) {
          if (streamClient.connected()) streamClient.stop();
          
          while (newClient.connected() && newClient.available()) {
            String line = newClient.readStringUntil('\n');
            if (line == "\r" || line.length() <= 1) break;
          }
          
          newClient.println("HTTP/1.1 200 OK");
          newClient.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
          newClient.println("Access-Control-Allow-Origin: *");
          newClient.println("Connection: close");
          newClient.println();
          
          streamClient = newClient;
          streamClient.setNoDelay(true); 
          streamClient.setTimeout(1); 
          Serial.println("Navegador conectado al stream");
        }
        
        if (streamClient && !streamClient.connected()) {
            streamClient.stop();
        }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ===== TAREAS BASE =====
// Mantener activo el bucle del WebSocket
void tareaWebSocket(void *pvParameters) { while (true) { webSocket.loop(); vTaskDelay(pdMS_TO_TICKS(10)); } }

// Medir la batería y enviar el heartbeat al servidor
void tareaHeartbeat(void *pvParameters) { 
  while (true) { 
    long suma = 0;
    for(int i = 0; i < 10; i++) {
        suma += analogReadMilliVolts(PIN_BATERIA);
        delay(10);
    }
    int mv = suma / 10;
    float vBat = (mv * 4) / 1000.0;
    Serial.printf("Latido OK | Batería: %.2fV\n", vBat);
    if (WiFi.status() == WL_CONNECTED) {
      enviarHeartbeat(vBat); 
    }
    vTaskDelay(pdMS_TO_TICKS(2000)); 
  } 
}

// Controlar el motor de dirección según la dirección actual
void tareaMotorDireccion(void *pvParameters) {
  Direccion ultimaDireccion = STOP;
  while (true) {
    if (direccionActual != ultimaDireccion) {
      ultimaDireccion = direccionActual;
      if (direccionActual == IZQUIERDA) motorDireccionIzquierda();
      else if (direccionActual == DERECHA) motorDireccionDerecha();
      else motorStop();
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// Controlar la velocidad y el freno del motor de tracción
void tareaMotorVelocidad(void *pvParameters) {
  while (true) {
    if (tipoFreno == 1) { 
      if (velocidadActual > 0) { velocidadActual -= 10; if (velocidadActual < 0) velocidadActual = 0; }
      else if (velocidadActual < 0) { velocidadActual += 10; if (velocidadActual > 0) velocidadActual = 0; }
      if (velocidadActual == 0) { tipoFreno = 0; }
    }
    
    if (velocidadActual > 0) motorTraccionAdelante(abs(velocidadActual));
    else if (velocidadActual < 0) motorTraccionAtras(abs(velocidadActual));
    else motorTraccionDetener();
    
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ===== TAREA DE LUCES =====
// Controlar todas las luces del vehículo
void tareaLuces(void *pvParameters) {
  while (true) {
    if (!sistemaEncendido) {
      apagarTodasLasLuces();
    } else {
      digitalWrite(LED_NORMALES_DEL_PIN, lucesBajasAdelante ? HIGH : LOW);

      // === LUCES AUTOMÁTICAS: solo con MOTOR encendido ===
      if (motorEncendido) {

        // Atrás (freno / direccional)
        if (tipoFreno != 0 || velocidadActual < 0 || atrasPresionado) {
          encenderFrenoAtras(); 
        } else if (direccionActiva) {
          encenderLucesAtras(); 
        } else {
          for(int i=0; i<LED_ATRAS_NUM; i++) tiraAtras.setPixelColor(i, 255, 255, 255);
          tiraAtras.show();
        }

        // Bajas atrás
        uint32_t colorBajas;
        if (tipoFreno != 0 || velocidadActual < 0 || atrasPresionado) colorBajas = tiraBajasAtras.Color(255, 0, 0); 
        else if (velocidadActual > 0) colorBajas = tiraBajasAtras.Color(255, 255, 255); 
        else colorBajas = tiraBajasAtras.Color(255, 200, 0); 
        
        for(int i=0; i<LED_BAJAS_ATRAS_NUM; i++) tiraBajasAtras.setPixelColor(i, colorBajas);
        tiraBajasAtras.show();

        // Turbo
        static unsigned long ultimoTurbo = 0;
        static bool turboEstado = true;
        if (millis() - ultimoTurbo > 150) { 
            ultimoTurbo = millis();
            turboEstado = !turboEstado;
        }
        
        uint32_t colorTurbo;
        if (velocidadActual > 0) {
            colorTurbo = turboEstado ? tiraTurbo.Color(255, 255, 0) : tiraTurbo.Color(255, 60, 0);
        } else {
            colorTurbo = tiraTurbo.Color(255, 60, 0); 
        }
        
        for(int i=0; i<LED_TURBO_NUM; i++) tiraTurbo.setPixelColor(i, colorTurbo);
        tiraTurbo.show();

        // Luces motor (tornado) - controlar por flag lucesMotor
        if (lucesMotor) { 
            static unsigned long ultimoMotor = 0;
            static int tornadoIndex = 0;
            
            if (millis() - ultimoMotor > 40) { 
                ultimoMotor = millis();
                tornadoIndex = (tornadoIndex + 1) % LED_MOTOR_NUM;
            }
            
            for(int i=0; i<LED_MOTOR_NUM; i++) {
                if (i == tornadoIndex) {
                    tiraMotor.setPixelColor(i, 255, 255, 255); 
                } else {
                    tiraMotor.setPixelColor(i, 0, 0, 255); 
                }
            }
        } else { 
            tiraMotor.clear(); 
        }
        tiraMotor.show();

      } else {
        // Motor apagado -> todas las automáticas OFF
        tiraAtras.clear();
        tiraBajasAtras.clear();
        tiraTurbo.clear();
        tiraMotor.clear();
        tiraAtras.show();
        tiraBajasAtras.show();
        tiraTurbo.show();
        tiraMotor.show();
      }

      // === LUCES MANUALES: solo depender del sistema encendido ===
      if (lucesAdelante) { for(int i=0; i<LED_ALTAS_NUM; i++) tiraAltas.setPixelColor(i, 255, 255, 255); } else { tiraAltas.clear(); }
      tiraAltas.show();

      // TECHO: CIAN (0, 200, 255)
      if (lucesTecho) { for(int i=0; i<LED_TECHO_NUM; i++) tiraTecho.setPixelColor(i, 0, 200, 255); } else { tiraTecho.clear(); }
      tiraTecho.show();

      // TABLERO: CIAN (0, 200, 255)
      if (lucesTablero) { for(int i=0; i<LED_TABLERO_NUM; i++) tiraTablero.setPixelColor(i, 0, 200, 255); } else { tiraTablero.clear(); }
      tiraTablero.show();
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ===== EVENTOS WEBSOCKET =====
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: 
      wsConectado = false; 
      apagarTodasLasLuces(); 
      break;
    case WStype_CONNECTED: 
      wsConectado = true; 
      webSocket.sendTXT("{\"tipo\":\"identificar\",\"vehiculoId\":" + String(VEHICULO_ID) + "}"); 
      break;
    case WStype_TEXT: {
      DynamicJsonDocument doc(256);
      if (deserializeJson(doc, payload)) return;
      String tipo = doc["tipo"].as<String>(); String valor = doc["valor"].as<String>();
      
      if (tipo == "arrancar") { sistemaEncendido = (valor == "on"); if(!sistemaEncendido) resetTodo(); }
      else if (tipo == "motor" && sistemaEncendido) { 
          motorEncendido = (valor == "on"); 
          if (motorEncendido) {
              lucesMotor = true;  // Auto-on: luces motor al encender motor
          } else {
              velocidadActual = 0; 
              lucesMotor = false;  // Auto-off: luces motor al apagar motor
          }
      }
      else if (tipo == "reset") { resetTodo(); }
      else if (tipo == "camara_on") { 
          usbCam->uvcCamResume(NULL); 
          camaraEncendida = true;
      }
      else if (tipo == "camara_off") { 
          if (streamClient.connected()) streamClient.stop();
          usbCam->uvcCamSuspend(NULL); 
          camaraEncendida = false;
      }
      else if ((tipo == "direccion" || tipo == "comando") && motorEncendido) {
        if (valor == "izquierda") { direccionActual = IZQUIERDA; direccionActiva = true; tipoFreno = 0; }
        else if (valor == "derecha") { direccionActual = DERECHA; direccionActiva = true; tipoFreno = 0; }
        else if (valor == "stop") { direccionActual = STOP; direccionActiva = false; }
      }
      else if (tipo == "bajando") { 
          atrasPresionado = (valor == "on"); 
      }
      else if (tipo == "subiendo") { 
          atrasPresionado = false; 
      }
      
      // === ESPÍA 1: VELOCIDAD RECIBIDA ===
      else if (tipo == "velocidad" && motorEncendido) { 
          velocidadActual = constrain(valor.toInt(), -100, 100); 
          tipoFreno = 0; 
          Serial.print("Comando recibido: "); Serial.println(velocidadActual);
      }
      
      else if (tipo == "freno" && motorEncendido) { tipoFreno = 1; atrasPresionado = false; }
      else if (tipo == "stop_freno") { tipoFreno = 0; }
      else if (tipo == "luces_altas" && sistemaEncendido) lucesAdelante = !lucesAdelante;
      else if (tipo == "luces_bajas" && sistemaEncendido) lucesBajasAdelante = !lucesBajasAdelante; 
      else if (tipo == "luces_techo" && sistemaEncendido) lucesTecho = !lucesTecho;
      else if (tipo == "luces_motor" && sistemaEncendido) lucesMotor = !lucesMotor;
      else if (tipo == "luces_tablero" && sistemaEncendido) lucesTablero = !lucesTablero;
      break;
    }
    default: break;
  }
}

// === ENVIAR HEARTBEAT ===
void enviarHeartbeat(float voltajeBateria) {
  HTTPClient http; 
  String url = "http://" + String(SERVER_HOST) + ":" + String(SERVER_PORT) + "/vehiculos/" + String(VEHICULO_ID);
  http.begin(url); 
  http.setTimeout(500); 
  http.addHeader("Content-Type", "application/json");
  
  String json = "{\"estado\":\"CONECTADO\",\"ip\":\"" + WiFi.localIP().toString() + "\",\"bateria\":" + String(voltajeBateria) + "}";
  
  int code = http.PUT(json);
  
  if (code != 200) {
    if (++heartbeatFallos >= MAX_HEARTBEAT_FALLOS) { resetTodo(); sistemaEncendido = false; motorEncendido = false; heartbeatFallos = 0; }
  } else {
    heartbeatFallos = 0;
  }
  http.end();
}

// ===== FUNCIONES DE APOYO =====
void resetTodo() {
  velocidadActual = 0; direccionActual = STOP; motorEncendido = false; direccionActiva = false;
  sistemaEncendido = false; 
  tipoFreno = 0; atrasPresionado = false; digitalWrite(DIR_IN1, LOW); digitalWrite(DIR_IN2, LOW);
  motorTraccionDetener(); apagarTodasLasLuces();
  lucesAtras = lucesAdelante = lucesMotor = lucesTecho = lucesTablero = false;
  lucesBajasAdelante = false; 
  
  if (camaraEncendida) {
    if (streamClient.connected()) streamClient.stop();
    if (usbCam != NULL) usbCam->uvcCamSuspend(NULL);
    camaraEncendida = false;
    Serial.println("Cámara dormida por reset");
  }
}

void motorDireccionIzquierda() { digitalWrite(DIR_IN1, HIGH); digitalWrite(DIR_IN2, LOW); }
void motorDireccionDerecha() { digitalWrite(DIR_IN1, LOW); digitalWrite(DIR_IN2, HIGH); }
void motorStop() { digitalWrite(DIR_IN1, LOW); digitalWrite(DIR_IN2, LOW); }

// === FUNCIONES DE TRACCIÓN (FIX: ledcWrite en lugar de digitalWrite) ===
void motorTraccionAdelante(int velocidad) {
  if (velocidad <= 0) {
    ledcWrite(TRAC_IN1, 0);
    ledcWrite(TRAC_IN2, 0);
    return;
  }
  
  int v = constrain(velocidad, 1, 100);
  int pwm = map(v, 1, 100, 210, 255);
  
  static int ultimoPwmFwd = -1;
  if (pwm != ultimoPwmFwd) {
    Serial.print("FWD Vel: "); Serial.print(v); Serial.print("% | PWM: "); Serial.println(pwm);
    ultimoPwmFwd = pwm;
  }
  
  ledcWrite(TRAC_IN1, pwm);      
  ledcWrite(TRAC_IN2, 0);   
}

void motorTraccionAtras(int velocidad) {
  if (velocidad <= 0) {
    ledcWrite(TRAC_IN1, 0);
    ledcWrite(TRAC_IN2, 0);
    return;
  }
  
  int v = constrain(velocidad, 1, 100);
  int pwm = map(v, 1, 100, 210, 255);
  
  static int ultimoPwmBwd = -1;
  if (pwm != ultimoPwmBwd) {
    Serial.print("BWD Vel: "); Serial.print(v); Serial.print("% | PWM: "); Serial.println(pwm);
    ultimoPwmBwd = pwm;
  }
  
  ledcWrite(TRAC_IN1, 0);   
  ledcWrite(TRAC_IN2, pwm);      
}

void motorTraccionDetener() {
  ledcWrite(TRAC_IN1, 0);
  ledcWrite(TRAC_IN2, 0);
}

void apagarTodasLasLuces() {
  digitalWrite(LED_NORMALES_DEL_PIN, LOW); 
  tiraAtras.clear(); tiraTurbo.clear(); tiraMotor.clear(); tiraTecho.clear();
  tiraTablero.clear(); tiraAltas.clear(); tiraBajasAtras.clear();
  tiraAtras.show(); tiraTurbo.show(); tiraMotor.show(); tiraTecho.show();
  tiraTablero.show(); tiraAltas.show(); tiraBajasAtras.show();
}

void encenderLucesAtras() {
  static unsigned long ultimo = 0;
  static bool encendido = false;
  
  if (millis() - ultimo >= 350) { 
      ultimo = millis(); 
      encendido = !encendido; 
  }
  
  tiraAtras.clear();
  
  if (encendido) {
      if (direccionActual == IZQUIERDA) { 
          tiraAtras.setPixelColor(0, 255, 255, 255); 
          tiraAtras.setPixelColor(1, 255, 255, 255); 
      }
      else if (direccionActual == DERECHA) { 
          tiraAtras.setPixelColor(2, 255, 255, 255); 
          tiraAtras.setPixelColor(3, 255, 255, 255); 
      }
  }
  
  tiraAtras.show();
}

void encenderFrenoAtras() { 
    for (int i = 0; i < LED_ATRAS_NUM; i++) tiraAtras.setPixelColor(i, 255, 0, 0); 
    tiraAtras.show(); 
}