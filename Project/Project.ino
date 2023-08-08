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

constexpr float TEMP_MAX_THRESHOLD = 95; // degrees Fahrenheit
constexpr float LIGHT_MAX_THRESHOLD = 800; // light intensity (clear channel from ADPS9960)

constexpr float TEMP_MIN_THRESHOLD = 72; // degrees Fahrenheit
constexpr float LIGHT_MIN_THRESHOLD = 100; // light intensity (clear channel from ADPS9960)

// Functions
void setup() {
  Serial.begin(SERIAL_SPEED);
  while (!Serial);

  if (!BARO.begin()) {
    Serial.println("Failed to initialize pressure sensor!");
    failed_sensor = true;
  }

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor!");
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

  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(LED, LOW);
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);

  BLE.setLocalName("Nano 33 BLE Sense: Blinds");
  BLE.setAdvertisedService(blindsService);

  blindsService.addCharacteristic(switchCharacteristic);
  BLE.addService(blindsService);

  BLE.advertise();

  Serial.println("BLE Service started...");
}

void loop() {

  // for some weird reason, temperature reads 0 when barometer pressure sensor is not read from?
  float pressure = BARO.readPressure();

  // this also affects temperature reading? Was reading temp of 95F when room temp is only 79-83. 
  float altitude = 44330 * ( 1 - pow(pressure/101.325, 1/5.255) );

  // print the sensor value
  Serial.print("Altitude according to kPa is = ");
  Serial.print(altitude);
  Serial.println(" m");

  //getting temp from sensor
  float temperature = (BARO.readTemperature() * 9) / 5 + 32;
  int r, g, b, c;
  
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println("°F");
  Serial.println();

  // check if a color reading is available
  while (! APDS.colorAvailable()) {
    delay(5);
  }

  // read the color
  APDS.readColor(r, g, b, c);

  // print the value of c only
  Serial.print("c = ");
  Serial.println(c);
  Serial.println();

  BLEDevice central = BLE.central();

  if(central) {
    Serial.print("Connected: ");
    Serial.println(central.address());

    digitalWrite(LED, HIGH);
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDB, HIGH);

    mode = MANUAL;

    // **** Start BLE logic

    while (central.connected()) {
      if (switchCharacteristic.written()) {
        switch(switchCharacteristic.value()) {
          // case 1 (open), case 2 (close), default
          case 01:
            if (currentState == BLINDS_CLOSED)
            {
              openBlinds();
            }
            break;
          case 02:
            if (currentState == BLINDS_OPEN)
            {
              closeBlinds();
            }
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
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDB, LOW);

    Serial.println("Running in Automatic control mode");

    switch(currentState) {
      case BLINDS_CLOSED:
        Serial.println("Blinds are currently closed");
        if (temperature < TEMP_MIN_THRESHOLD)
        {
          openBlinds();
        }
        if (c < LIGHT_MIN_THRESHOLD)
        {
          openBlinds();
        }
        break;
      case BLINDS_OPEN:
        Serial.println("Blinds are currently open");
        
        //closing blinds if temp is above 80
        if (temperature > TEMP_MAX_THRESHOLD){
        closeBlinds();
        Serial.println("Temperature above 80°F, closing blinds");
        }
        //closing blinds if clear light is above 800 
        if (c > LIGHT_MAX_THRESHOLD){
          closeBlinds();
          Serial.println("Too bright outside, closing blinds");
        }
        break;
    }

  }

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
