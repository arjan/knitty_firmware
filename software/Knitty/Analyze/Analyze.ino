// analyze

#define PIN_CSENSE  2         // Yellow
#define PIN_CREF    3         // White

void setup() {
    Serial.begin(115200);

    // Setup PHOTO SENSOR pin change interrupt
    pinMode(PIN_CSENSE, INPUT_PULLUP);
    pinMode(PIN_CREF,  INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PIN_CSENSE), interruptPinChangeEncoder, CHANGE);

}

void loop() {
}

#define DIRECTION_LEFT_RIGHT   -1
#define DIRECTION_RIGHT_LEFT    1

signed int currentCursorPosition = 0;
unsigned long lastCursorChange = 0;
char lastCSENSE = 0;
char lastCREF = 0;
int currentDirection = 0;

void interruptPinChangeEncoder() {
    unsigned long now = micros();

    if (now - lastCursorChange < 1000) {
        lastCursorChange = now;
        return;
    }

    char CSENSE = digitalRead(PIN_CSENSE);
    char CREF = digitalRead(PIN_CREF);

    if (CSENSE == lastCSENSE && CREF == lastCREF) {
        return;
    }

    if (now - lastCursorChange > 50000) {
        if (CSENSE == CREF) {
            currentDirection = DIRECTION_LEFT_RIGHT;
        } else {
            currentDirection = DIRECTION_RIGHT_LEFT;
        }
        Serial.print("D:");
        Serial.println(String(currentDirection));
    }

    if (CREF) {
        currentCursorPosition += currentDirection;
        Serial.print("C:");
        Serial.println(String(currentCursorPosition));
    }

    lastCSENSE = CSENSE;
    lastCREF = CREF;

    lastCursorChange = now;

    Serial.print("S:");
    Serial.print(String(now));
    Serial.print(" ");
    Serial.print(String(CSENSE!=0));
    Serial.print(" ");
    Serial.print(String(CREF!=0));
    Serial.println("");

    autoCalibrate(CSENSE, CREF);
}

int passapCalibrateArray[8] = {0};
signed int  passapCalibratePointer = 0;
static unsigned char passaptestpattern[8] = {  1, 0, 1, 1, 1, 0, 0, 1};

void autoCalibrate(char CSENSE, char CREF) {
    //AutoCalibrate
    passapCalibrateArray[passapCalibratePointer & 0x07] = CSENSE;
    passapCalibrateArray[(passapCalibratePointer + 1) & 0x07] = CREF;

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
        Serial.print("C:");
        Serial.println(String(currentCursorPosition));
    }

    passapCalibratePointer = passapCalibratePointer + 2;
    if (passapCalibratePointer >= 8) {
        passapCalibratePointer = 0;
    }

}
