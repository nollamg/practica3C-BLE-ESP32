/*
 * Servidor BLE para ESP32-S3
 * NOTIFICACIONES automáticas CADA 1 SEGUNDO usando TIMER (sin polling)
 * LED controlable por escritura BLE
 * Característica JSON con estado completo: ADC, LED y UPTIME
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// ==================== DEFINICIONES ====================
// UUIDs según la práctica
#define SERVICE_UUID               "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_READ_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_WRITE_UUID  "d9c8a5a4-5e3c-4b7f-9a2b-1e4c8f7d6e5a"
#define CHARACTERISTIC_JSON_UUID   "e5c9a2b1-7f6d-4c8f-9e5a-3b1c7d2e8f4a"

// Pines para ESP32-S3
#define ADC_PIN     4       // GPIO4 - Pin con ADC válido en ESP32-S3
#define LED_PIN     38      // GPIO38 - LED integrado en la mayoría de placas ESP32-S3

// Temporizador
#define NOTIFY_INTERVAL_MS  1000  // 1 segundo entre notificaciones

// ==================== VARIABLES GLOBALES ====================
BLEServer* pServer = NULL;
BLECharacteristic* pReadCharacteristic = NULL;
BLECharacteristic* pJsonCharacteristic = NULL;
bool deviceConnected = false;
bool ledState = false;              // Estado actual del LED
unsigned long startTime = 0;        // Tiempo de inicio para uptime
hw_timer_t* timer = NULL;           // Timer de hardware
volatile bool sendNotification = false;  // Flag para enviar notificación desde el timer
volatile int lastAdcValue = 0;      // Último valor ADC leído

// ==================== FUNCIÓN PARA CREAR JSON ====================
String createJsonStatus(int adcValue, bool led, unsigned long uptimeSec) {
  // Crear documento JSON con capacidad suficiente
  StaticJsonDocument<128> doc;
  
  // Añadir campos al JSON
  doc["adc"] = adcValue;
  doc["led"] = led;
  doc["uptime"] = uptimeSec;
  
  // Serializar JSON a string
  String jsonString;
  serializeJson(doc, jsonString);
  
  return jsonString;
}

// ==================== CALLBACKS DE CONEXIÓN ====================
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║   ✅ CLIENTE BLE CONECTADO        ║");
    Serial.println("╚════════════════════════════════════╝");
    Serial.println("📢 NOTIFICACIONES CADA 1 SEGUNDO (Timer)");
    Serial.println("📄 Característica JSON disponible");
    Serial.println("💡 Activa NOTIFY en nRF Connect para recibir datos\n");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║   ❌ CLIENTE BLE DESCONECTADO     ║");
    Serial.println("╚════════════════════════════════════╝");
    Serial.println("🔄 Reiniciando publicidad...\n");
    pServer->startAdvertising();
  }
};

// ==================== CALLBACKS DE ESCRITURA (LED) ====================
class MyWriteCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    
    if (value.length() > 0) {
      uint8_t cmd = value[0];
      
      Serial.print("📝 Comando recibido: ");
      
      if (cmd == 1 || cmd == '1') {
        digitalWrite(LED_PIN, HIGH);
        ledState = true;
        Serial.println("1 → LED ENCENDIDO 💡");
        
        int estado = digitalRead(LED_PIN);
        Serial.print("   Estado real del pin GPIO");
        Serial.print(LED_PIN);
        Serial.print(": ");
        Serial.println(estado == HIGH ? "HIGH (Encendido)" : "LOW (Apagado)");
      } 
      else if (cmd == 0 || cmd == '0') {
        digitalWrite(LED_PIN, LOW);
        ledState = false;
        Serial.println("0 → LED APAGADO 💡");
        
        int estado = digitalRead(LED_PIN);
        Serial.print("   Estado real del pin GPIO");
        Serial.print(LED_PIN);
        Serial.print(": ");
        Serial.println(estado == HIGH ? "HIGH (Encendido)" : "LOW (Apagado)");
      }
      else {
        Serial.print(cmd);
        Serial.println(" → Comando no válido (usa 0 o 1)");
      }
      
      // Actualizar característica JSON después de cambiar el LED
      if (pJsonCharacteristic != NULL && deviceConnected) {
        unsigned long uptime = (millis() - startTime) / 1000;
        String jsonStatus = createJsonStatus(lastAdcValue, ledState, uptime);
        pJsonCharacteristic->setValue(jsonStatus.c_str());
        pJsonCharacteristic->notify();
        Serial.println("   📄 JSON actualizado por cambio de LED");
      }
    }
  }
};

// ==================== FUNCIÓN DEL TIMER (ISR) ====================
// Esta función se ejecuta automáticamente CADA 1 SEGUNDO sin polling
void IRAM_ATTR onTimer() {
  // Leer ADC dentro de la ISR (es rápido y seguro)
  lastAdcValue = analogRead(ADC_PIN);
  
  // Activar flag para enviar notificación desde el loop
  sendNotification = true;
}

// ==================== SETUP ====================
void setup() {
  // Inicializar Serial
  Serial.begin(115200);
  delay(1000);
  
  // Registrar tiempo de inicio
  startTime = millis();
  
  Serial.println("\n");
  Serial.println("╔════════════════════════════════════════════════════════════╗");
  Serial.println("║   SERVICIO BLE ESP32-S3 - NOTIFICACIONES TIMER + JSON    ║");
  Serial.println("║           (SIN POLLING - CADA 1 SEGUNDO)                  ║");
  Serial.println("╚════════════════════════════════════════════════════════════╝");
  
  // ========== CONFIGURACIÓN DEL LED ==========
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  ledState = false;
  
  Serial.println("\n🔧 CONFIGURACIÓN DE HARDWARE:");
  Serial.print("   💡 LED en GPIO");
  Serial.println(LED_PIN);
  
  // PRUEBA DEL LED
  Serial.print("   Probando LED");
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
    Serial.print(".");
  }
  Serial.println(" ✅");
  
  // ========== CONFIGURACIÓN DEL ADC ==========
  analogReadResolution(12);           // Resolución 12 bits (0-4095)
  analogSetAttenuation(ADC_11db);     // Rango 0-3.3V
  
  Serial.print("   📊 ADC en GPIO");
  Serial.println(ADC_PIN);
  Serial.println("   Rango: 0-4095 (0V - 3.3V)");
  
  // ========== CONFIGURACIÓN DEL TIMER (SIN POLLING) ==========
  Serial.println("\n⏱️ CONFIGURANDO TIMER CADA 1 SEGUNDO...");
  
  timer = timerBegin(0, 80, true);
  timerAlarmWrite(timer, NOTIFY_INTERVAL_MS * 1000, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmEnable(timer);
  
  Serial.print("   ✅ Timer configurado cada ");
  Serial.print(NOTIFY_INTERVAL_MS);
  Serial.println(" ms");
  Serial.println("   Las notificaciones se disparan automáticamente sin polling");
  
  // ========== CONFIGURACIÓN BLE ==========
  Serial.println("\n🔵 CONFIGURANDO BLE...");
  
  // Inicializar dispositivo BLE
  BLEDevice::init("ESP32_BLE_Server");
  
  // Crear servidor
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Crear servicio con el UUID solicitado
  BLEService* pService = pServer->createService(SERVICE_UUID);
  
  // ========== CARACTERÍSTICA 1: LECTURA ADC (READ/NOTIFY) ==========
  pReadCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_READ_UUID,
    BLECharacteristic::PROPERTY_READ |   
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pReadCharacteristic->addDescriptor(new BLE2902());
  pReadCharacteristic->setValue("0");
  
  // ========== CARACTERÍSTICA 2: ESCRITURA LED (WRITE) ==========
  BLECharacteristic* pWriteCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_WRITE_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pWriteCharacteristic->setCallbacks(new MyWriteCallbacks());
  
  // ========== CARACTERÍSTICA 3: JSON (READ/NOTIFY) ==========
  pJsonCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_JSON_UUID,
    BLECharacteristic::PROPERTY_READ |   
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pJsonCharacteristic->addDescriptor(new BLE2902());
  
  // Valor inicial del JSON
  String initialJson = createJsonStatus(0, false, 0);
  pJsonCharacteristic->setValue(initialJson.c_str());
  
  // Iniciar servicio
  pService->start();
  
  // ========== CONFIGURAR PUBLICIDAD ==========
  BLEAdvertising* pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  
  // Iniciar publicidad
  pServer->startAdvertising();
  
  // ========== MOSTRAR INFORMACIÓN FINAL ==========
  Serial.println("✅ SERVIDOR BLE INICIADO CORRECTAMENTE\n");
  Serial.println("╔════════════════════════════════════════════════════════════╗");
  Serial.println("║                   DATOS DEL SERVICIO BLE                   ║");
  Serial.println("╠════════════════════════════════════════════════════════════╣");
  Serial.println("║ Nombre: ESP32_BLE_Server                                   ║");
  Serial.print("║ Servicio UUID: ");
  Serial.println(SERVICE_UUID);
  Serial.print("║ Característica ADC (READ/NOTIFY): ");
  Serial.println(CHARACTERISTIC_READ_UUID);
  Serial.print("║ Característica LED (WRITE): ");
  Serial.println(CHARACTERISTIC_WRITE_UUID);
  Serial.print("║ Característica JSON (READ/NOTIFY): ");
  Serial.println(CHARACTERISTIC_JSON_UUID);
  Serial.println("╠════════════════════════════════════════════════════════════╣");
  Serial.println("║  📄 FORMATO JSON:                                          ║");
  Serial.println("║     {\"adc\": valor, \"led\": true/false, \"uptime\": segundos} ║");
  Serial.println("╠════════════════════════════════════════════════════════════╣");
  Serial.println("║  ⏱️  NOTIFICACIONES: CADA 1 SEGUNDO (TIMER)                ║");
  Serial.println("║  📱 CONEXIÓN CON nRF CONNECT:                              ║");
  Serial.println("║  1. Escanear y conectar a ESP32_BLE_Server                 ║");
  Serial.println("║  2. Expandir el servicio con UUID indicado                 ║");
  Serial.println("║  3. En característica ADC: activar NOTIFY                  ║");
  Serial.println("║  4. En característica JSON: activar NOTIFY                 ║");
  Serial.println("║  5. En característica LED: escribir 0 o 1                  ║");
  Serial.println("╚════════════════════════════════════════════════════════════╝\n");
  
  Serial.println("📊 ESPERANDO CONEXIONES...\n");
  Serial.println("⏱️  El timer está enviando notificaciones CADA 1 SEGUNDO");
  Serial.println("   (sin polling - el loop solo procesa eventos)\n");
}

// ==================== LOOP ====================
void loop() {
  // Verificar si el timer ha activado el flag
  if (sendNotification) {
    // Limpiar el flag
    sendNotification = false;
    
    // Calcular uptime actual (segundos desde inicio)
    unsigned long uptime = (millis() - startTime) / 1000;
    
    // Calcular voltaje para debug
    float voltage = (lastAdcValue * 3.3) / 4095.0;
    
    // Mostrar en monitor serial
    Serial.print("⏱️  [TIMER] ADC GPIO");
    Serial.print(ADC_PIN);
    Serial.print(": ");
    Serial.print(lastAdcValue);
    Serial.print(" (");
    Serial.print(voltage, 2);
    Serial.print("V) | LED: ");
    Serial.print(ledState ? "ON" : "OFF");
    Serial.print(" | Uptime: ");
    Serial.print(uptime);
    Serial.print("s");
    
    // Enviar NOTIFICACIÓN de la característica ADC
    if (deviceConnected && pReadCharacteristic != NULL) {
      std::string valueStr = std::to_string(lastAdcValue);
      pReadCharacteristic->setValue(valueStr);
      pReadCharacteristic->notify();
      Serial.print(" 🔔 ADC");
    }
    
    // Crear y enviar NOTIFICACIÓN de la característica JSON
    if (deviceConnected && pJsonCharacteristic != NULL) {
      String jsonStatus = createJsonStatus(lastAdcValue, ledState, uptime);
      pJsonCharacteristic->setValue(jsonStatus.c_str());
      pJsonCharacteristic->notify();
      Serial.print(" | 📄 JSON");
    }
    
    if (deviceConnected) {
      Serial.println(" NOTIFICACIONES ENVIADAS");
    } else {
      Serial.println(" ⚪ Sin cliente conectado");
    }
  }
  
  // Pequeña pausa para no saturar el procesador
  delay(10);
}