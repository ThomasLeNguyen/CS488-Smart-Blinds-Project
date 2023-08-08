/**
* Smart Blinds Project Sketch for Arduino Nano 33 BLE Sense
*/

// Libraries: Stepper, Arduino_APDS9960, Arduino_LPS22HB,  
#include <ArduinoBLE.h> // BLE control
#include <Arduino_APDS9960.h> // light intensity
#include <Arduino_LPS22HB.h> // temperature sensor
//#include <Stepper.h> // for stepper control

// Enums
enum BlindsState { // Track blinds open/close state
  BLINDS_CLOSED,
  BLINDS_OPEN
};

enum OperationMode { // Track whether automated control or manual via BLE
  MANUAL,
  AUTOMATIC
};

// Global variables
BLEService blindsService("180A");
BLEByteCharacteristic switchCharacteristic("2A57", BLERead | BLEWrite);

constexpr int SERIAL_SPEED = 9600;
const int LED = LED_BUILTIN;

bool failed_sensor = false;

BlindsState currentState = BLINDS_CLOSED;
OperationMode mode = AUTOMATIC;

// Functions
void setup() {
  Serial.begin(SERIAL_SPEED);

  while (!Serial);

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

  if(failed_sensor) {
    Serial.println("Status: FAILED");
    Serial.println("One or more sensors failed to initialize!");
    while (1);
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

  if(central) {
    Serial.print("Connected: ");
    Serial.println(central.address());
    digitalWrite(LED, HIGH);

    mode = MANUAL;

    // **** Start BLE logic

    while (central.connected()) {
      if (switchCharacteristic.written()) {
        switch(switchCharacteristic.value()) {
          // case 1 (open), case 2 (close), default
          case 01:
            openBlinds();
            break;
          case 02:
            closeBlinds();
            break;
          default:
            Serial.println("Ignored input.");
            break;
        }
      }
    }

    // **** On Disconnection

    Serial.println("Disconnected...");
    digitalWrite(LED, LOW);

    mode = AUTOMATIC;
  }

  if(mode == AUTOMATIC) {
    Serial.println("Running in Automatic control mode");

    switch(currentState) {
      case BLINDS_CLOSED:
        Serial.println("Blinds are currently closed");
        break;
      case BLINDS_OPEN:
        Serial.println("Blinds are currently open");
        
        //closing blinds if temp is above 80
        if(temperature > 80){
        closeBlinds();
        Serial.println("Temperature above 80°F, closing blinds");
        }
        //closing blinds if clear light is above 800 
        if(c > 800){
          closeBlinds();
          Serial.println("Too bright outside, closing blinds");
        }
        break;
    }

  }
  //getting temp from sensor
  float temperature = BARO.readTemperature();

  //converting temp to fahrenheit
  temperature = (temperature * 9/5) + 32;

  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println("°F");
  Serial.println();

  // check if a color reading is available
  while (! APDS.colorAvailable()) {
    delay(5);
  }
  int r, g, b, c;

  // read the color
  APDS.readColor(r, g, b, c);

  // print the value of c only
  Serial.print("c = ");
  Serial.println(c);
  Serial.println();

  delay(1000);
}

void openBlinds() {
  Serial.println("Opening Blinds....");
  currentState = BLINDS_OPEN;
}

void closeBlinds() {
  Serial.println("Closing Blinds....");
  currentState = BLINDS_CLOSED;
}
