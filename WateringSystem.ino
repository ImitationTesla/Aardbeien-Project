#define NumberOfPlants 3
// The current maximum amount of supervisable plants is 8 after proper hardware modification
// System can only function when the soil is not severely dehydrated before the system is deployed.
// System works best with grow-pots with water trays
bool waterTrayMode[] = {false, false, true, false, false, false, false, false};  //if set true, the system will fill the tray regardless of soil moisture level
byte HumiditySensor[] = {A0, A1, A2, A3, A4, A5, A6, A7};         // {A0, A1, A2,  A3,  A4,  A5,  A6,  A7};
byte WaterLevelSensor[] = {A8, A9, A10, A11, A12, A13, A14, A15}; // {A8, A9, A10, A11, A12, A13, A14, A15};
int relay[] = {29, 28, 27, 26, 25, 24, 23, 22};  // amount of relays controlling the valves

bool pumpKillswitch = false;              // disables pump during testing

const int pump = 23;                      // pin associated with the pump-cycle
const int drain = 22;                     // pin associated with the drain-cycle
// nat potgrond: 271
// lucht: 1023

//TODO: find way to implement this concept to code
//int  someRegularValue;
//static int someStaticValue;
//static void someFunction(void){
//    if (something) {
//    }
//    else;
//  }


//DEFINE MINIMUM_HUMIDITY (400)
const int minimalHumidity = 400;          // at this point soil moisture is at 50% of detectable level
const int minimalHumidityWaterLevelSensor = 700;
const int noSensorValue = 1020;            // value prohibits row from activating when sensor is not in the ground
const int pumpRuntime = 5000;             // the delay until pump is switched off
const int testingSamples = 20;            // for the readSensorValue() and readWaterCheck(), more samples = higher accuracy

volatile int test, x, RSV, RWC, sensorValue, waterCheck = 0;

void setup(void) {
  // put your setup code here, to run once:
  Serial.begin(57600);
  for (int a = 0; a < NumberOfPlants; a++) {
    pinMode(relay[a], OUTPUT);
  }
  Serial.println("The Sketch has started! Here we go.");
  //motorPins
  pinMode(pump, OUTPUT);
  pinMode(drain, OUTPUT);

  resetValves();
}
void loop(void) {
  // put your main code here, to run repeatedly:

  //the code will check the plants one by one and evaluate the waterlevels in the soil and tray
  for (int valve = 0; valve < NumberOfPlants; valve++) {

    Serial.println();
    Serial.print("sensor: ");
    Serial.print(valve);
    RSV = ReadSensorValue(valve, testingSamples);
    Serial.print(" - ReadSensorValue = ");
    Serial.print(RSV);

    RWC = ReadWaterCheck(valve, testingSamples);
    Serial.print(" - ReadWaterCheck = ");
    Serial.print(RWC);

    //if a soil sensor reads a very low moisture-content the system will assume the sensor is not used
    //and skips to the next plant in the process. Otherwise system executes stage 1.
    if (RSV < noSensorValue) {
      // stage 1: is the soil dry? and is there any water in the tray? if there is water in the tray move to stage 3, otherwise activates the pump cycle.
      if (waterTrayMode[valve] == false && RSV > minimalHumidity && RWC > minimalHumidityWaterLevelSensor || waterTrayMode[valve] == true && RWC > minimalHumidityWaterLevelSensor) {
        Serial.println(" - soil is too dry and no water in tray");
        //closing appropriate valves
        resetValves();
        delay(20);
        digitalWrite(relay[valve], LOW);

        if (waterTrayMode[valve] == true) {
          do {
            delay(200);
            digitalWrite(relay[valve], LOW);
            x = QuickCheck(valve);
            pumpWater();
            resetValves();
            if ( x >= minimalHumidityWaterLevelSensor) {
              break;
            }
          } while (x > minimalHumidityWaterLevelSensor);
        } else if (waterTrayMode[valve] == false) {
        pumpWater();
          resetValves();
        } else {
          Serial.print("something is wrong in the do while loop");
        }
      }

      // stage 2: if there is enough water in the soil and tray, then we assume a over abundance of water. this is the drain cycle.
      else if (RSV < minimalHumidity && RWC < minimalHumidityWaterLevelSensor) {
        Serial.println(" - soil is moist and water tray is full, will drain excess water.");
        Serial.println();
        resetValves();
        delay(20);
        digitalWrite(relay[valve], LOW);

        drainWater();
        resetValves();
      }
      // stage 3: the system assumes that the soil needs time to absorb the water. No actions taken
      else if (RSV > minimalHumidity && RWC < minimalHumidityWaterLevelSensor) {
        Serial.print(" - soil is dry and the water tray is full, will hold off watering cycle until water tray is depleted.");
      } else if (RSV > minimalHumidity && RWC < minimalHumidityWaterLevelSensor) {
        // stage 4: the soil is moist and the tray is (mostly) empty, here the system assumes all is fine!
        Serial.print(" - water level is OK! - Soil is moist and water tray is empty.");
      } else {
        Serial.print("ERROR - None of the conditions where hit");
      }
    } else {
      // stage 1b: the sensor is assummed not to be in soil due to exceding the noSensorValue
      Serial.println(" - This valve has been deactivated or the soil is too dry.");
    }
  }
  //this delay is to ensure the pump is less likely to overheat. Slowing the process down.
  delay(5000);
}

int QuickCheck(int sensor) {
  test = analogRead(WaterLevelSensor[sensor]);
  Serial.print("This one needs extra attention. Value is: ");
  Serial.print(test);
  return test;
}

int ReadSensorValue(int sensor, int testingSamples) {
  int sensorValueArray[testingSamples];
  int i, totalSensorValue = 0;

  for (int i = 0; i < testingSamples; i++) {
    sensorValueArray[i] = analogRead(HumiditySensor[sensor]);
    totalSensorValue += sensorValueArray[i];
    delay(2000);
  }

  int sensorValue = totalSensorValue / testingSamples;
  for (int i = 0; i < testingSamples; i++) {
//    Serial.print("sensorValueArray %i = %i", i, sensorValueArray[i]);
  }
//  Serial.println("sensorValue = %i", sensorValue);
  return sensorValue;
}

int ReadWaterCheck(int sensor, int testingSamples) {
  int waterCheckArray[testingSamples];
  int j, totalWaterCheck = 0;

  for (int j = 0; j < testingSamples; j++) {
    waterCheckArray[j] = analogRead(WaterLevelSensor[sensor]);
    totalWaterCheck += waterCheckArray[j];
    delay(10);
  }
  int waterCheck = totalWaterCheck / testingSamples;


  return waterCheck;
}

void resetValves(void) {
  //pre-cautionairy class to ensure all valves are closed before rows are executed
  Serial.println();
  for (int reset = 0; reset < NumberOfPlants; reset++) {
    digitalWrite(relay[reset], HIGH);
  }
  digitalWrite(pump, HIGH);
  digitalWrite(drain, HIGH);
  Serial.println("valves have been reset.");
  Serial.println();
}

void pumpWater(void) {
  Serial.println("\n entering watering cycle");
  Serial.println(" engaging motor");
  delay(200);
  digitalWrite(pump, LOW);
  delay(pumpRuntime);
  digitalWrite(pump, HIGH);
  delay(200);
  Serial.println(" disengaging motor");
}

void drainWater(void) {
  Serial.println("\n entering draining cycle");
  Serial.println(" engaging motor");
  delay(200);
  digitalWrite(drain, LOW);
  delay(pumpRuntime);
  digitalWrite(drain, HIGH);
  delay(200);
  Serial.println(" disengaging motor");
}
