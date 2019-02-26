#define RELAY_SIZE 8
#define SENSORS 2

const int relay8 = 29;
const int relay7 = 28;
const int OnOff = 27;
const int DrainValve = 26;
const int Valve2 = 25;
const int Valve1 = 24;
const int Sensor2 = 23;
const int Sensor1 = 22;

bool activated[] = {true, false};
byte HumiditySensor[] = {A0, A1, A2};
int sensors[] = {22, 23};
int valves[] = {24, 24};
int relay[] = {29, 28, 27, 26, 25, 24, 23, 22};  // amount of relays controlling the system

const int minimumValue = 1000;
const int pumpRuntime = 12500;
const int testingSamples = 10;

int RSV, sensorValue = 0;

void setup() {
  //=========== INNITIALIZATION ============
  Serial.begin(57600);
  delay(1000);
  Serial.println("### Engaged ###");
  for (int a = 0; a < RELAY_SIZE; a++) {
    pinMode(relay[a], OUTPUT);
  }
  for (int b = 0; b < SENSORS; b++) {
    pinMode(HumiditySensor[b], INPUT);
  }
  resetValves();
  //========================================

  //++++++++++ PROGRAM: automate watering-cycle +++++++++++++++++
  //the code only needs to run once for every valve.
  for (int valve = 0; valve < SENSORS; valve++) {
    if (activated[valve] != false) {
      //measure water level and print result
      Serial.println();
      Serial.print("reading sensor: ");
      Serial.print(valve);
      RSV = RunTest(valve, testingSamples);
      resetValves();

      //if the waterlevel is low, activate pump en appropriate valve
      if (RSV > minimumValue) {
        ActivatePump(valve);

      } else if (RSV == minimumValue) {
        Serial.println("Water supply sufficient.");
      } else {
        Serial.println("ERROR");
      }
    } else if (activated[valve] == false) {
      Serial.println("Valve is deactivated.");
    } else {
      Serial.print("something is wrong in the loop");
    }
  }
  //++++ PROGRAM END ++++++++++++++++++++
}

void loop() {
  // put your main code here, to run repeatedly:
  resetValves();
}

int RunTest(int sensor, int testingSamples) {
  int sensorValueArray[testingSamples];
  int i, totalSensorValue = 0;
  digitalWrite(sensors[sensor], LOW);
  delay(200);
  for (int i = 0; i < testingSamples; i++) {
    sensorValueArray[i] = analogRead(HumiditySensor[sensor]);
    totalSensorValue += sensorValueArray[i];
    delay(20);
  }

  int sensorValue = totalSensorValue / testingSamples;
  for (int i = 0; i < testingSamples; i++) {
    sensorValueArray[i];
  }
  Serial.println(" - Sensor reads: ");
  Serial.print(sensorValue);
  digitalWrite(sensors[sensor], HIGH);
  return sensorValue;
}

void resetValves(void) {
  //pre-cautionairy class to ensure all valves are closed before rows are executed
  Serial.println();
  for (int reset = 0; reset < RELAY_SIZE; reset++) {
    digitalWrite(relay[reset], HIGH);
    //Serial.println(reset);
  }
  Serial.print("relais on standby.");
  Serial.println();
}

int ActivatePump(int valve) {
  Serial.println("\n entering pumping cycle");
  if (activated[valve] != false) {
    digitalWrite(valves[valve], LOW);
    
    Serial.print("opening valve ");
    Serial.print(valve);
    
    delay(200);
    Serial.println(" engaging motor");
    digitalWrite(relay7, LOW);
    digitalWrite(relay8, LOW);
    delay(20);
    digitalWrite(OnOff, LOW);
    delay(pumpRuntime);
    digitalWrite(OnOff, HIGH);
    delay(200);
    digitalWrite(relay7, HIGH);
    digitalWrite(relay8, HIGH);
    delay(200);
    
    digitalWrite(valves[valve], HIGH);
    Serial.println("closing valve ");
    Serial.print(valve);
  } else {
    Serial.println("valve ");
    Serial.println(valve);
    Serial.println(" is deactivated, pumping cycle aborted");
  }
  Serial.println(" disengaging motor");
}

int Drain(int valve) {
  if (activated[valve] != false) {
    digitalWrite(valves[valve], LOW);
    delay(200);
    Serial.println(" engaging motor");
    digitalWrite(relay7, HIGH);
    digitalWrite(relay8, HIGH);
    delay(pumpRuntime);
    digitalWrite(relay7, LOW);
    digitalWrite(relay8, LOW);
    delay(200);
    digitalWrite(valves[valve], HIGH);
  } else {
    Serial.println("valve ");
    Serial.println(valve);
    Serial.println(" is deactivated, pumping cycle aborted");
  }
  Serial.println(" disengaging motor");
}

void Purge(void) {
  resetValves(); //ensure all relays are turned off

  //open all valves to plant trays
  digitalWrite(Valve1, LOW);
  digitalWrite(Valve2, LOW);

  //suck any and all water out of the trays
  delay(200);
  Serial.println(" engaging motor");
  digitalWrite(relay7, LOW);
  delay(pumpRuntime);
  digitalWrite(relay7, HIGH);
  delay(200);
  digitalWrite(Valve1, HIGH);
  digitalWrite(Valve2, HIGH);

  //expel drained fluids to drain
  digitalWrite(DrainValve, LOW);
  digitalWrite(relay7, LOW);
  delay(pumpRuntime);
  delay(pumpRuntime);
  digitalWrite(DrainValve, HIGH);
}
