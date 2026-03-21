/*
 * Servidor BLE para ESP32-S3
 * Con NOTIFICACIONES funcionando y LED visible
 * Versión compatible con todas las versiones de la librería BLE
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// ==================== DEFINICIONES ====================
// UUIDs según la práctica
#define SERVICE_UUID               "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_READ_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_WRITE_UUID  "d9c8a5a4-5e3c-4b7f-9a2b-1e4c8f7d6e5a"

// Pines para ESP32-S3
#define ADC_PIN     4       // GPIO4 - Pin con ADC válido en ESP32-S3
#define LED_PIN     38     // GPIO38 - LED integrado en la mayoría de placas ESP32-S3

// ==================== VARIABLES GLOBALES ====================
BLEServer* pServer = NULL;
BLECharacteristic* pReadCharacteristic = NULL;
bool deviceConnected = false;

// ==================== CALLBACKS DE CONEXIÓN ====================
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║   ✅ CLIENTE BLE CONECTADO        ║");
    Serial.println("╚════════════════════════════════════╝");
    Serial.println("💡 Para recibir notificaciones:");
    Serial.println("   En nRF Connect, activa NOTIFY en la característica ADC\n");
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
        Serial.println("1 → LED ENCENDIDO 💡");
        
        // Verificación adicional: leer el estado actual
        int estado = digitalRead(LED_PIN);
        Serial.print("   Estado real del pin GPIO");
        Serial.print(LED_PIN);
        Serial.print(": ");
        Serial.println(estado == HIGH ? "HIGH (Encendido)" : "LOW (Apagado)");
      } 
      else if (cmd == 0 || cmd == '0') {
        digitalWrite(LED_PIN, LOW);
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
    }
  }
};

// ==================== SETUP ====================
void setup() {
  // Inicializar Serial
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n");
  Serial.println("╔════════════════════════════════════════════════════╗");
  Serial.println("║     SERVICIO BLE ESP32-S3 - PRÁCTICA ADC/LED      ║");
  Serial.println("╚════════════════════════════════════════════════════╝");
  
  // ========== CONFIGURACIÓN DEL LED ==========
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("\n🔧 CONFIGURACIÓN DE HARDWARE:");
  Serial.print("   💡 LED en GPIO");
  Serial.println(LED_PIN);
  
  // PRUEBA DEL LED - Para verificar físicamente que funciona
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
  
  // ========== CONFIGURACIÓN BLE ==========
  Serial.println("\n🔵 CONFIGURANDO BLE...");
  
  // Inicializar dispositivo BLE
  BLEDevice::init("ESP32_BLE_Server");
  
  // Crear servidor
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Crear servicio con el UUID solicitado
  BLEService* pService = pServer->createService(SERVICE_UUID);
  
  // ========== CARACTERÍSTICA DE LECTURA (ADC) ==========
  pReadCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_READ_UUID,
    BLECharacteristic::PROPERTY_READ |   // Lectura manual
    BLECharacteristic::PROPERTY_NOTIFY    // Notificaciones automáticas
  );
  
  // AÑADIR DESCRIPTOR BLE2902 - OBLIGATORIO PARA NOTIFICACIONES
  pReadCharacteristic->addDescriptor(new BLE2902());
  
  // ========== CARACTERÍSTICA DE ESCRITURA (LED) ==========
  BLECharacteristic* pWriteCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_WRITE_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pWriteCharacteristic->setCallbacks(new MyWriteCallbacks());
  
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
  Serial.println("╔════════════════════════════════════════════════════╗");
  Serial.println("║              DATOS DEL SERVICIO BLE                ║");
  Serial.println("╠════════════════════════════════════════════════════╣");
  Serial.println("║ Nombre: ESP32_BLE_Server                          ║");
  Serial.print("║ Servicio UUID: ");
  Serial.println(SERVICE_UUID);
  Serial.print("║ Característica ADC (READ/NOTIFY): ");
  Serial.println(CHARACTERISTIC_READ_UUID);
  Serial.print("║ Característica LED (WRITE): ");
  Serial.println(CHARACTERISTIC_WRITE_UUID);
  Serial.println("╠════════════════════════════════════════════════════╣");
  Serial.println("║  📱 CONEXIÓN CON nRF CONNECT:                     ║");
  Serial.println("║  1. Escanear y conectar a ESP32_BLE_Server        ║");
  Serial.println("║  2. Expandir el servicio con UUID indicado        ║");
  Serial.println("║  3. En característica ADC: activar NOTIFY         ║");
  Serial.println("║  4. En característica LED: escribir 0 o 1         ║");
  Serial.println("╚════════════════════════════════════════════════════╝\n");
  
  Serial.println("📊 ESPERANDO CONEXIONES...\n");
}

// ==================== LOOP ====================
void loop() {
  // Leer valor del ADC (valores variables, es normal)
  int adcValue = analogRead(ADC_PIN);
  float voltage = (adcValue * 3.3) / 4095.0;
  
  // Actualizar la característica con el nuevo valor
  if (pReadCharacteristic != NULL) {
    std::string valueStr = std::to_string(adcValue);
    pReadCharacteristic->setValue(valueStr);
    
    // ENVIAR NOTIFICACIÓN si hay cliente conectado
    // El descriptor BLE2902 maneja automáticamente si el cliente quiere notificaciones
    if (deviceConnected) {
      pReadCharacteristic->notify();
    }
  }
  
  // Mostrar valores en monitor serial (cada 500ms para no saturar)
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.print("📊 ADC GPIO");
    Serial.print(ADC_PIN);
    Serial.print(": ");
    Serial.print(adcValue);
    Serial.print(" (");
    Serial.print(voltage, 2);
    Serial.print("V)");
    
    if (deviceConnected) {
      Serial.println(" 🔔 Conectado - Enviando notificaciones BLE");
    } else {
      Serial.println(" ⚪ Sin conexión BLE");
    }
    
    lastPrint = millis();
  }
  
  delay(100);
}