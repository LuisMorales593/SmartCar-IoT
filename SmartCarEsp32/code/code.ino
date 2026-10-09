#include "USB_STREAM.h"

// Buffers para los datos de la cámara
uint8_t *_xferBufferA;
uint8_t *_xferBufferB;
uint8_t *_frameBuffer;

USB_STREAM *usb;

// Esta es la firma correcta que espera la librería
void cameraFrameCallback(uvc_frame_t *frame, void *arg) {
  // El frame trae el tamaño en data_bytes
  Serial.printf("Frame recibido: %u bytes\n", frame->data_bytes);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  _xferBufferA = (uint8_t *)malloc(55 * 1024);
  _xferBufferB = (uint8_t *)malloc(55 * 1024);
  _frameBuffer = (uint8_t *)malloc(55 * 1024);

  if (_xferBufferA == NULL || _xferBufferB == NULL || _frameBuffer == NULL) {
    Serial.println("Error: No se pudo reservar memoria.");
    while (1) delay(10);
  }

  usb = new USB_STREAM();

  usb->uvcConfiguration(FRAME_RESOLUTION_ANY, FRAME_RESOLUTION_ANY,
                        FRAME_INTERVAL_FPS_15, 55 * 1024, _xferBufferA,
                        _xferBufferB, 55 * 1024, _frameBuffer);

  // El nombre correcto es uvcCamRegisterCb
  usb->uvcCamRegisterCb(&cameraFrameCallback, NULL);

  usb->start();
  Serial.println("Esperando a que se conecte la cámara...");
}

void loop() {
  delay(1000);
}
