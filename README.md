# Práctica 3C: Servidor BLE con ESP32-S3

Este repositorio contiene el código fuente y la documentación de la práctica 3C de la asignatura **Microprocesadores** (UPC). Se desarrolla un servidor BLE sobre una placa ESP32-S3 que expone tres características: lectura del valor analógico de un potenciómetro (ADC), control de un LED mediante escritura y envío periódico del estado completo en formato JSON.

## Características

- **Servicio BLE** con UUID fijo: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- **Característica de lectura (ADC)**: lee el valor del pin GPIO4 (0-4095) y envía notificaciones cada 1 segundo mediante un temporizador hardware (sin polling).
- **Característica de escritura (LED)**: controla el LED integrado (GPIO38) escribiendo `0` o `1`.
- **Característica JSON**: devuelve un objeto JSON con los campos `adc`, `led` y `uptime` (tiempo de ejecución en segundos). También admite notificaciones.
- **Mensajes de depuración** por monitor serie: conexión/desconexión de clientes, valores ADC, comandos recibidos y confirmación del estado del LED.

## Requisitos

### Hardware
- Placa ESP32-S3 (ej. ESP32-S3-DevKitC-1)
- Potenciómetro de 10 kΩ (opcional, para variar el valor ADC)
- Cables de conexión

### Software
- **Arduino IDE** (versión 2.x) o **PlatformIO**
- **Librerías Arduino**:
  - `BLEDevice`, `BLEUtils`, `BLEServer` (incluidas en el paquete ESP32)
  - `BLE2902` (también incluida)
  - `ArduinoJson` (instalar desde el gestor de librerías)
- **Aplicación móvil** para pruebas: [nRF Connect](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-mobile) (Android/iOS)

## Conexiones de hardware

| Componente          | Pin ESP32-S3 |
|---------------------|--------------|
| Potenciómetro (centro) | GPIO4        |
| Potenciómetro (3.3V)   | 3.3V         |
| Potenciómetro (GND)    | GND          |
| LED integrado          | GPIO38       |

*Nota:* Si tu placa utiliza otro pin para el LED integrado (por ejemplo GPIO2, GPIO15 o GPIO48), cambia la definición `LED_PIN` en el código.

## Instalación y carga

### 1. Clonar el repositorio
```bash
git clone https://github.com/usuari/practica-3c-ble-esp32.git
cd practica-3c-ble-esp32
2. Instalar la librería ArduinoJson
Arduino IDE: Sketch → Include Library → Manage Libraries → busca "ArduinoJson" → instala la versión 7.x.

PlatformIO: añade la dependencia al archivo platformio.ini:

ini
lib_deps = bblanchon/ArduinoJson @ ^7.4.3
3. Seleccionar la placa
Arduino IDE: Tools → Board → ESP32 Arduino → ESP32S3 Dev Module.

PlatformIO: configura el archivo platformio.ini con:

ini
[env:esp32-s3-devkitm-1]
platform = espressif32
board = esp32-s3-devkitm-1
framework = arduino
monitor_speed = 115200
4. Compilar y subir el código
Arduino IDE: pulsa el botón Upload.

PlatformIO: pio run --target upload y después pio device monitor para ver la salida serie.

Uso con nRF Connect
Abre la aplicación nRF Connect en tu dispositivo móvil.

Escanea y conéctate al dispositivo ESP32_BLE_Server.

Expande el servicio con UUID 4fafc201-1fb5-459e-8fcc-c5c9c331914b.

Encontrarás tres características:

UUID	Propiedades	Descripción
beb5483e-36e1-4688-b7f5-ea07361b26a8	READ, NOTIFY	Valor ADC (0-4095). Activa NOTIFY para recibir actualizaciones cada 1 s.
d9c8a5a4-5e3c-4b7f-9a2b-1e4c8f7d6e5a	WRITE	Control del LED. Escribe 0x01 para encender, 0x00 para apagar.
e5c9a2b1-7f6d-4c8f-9e5a-3b1c7d2e8f4a	READ, NOTIFY	Estado JSON. Ejemplo: {"adc":2048,"led":true,"uptime":45}. Activa NOTIFY para recibir actualizaciones automáticas.
Activa las notificaciones (NOTIFY) en las características ADC y JSON. A partir de ese momento, los valores se actualizarán automáticamente cada segundo.

Escribe en la característica LED para encender/apagar el LED físico. El cambio se reflejará inmediatamente en la característica JSON.

Documentación adicional
El informe completo de la práctica (en PDF) se encuentra en la carpeta informe/ y se puede descargar mediante el siguiente enlace:

🔗 Informe Técnico - Práctica 3C

Este documento incluye:

Descripción detallada del código.

Análisis de resultados con capturas de pantalla.

Reflexión sobre el uso de la IA durante el desarrollo.

Conclusiones.

Reflexión sobre el uso de la IA durante el desarrollo.

Conclusiones.
