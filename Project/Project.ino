/**
* Smart Blinds Project Sketch for Arduino Nano 33 BLE Sense
*/

// Libraries: Stepper, Arduino_APDS9960, Arduino_LPS22HB, ArduinoBLE
#include <ArduinoBLE.h>        // BLE control
#include <Arduino_APDS9960.h>  // light intensity
#include <Arduino_LPS22HB.h>   // temperature sensor
#include <Stepper.h>           // for stepper control

bool blinds = false;

const int stepsPerRevolution = 360;

Stepper myStepper(stepsPerRevolution, 12, 11, 10, 9);

// Enums
enum BlindsState {  // Track blinds open/close state
  BLINDS_CLOSED,
  BLINDS_OPEN
};

enum OperationMode {  // Track whether automated control or manual via BLE
  MANUAL,
  AUTOMATIC
};

// Global variables
BLEService blindsService("180A");
BLEByteCharacteristic switchCharacteristic("2A57", BLERead | BLEWrite);

constexpr int SERIAL_SPEED = 9600;
const int LED = LED_BUILTIN;

bool failed_sensor = false;

bool connected_to_bluetooth = false;

BlindsState currentState = BLINDS_CLOSED;
OperationMode mode = AUTOMATIC;

// Functions
void setup() {
  myStepper.setSpeed(60);

  Serial.begin(SERIAL_SPEED);

  while (!Serial)
    ;

  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(LED, LOW);
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor!");
    failed_sensor = true;
  }

  if (!BARO.begin()) {
    Serial.println("Failed to initialize pressure sensor!");
    failed_sensor = true;
  }

  if (!BLE.begin()) {
    Serial.println("Failed to start BLE module!");
    failed_sensor = true;
  }

  if (failed_sensor) {
    Serial.println("Status: FAILED");
    Serial.println("One or more sensors failed to initialize!");
    while (1)
      ;
  } else {
    Serial.println("Status: OK");
    Serial.println("All sensors ready to go...");
  }

  BLE.setLocalName("Nano 33 BLE Sense: Blinds");
  BLE.setAdvertisedService(blindsService);

  blindsService.addCharacteristic(switchCharacteristic);
  BLE.addService(blindsService);

  BLE.advertise();

  Serial.println("BLE Service started...");
}

void loop() {
  BLEDevice central = BLE.central();

  // If bluetooth connected
  if (central.connected()) {
    if (connected_to_bluetooth == false) {
      Serial.print("CONNECTED: ");
      Serial.println(central.address());
      digitalWrite(LED, HIGH);
      connected_to_bluetooth = true;
    }

    Serial.println("Running in MANUAL control mode! 1 is open, 2 is close.");

    // Checks input
    if (switchCharacteristic.written()) {
      Serial.print(switchCharacteristic.value());
      Serial.println(" was pressed.");
      if (switchCharacteristic.value() == 1 && currentState == BLINDS_CLOSED) {
        openBlinds();
        Serial.println("OPENING blinds.");
      } else if (switchCharacteristic.value() == 1 && currentState == BLINDS_OPEN) {
        Serial.println("Sorry blinds are already OPEN.");
      } else if (switchCharacteristic.value() == 2 && currentState == BLINDS_OPEN) {
        closeBlinds();
        Serial.println("CLOSING blinds.");
      } else if (switchCharacteristic.value() == 2 && currentState == BLINDS_CLOSED) {
        Serial.println("Sorry blinds are already CLOSED.");
      } else {
        Serial.println("Invalid input!");
      }
    }

    digitalWrite(LED, LOW);
  }

  // If not bluetooth connected
  if (!central) {
    if (connected_to_bluetooth == true) {
      Serial.println("DISCONNECTED");
      digitalWrite(LED, HIGH);
      connected_to_bluetooth = false;
    }

    Serial.println("Running in AUTOMATIC control mode!");

    float pressure = BARO.readPressure();
    float temperature = BARO.readTemperature();

    temperature = (temperature * 9 / 5) + 32;

    // Prints temperature
    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println("°F");

    while (!APDS.colorAvailable()) {
      delay(5);
    }

    int r, g, b, c;

    APDS.readColor(r, g, b, c);

    // Prints light intensity
    Serial.print("Light intensity = ");
    Serial.println(c);

    // Checks temperature and light intensity
    if ((temperature > 87 || c > 800) && currentState == BLINDS_CLOSED) {
      openBlinds();
      Serial.println("Temperature above 93°F or light intensity greater than 800, OPENING blinds");
    } else if ((temperature > 87 || c > 800) && currentState == BLINDS_OPEN) {
      Serial.println("Temperature above 93°F or light intensity greater than 800, sorry blinds are already OPEN.");
    } else if ((temperature <= 87 || c <= 800) && currentState == BLINDS_OPEN) {
      closeBlinds();
      Serial.println("Temperature below 93°F or light intensity less than 800, CLOSING blinds");
    } else if ((temperature <= 87 || c <= 800) && currentState == BLINDS_CLOSED) {
      Serial.println("Temperature below 93°F or light intensity less than 800, sorry blinds are already CLOSED.");
    }
  }

  // Prints if they are open or closed
  if (currentState == BLINDS_CLOSED)
    Serial.println("Blinds are currently closed");
  else if (currentState == BLINDS_OPEN)
    Serial.println("Blinds are currently open");

  delay(1000);
}

// Function to open blinds
void openBlinds() {
  for (int i = 0; i < 9; i++)
    myStepper.step(-stepsPerRevolution);
  currentState = BLINDS_OPEN;
}

// Function to close blinds
void closeBlinds() {
  for (int i = 0; i < 9; i++)
    myStepper.step(stepsPerRevolution);
  currentState = BLINDS_CLOSED;
}
