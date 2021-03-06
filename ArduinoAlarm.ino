// Constant values for the secret code and user code
const byte constBitValueLow = 0;
const byte constBitValueHigh = 1;
const byte constBitValueNone = 99;

int ledPins[] = {11, 12};
int ledPinCount = 2;
int buttonPins[] = {2, 3};
int buttonPinCount = 2;
int wrongCodePin = 13;
int rightCodePin = 10;
int soundPin = 4;
const byte constUserInputActions[] = {constBitValueLow, constBitValueHigh};
byte prevButtonStates[] = {LOW, LOW};
byte currButtonStates[] = {LOW, LOW};

unsigned long constAlarmTimeMs = 100;
unsigned long currTimeMs = 0;
unsigned long constSnoozeDurationMs = 10000;

volatile bool ledDisplayFlag = false;
volatile byte alarmLedDisplayCounter = 0;

// The code is generated by taking the binary representation
// of a 6 bit int value. Thus the max allowed value is 2^6-1
const int constMaxCodeSize = 6;
byte secretCode[constMaxCodeSize];
byte userCode[constMaxCodeSize] = {constBitValueNone, constBitValueNone, constBitValueNone, constBitValueNone, constBitValueNone, constBitValueNone};

int constLedWaitDelay = 400;
int constLedDisplayDelay = 2000;

volatile bool isAlarmOff = false;
bool isAlarmRunning = false;

int soundSequence[constMaxCodeSize]; 

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // Set up the led pins as outputs
  for (int i = 0; i < ledPinCount; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  // Set up feedback pins
  pinMode(wrongCodePin, OUTPUT);
  digitalWrite(wrongCodePin, LOW);
  pinMode(rightCodePin, OUTPUT);
  digitalWrite(rightCodePin, LOW);

  // Set up the buttons as inputs
  for (int i = 0; i < buttonPinCount; i++) {
    pinMode(buttonPins[i], INPUT);
    attachInterrupt(digitalPinToInterrupt(buttonPins[i]), buttonInterrupt, CHANGE);
  }

  // Set up pin for sound
  pinMode(soundPin, OUTPUT);
  noTone(soundPin);
  
  randomSeed(analogRead(0) + analogRead(1) + analogRead(2) + analogRead(3) + analogRead(4));
}

void loop() {
  // put your main code here, to run repeatedly:
  // Save the previous button states
  prevButtonStates[0] = currButtonStates[0];
  prevButtonStates[1] = currButtonStates[1];

  currButtonStates[0] = digitalRead(buttonPins[0]);
  currButtonStates[1] = digitalRead(buttonPins[1]);

  // Get current time
  currTimeMs = millis();

  // Check if the alarm needs to ring
  if (shouldAlarmRing(currTimeMs, constAlarmTimeMs)) {

    if (!isAlarmRunning) {
      Serial.println("Entered the alarm state!");
      isAlarmRunning = true;
      generateCode(secretCode);
      generateSoundSequence(soundSequence);
      Serial.println("The secret code is:");
      printArrayForDebugging(secretCode);
      setupInterruptTimer1();
      enableInterruptTimer1();
    }

    bool isPressedButton0 = ((currButtonStates[0] == LOW) && (prevButtonStates[0] == HIGH));
    bool isPressedButton1 = ((currButtonStates[1] == LOW) && (prevButtonStates[1] == HIGH));
    int userPressedButton;

    if ((isPressedButton0) || (isPressedButton1)) {
      if (isPressedButton0) {
        debounce(buttonPins[0]);
        userPressedButton = 0;
      } else {
        debounce(buttonPins[1]);
        userPressedButton = 1;
      }
      
      byte userAction = addUserButtonPressToUserCode(userPressedButton, userCode);

      if (userAction == constBitValueHigh) {

        if (isUserCodeCorrect(userCode, secretCode)) {
          Serial.println("Alarm turned off!");
          isAlarmOff = true;
          disableInterruptTimer1();
          digitalWrite(rightCodePin, HIGH);
          digitalWrite(wrongCodePin, LOW);
          noTone(soundPin);
        } else {
          Serial.println("Not quite, try again!");
          clearUserCode(userCode);
          digitalWrite(rightCodePin, LOW);
          digitalWrite(wrongCodePin, HIGH);
        }
      } else {
        digitalWrite(wrongCodePin, LOW);
        digitalWrite(rightCodePin, LOW);
      }
      
      printArrayForDebugging(userCode);
    }

  } else {
    setLedOutputsLow();
    isAlarmRunning = false;
    noTone(soundPin);
  }
}

void generateCode(byte * codeArray) {
  int codeValue = random(1 << constMaxCodeSize);

  for (int i = 0; i < constMaxCodeSize; i++) {
    if ((codeValue & (1 << i)) > 0) {
      codeArray[i] = constBitValueHigh;
    } else {
      codeArray[i] = constBitValueLow;
    }
  }
}

