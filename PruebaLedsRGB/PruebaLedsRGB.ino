// ===== TEST 4 LEDS WS2812B - PIN 5 =====
// Fase 1: 5 colores (todos los LEDs a la vez)
// Fase 2: Ping-pong BLANCO (ida) / ROJO (vuelta)
#include <Adafruit_NeoPixel.h>

#define LED_PIN   5
#define LED_NUM   4
#define BRILLO    50   // 0-255

Adafruit_NeoPixel tira(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

// ===== 5 colores de prueba (R, G, B) =====
const uint8_t colores[5][3] = {
  {255, 0,   0},    // Rojo
  {0,   255, 0},    // Verde
  {0,   0,   255},  // Azul
  {255, 200, 0},    // Amarillo
  {255, 255, 255}   // Blanco
};
const char* nombres[5] = {"ROJO", "VERDE", "AZUL", "AMARILLO", "BLANCO"};

// Colores para la animación
const uint8_t BLANCO[3] = {255, 255, 255};
const uint8_t ROJO[3]   = {255, 0,   0};

// ===== FUNCIONES =====
void pintarTodos(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < LED_NUM; i++) {
    tira.setPixelColor(i, r, g, b);
  }
  tira.show();
}

void faseColores() {
  Serial.println("--- FASE 1: 5 COLORES ---");
  for (int c = 0; c < 5; c++) {
    Serial.print("Color: ");
    Serial.println(nombres[c]);

    pintarTodos(colores[c][0], colores[c][1], colores[c][2]);
    delay(800);

    tira.clear();
    tira.show();
    delay(200);
  }
}

void fasePingPong() {
  Serial.println("--- FASE 2: PING-PONG BLANCO / ROJO ---");

  // IDA: 0 -> 3 en BLANCO
  for (int i = 0; i < LED_NUM; i++) {
    tira.clear();
    tira.setPixelColor(i, BLANCO[0], BLANCO[1], BLANCO[2]);
    tira.show();
    Serial.print("IDA    - LED ");
    Serial.print(i);
    Serial.println(" BLANCO");
    delay(250);
  }

  // VUELTA: 3 -> 0 en ROJO
  for (int i = LED_NUM - 1; i >= 0; i--) {
    tira.clear();
    tira.setPixelColor(i, ROJO[0], ROJO[1], ROJO[2]);
    tira.show();
    Serial.print("VUELTA - LED ");
    Serial.print(i);
    Serial.println(" ROJO");
    delay(250);
  }

  tira.clear();
  tira.show();
  delay(300);
}

// ===== SETUP / LOOP =====
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Test 4 LEDs WS2812B en pin 5");

  tira.begin();
  tira.setBrightness(BRILLO);
  tira.clear();
  tira.show();
  delay(500);
}

void loop() {
  faseColores();    // 5 colores -> todos los LEDs a la vez
  fasePingPong();   // blanco ida / rojo vuelta
  delay(500);       // pausa antes de repetir el ciclo completo
}