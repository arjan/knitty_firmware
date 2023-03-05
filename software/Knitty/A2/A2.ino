#define PIN_A  3         // Yellow
#define PIN_B    2         // White

#define PIN_NEEDLE_RTL 5      // Blue,  Pattern RTL
#define PIN_NEEDLE_LTR 6      // ,  Pattern LTR

void setup() {
    Serial.begin(115200);

    pinMode(PIN_NEEDLE_LTR, OUTPUT);
    pinMode(PIN_NEEDLE_RTL, OUTPUT);

    pinMode(PIN_A, INPUT_PULLUP);
    pinMode(PIN_B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PIN_A), interruptA, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_B), interruptB, CHANGE);
}

void loop() {
}

char pinA = LOW, pinB = LOW;
unsigned long now, lastInterrupt, currentCursorPosition = 0;

#define MIN_INTERRUPT 1000
#define LTR 1
#define RTL 2


char direction;

void interruptA() {
    now = micros();
    if (now - lastInterrupt < MIN_INTERRUPT) {
//        Serial.println("Too fast!");
        return;
    }

    char a = digitalRead(PIN_A);
    if (a == pinA) {
//        Serial.println("Same!");
        return;
    }

    pinA = a;

    pinB = digitalRead(PIN_B);

    if (pinB != pinA) {
        currentCursorPosition++;
        direction = RTL;
    } else {
        currentCursorPosition--;
        direction = LTR;
    }
    Serial.print("Position: ");
    Serial.println(currentCursorPosition);

//    autoCalibrate(pinA, pinB);

//    pinB = digitalRead(PIN_B);

    if (direction == RTL) {
        int needle = currentCursorPosition / 2;

        digitalWrite(PIN_NEEDLE_RTL, 1);
        digitalWrite(PIN_NEEDLE_LTR, 1);
//        return;

        if (pinB == LOW) return;

        if (needle >= 30 && needle <= 50) {
            digitalWrite(PIN_NEEDLE_LTR, (needle / 3) % 2);
        }
    } else {
        int needle = (currentCursorPosition + 25) / 2;

        digitalWrite(PIN_NEEDLE_RTL, 1);
        digitalWrite(PIN_NEEDLE_LTR, 1);

        if (pinB == HIGH) return;

        if (needle >= 30 && needle <= 50) {
            digitalWrite(PIN_NEEDLE_RTL, (needle / 3) % 2);
        }
    }
}

void interruptB() {
}

///

int passapCalibrateArray[8] = {0};
signed int  passapCalibratePointer = 0;
static unsigned char passaptestpattern[8] = {  1, 1, 0, 1, 1, 1, 0, 1};

void autoCalibrate(char CSENSE, char CREF) {
    //AutoCalibrate
    passapCalibrateArray[passapCalibratePointer & 0x07] = CSENSE;
    passapCalibrateArray[(passapCalibratePointer + 1) & 0x07] = CREF;

    Serial.print(String(CSENSE!=0));
    Serial.print(" ");
    Serial.print(String(CREF!=0));
    Serial.println(" ");


    // finding the calibration sequence -> store in 'passaptestpattern'
    int found = 1;
    for (int i = 0; i < 8; i++) {
        if ((passapCalibratePointer - i) >= -1) {
            if (passapCalibrateArray[(passapCalibratePointer - i + 1) & 0x07] !=
                passaptestpattern[i]) {
                found = 0;
                break;
            }
        }
        else {
            if (passapCalibrateArray[(passapCalibratePointer - i + 9) & 0x07] !=
                passaptestpattern[i]) {
                found = 0;
                break;
            }
        }
    }

    // Serial.println("end");
    if (found) {
        currentCursorPosition = 0;
        Serial.println("R:fc");
    }

    passapCalibratePointer = passapCalibratePointer + 2;
    if (passapCalibratePointer >= 8) {
        passapCalibratePointer = 0;
    }

}