void printArrayForDebugging(byte * arrayToPrint) {
  for (int i = 0; i < constMaxCodeSize; i++) {
    Serial.print(arrayToPrint[i]);
    Serial.print(" ");
  }

  Serial.println();
}

bool shouldAlarmRing(unsigned long currTime, unsigned long alarmTime) {
  return shouldAlarmRing(currTime, alarmTime, false, constSnoozeDurationMs);
}

bool shouldAlarmRing(unsigned long currTime, unsigned long alarmTime, bool isSnoozed, unsigned long snoozeDuration) {
  if (isAlarmOff) {
    return false;
  }
  
  if (!isSnoozed) {
    return (currTime >= alarmTime);
  }

  return (currTime >= (alarmTime + snoozeDuration));
}

void displayCodeOnLeds() {
  int signals[2];
  
  setLedOutputsHigh();
  delay(constLedDisplayDelay);
  
  for (int i = (constMaxCodeSize - 1); i >= 0; i--) {
    setLedOutputsLow();
    delay(constLedWaitDelay);
    
    //if (((1 << i) & secretCode) == 0) {
    if (secretCode[i] == 0) {
      signals[0] = HIGH;
      signals[1] = LOW;
    } else {
      signals[0] = LOW;
      signals[1] = HIGH;
    }

    Serial.print("Next value is: ");
    //Serial.println(((1 << i) & secretCode));

    setLedOutputs(signals);
    delay(constLedDisplayDelay);
  }
  
  setLedOutputsLow();
  delay(constLedWaitDelay);
}

void setLedOutputs(int ledPinSignals[]) {
  for (int i = 0; i < ledPinCount; i++) {
    digitalWrite(ledPins[i], ledPinSignals[i]);
  }
}

void setLedOutputsLow() {
  int lowSignals[] = {LOW, LOW};
  setLedOutputs(lowSignals);
}

void setLedOutputsHigh() {
  int highSignals[] = {HIGH, HIGH};
  setLedOutputs(highSignals);
}

void setLedOutputsHighLow() {
  int signals[] = {HIGH, LOW};
  setLedOutputs(signals);
}

void setLedOutputsLowHigh() {
  int signals[] = {LOW, HIGH};
  setLedOutputs(signals);
}

void buttonInterrupt() {
  for (int i = 0; i < buttonPinCount; i++) {
    if (digitalRead(buttonPins[i]) == LOW) {
      return;
    }
  }
  
  isAlarmOff = true;
}

int calcValueForTimer(float timeInSeconds) {
  float clockSpeed = 16000000;
  float prescaler = 1024;

  return (int) (((clockSpeed * timeInSeconds) / prescaler) - 1);
}

void setupInterruptTimer1() {
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = calcValueForTimer(0.5); // = ((16*10^6) / (1*1024)) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
}

void enableInterruptTimer1() {
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
}

void disableInterruptTimer1() {
  TIMSK1 &= ~(1 << OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
  ledDisplayFlag = !ledDisplayFlag;

  if (ledDisplayFlag) {
    if (alarmLedDisplayCounter == constMaxCodeSize) {
      setLedOutputsHigh();
      alarmLedDisplayCounter = 0;
      noTone(soundPin);
    } else {
      tone(soundPin, soundSequence[alarmLedDisplayCounter]);
      if (secretCode[alarmLedDisplayCounter] == 0) {
        setLedOutputsHighLow();
      } else {
        setLedOutputsLowHigh();
      }

      alarmLedDisplayCounter++;
    }

  } else {
    setLedOutputsLow();
  }
}

void debounce(int pinToDebounce) {
  do {
    delay(50);
  } while (digitalRead(pinToDebounce) == HIGH);
}

int whichButtonPressed() {
  for (int i = 0; i < buttonPinCount; i++) {
    if (digitalRead(buttonPins[i]) == HIGH) {
      return i;
    }
  }

  return -1;
}

byte addUserButtonPressToUserCode(int buttonPressed, byte * userCode) {
  int i = 0;

  while (userCode[i] != constBitValueNone) {
    if (i == constMaxCodeSize) {
      return constBitValueNone;
    }

    i++;
  }

  userCode[i] = constUserInputActions[buttonPressed];

  if (i == (constMaxCodeSize - 1)) {
    return constBitValueHigh;
  }

  return constBitValueLow;
}

void clearUserCode(byte * userCode) {
  for (int i = 0; i < constMaxCodeSize; i++) {
    userCode[i] = constBitValueNone;
  }
}

bool isUserCodeCorrect(byte * userCode, byte * secretCode) {
  for (int i = 0; i < constMaxCodeSize; i++) {
    if (userCode[i] != secretCode[i]) {
      return false;
    }
  }

  return true;
}

void generateSoundSequence(int * soundCode) {
  for (int i = 0; i < constMaxCodeSize; i++) {
    soundCode[i] = 200 + random(500);
  }
}
