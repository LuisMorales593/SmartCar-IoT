// ===== TEST LED NORMAL - GPIO 2 =====
// Sketch independiente, no modifica el de los 4 LEDs WS2812B
#define LED_PIN 2

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Test LED normal en GPIO 2");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  Serial.println("LED ON");
  digitalWrite(LED_PIN, HIGH);
  delay(500);

  Serial.println("LED OFF");
  digitalWrite(LED_PIN, LOW);
  delay(500);
}
