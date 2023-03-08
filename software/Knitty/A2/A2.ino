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
    attachInterrupt(digitalPinToInterrupt(PIN_B), interruptCalibrate, CHANGE);
}

unsigned char patternLength = 20;
signed int firstNeedle = 20;
int knitPattern[255] = {0};

void loop() {
    parserSerialStream();
}

/// serial ///

#define COM_CMD_PATTERN      'P'
#define COM_CMD_FIRST_NEEDLE 'F'  //first needle of pattern from right
#define COM_CMD_SEPERATOR    ':'
#define COM_CMD_RESPONSE     'R'
#define COM_CMD_PLOAD_END    '\n'

// Parser states
#define COM_PARSE_CMD      0x01
#define COM_PARSE_SEP      0x02
#define COM_PARSE_PLOAD    0x03

unsigned char parserState = COM_PARSE_CMD;
unsigned char parserReceivedCommand = 0;
String parserReceivedPayload = "";

void executeCommand(unsigned char cmd, String payload) {
    switch (cmd) {
    case COM_CMD_PATTERN:
        sendCommand(COM_CMD_RESPONSE, "PatternLength: " + String(patternLength));
        break;

    case COM_CMD_FIRST_NEEDLE:
        firstNeedle = payload.toInt();
        sendCommand(COM_CMD_RESPONSE, "FirstNeedle: " + String(firstNeedle));
        break;
    }
}

void sendCommand(unsigned char cmd, String payload) {
    Serial.write(cmd);
    Serial.write(COM_CMD_SEPERATOR);
    Serial.print(payload);
    Serial.write("\n");
}

void parserSerialStream() {

    if (Serial.available() == 0) {
        return;
    }

    char buffer = Serial.read();

    switch (parserState) {

    case COM_PARSE_CMD:
        parserState = COM_PARSE_SEP;
        parserReceivedCommand = buffer;
        parserReceivedPayload = "";
        if (buffer == COM_CMD_PATTERN) {
            patternLength = 0;
        }

        break;

    case COM_PARSE_SEP:

        // We're awaiting a seperator here, if not, back to cmd
        if (buffer == COM_CMD_SEPERATOR) {
            parserState = COM_PARSE_PLOAD;
            break;
        }

        parserState = COM_PARSE_CMD;
        break;

    case COM_PARSE_PLOAD:

        if (buffer == COM_CMD_PLOAD_END) {

            executeCommand(parserReceivedCommand, parserReceivedPayload);
            parserState = COM_PARSE_CMD;

            sendCommand(COM_CMD_RESPONSE, "Received");
            break;
        }

        if (parserReceivedCommand == COM_CMD_PATTERN) {
            //Change state because the E6000 set the needle at '0'
            knitPattern[patternLength] = (buffer == '0') ? 1 : 0;

            patternLength += 1;
        }
        else {
            parserReceivedPayload += buffer;
        }

        break;
    }
}


///

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

//    autoCalibrate(pinA, pinB);

//    pinB = digitalRead(PIN_B);

    digitalWrite(PIN_NEEDLE_RTL, 1);
    digitalWrite(PIN_NEEDLE_LTR, 1);

    if (pinB == HIGH) {
        if (direction == RTL) {
            int needle = - 27 + (currentCursorPosition / 2);
            if (needle >= firstNeedle && needle < (firstNeedle + patternLength)) {
                unsigned int index = patternLength - (needle - firstNeedle) - 1;
                digitalWrite(PIN_NEEDLE_LTR, knitPattern[index]);
            }
            if (needle == firstNeedle + patternLength) {
                Serial.print("E:1\n");
            }
        } else {
            int needle = - 27 + ((currentCursorPosition + 23) / 2);
            if (needle >= firstNeedle && needle < (firstNeedle + patternLength)) {
                unsigned int index = patternLength - (needle - firstNeedle) - 1;
                digitalWrite(PIN_NEEDLE_RTL, knitPattern[index]);
            }
            if (needle == firstNeedle) {
                Serial.print("E:1\n");
            }
        }
    }
}

///
/// interrupt
char clastCSENSE = 0;
char clastCREF = 0;

void interruptCalibrate() {
    unsigned long now = micros();

    char CSENSE = digitalRead(PIN_A);
    char CREF = digitalRead(PIN_B);

    if (CSENSE == clastCSENSE && CREF == clastCREF) {
        return;
    }

    clastCREF = CREF;
    clastCSENSE = CSENSE;

    autoCalibrate(CSENSE, CREF);
}

int passapCalibrateArray[8] = {0};
signed int  passapCalibratePointer = 0;
static unsigned char passaptestpattern[8] = {  1, 1, 0, 1, 1, 1, 0, 1};

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
        Serial.print("R:fc\n");
    }

    passapCalibratePointer = passapCalibratePointer + 2;
    if (passapCalibratePointer >= 8) {
        passapCalibratePointer = 0;
    }

}
