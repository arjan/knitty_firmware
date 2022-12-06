// analyze
// #define DEBUG_CSENSE

#define PIN_CSENSE  2         // Yellow
#define PIN_CREF    3         // White
#define PIN_NEEDLE_RTL 5      // Blue,  Pattern RTL
#define PIN_NEEDLE_LTR 6      // ,  Pattern LTR

#define DIRECTION_LEFT_RIGHT   -1
#define DIRECTION_RIGHT_LEFT    1

signed int currentCursorPositionLast = 0;
signed int currentCursorPosition = 0;
unsigned long lastCursorChange = 0;
char lastCSENSE = 0;
char lastCREF = 0;
char currentDirection = 0;

//////////////////////////////////////////////////////////////////////////////
// Knitty Serial Protocol

// Receive commands
#define COM_CMD_PATTERN      'P'
#define COM_CMD_PATTERN_BACK 'B'
#define COM_CMD_PATTERN_END  'E'
#define COM_CMD_CURSOR       'C'
#define COM_CMD_RESPONSE     'R'
#define COM_CMD_DIRECTION    'D'
#define COM_CMD_DEBUG        'V'
#define COM_CMD_NEW_PATTERN  'N'
#define COM_CMD_FIRST_NEEDLE 'F'  //first needle of pattern from right
#define COM_CMD_SEPERATOR    ':'
#define COM_CMD_STATUS       'T'

#define COM_CMD_SERVO        'S'

#define COM_CMD_PLOAD_END    '\n'

// Parser states
#define COM_PARSE_CMD      0x01
#define COM_PARSE_SEP      0x02
#define COM_PARSE_PLOAD    0x03


unsigned char parserState = COM_PARSE_CMD;

unsigned char parserReceivedCommand = 0;
String parserReceivedPayload = "";

// pattern

unsigned char patternLength = 0;

unsigned int currentPatternIndex = 0;
signed int firstNeedle = 0;
signed int offsetCarriage_RTL = 33;
signed int offsetCarriage_LTR = 33 + 22;

int knitPattern[255] = {0};


void setup() {
    Serial.begin(115200);

    // Setup PHOTO SENSOR pin change interrupt
    pinMode(PIN_CSENSE, INPUT_PULLUP);
    pinMode(PIN_CREF,  INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PIN_CSENSE), interruptPinChangeEncoder, CHANGE);


    // Setup Needles
    pinMode(PIN_NEEDLE_RTL, OUTPUT);
    digitalWrite(PIN_NEEDLE_RTL, HIGH);
    pinMode(PIN_NEEDLE_LTR, OUTPUT);
    digitalWrite(PIN_NEEDLE_LTR, HIGH);

}



void loop() {
    parserSerialStream();
}


void executeCommand(unsigned char cmd, String payload) {
    switch (cmd) {
    case COM_CMD_PATTERN:
        sendCommand(COM_CMD_RESPONSE, "PatternLength: " + String(patternLength));
        break;

    case COM_CMD_PATTERN_BACK:
        sendCommand(COM_CMD_RESPONSE, "Back pattern not implemented");
        break;

    case COM_CMD_CURSOR:
        currentCursorPosition = payload.toInt();
        break;

    case COM_CMD_FIRST_NEEDLE:
        firstNeedle = payload.toInt() * 2 - 2;
        sendCommand(COM_CMD_RESPONSE, "FirstNeedle: " + String(firstNeedle));
        break;

    case COM_CMD_STATUS:
        sendCommand(COM_CMD_RESPONSE, "-------------------------");
        sendCommand(COM_CMD_RESPONSE, "Cursorposition: " + String(currentCursorPosition));
        sendCommand(COM_CMD_RESPONSE, "PatternIndex: " + String(currentPatternIndex) );
        if (currentDirection == DIRECTION_RIGHT_LEFT) {
            sendCommand(COM_CMD_DIRECTION, "RTL");
        }
        else {
            sendCommand(COM_CMD_DIRECTION, "LTR");
        }
        sendCommand(COM_CMD_RESPONSE, "-------------------------");
        break;

    case COM_CMD_SERVO:
        sendCommand(COM_CMD_RESPONSE, "Servo commands not implemented");
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


void setNeedle_RTL(char state) {
    //Attention E6000 sets needle by 0
    digitalWrite(PIN_NEEDLE_RTL, state);
}

void setNeedle_LTR(char state) {
    //Attention E6000 sets needle by 0
    digitalWrite(PIN_NEEDLE_LTR, state);
}

void patternFront() {
    //RTL
    if (currentDirection == DIRECTION_RIGHT_LEFT) {
        int patternPositionRTL = currentCursorPosition  - (firstNeedle + offsetCarriage_RTL);
        //set needles in absolute position
        if (patternPositionRTL > 0 && patternPositionRTL <= patternLength * 2) {
            setNeedle_RTL(knitPattern[((patternLength * 2 - patternPositionRTL) / 2)]);
        }
        else {
            setNeedle_RTL(1);
        }
        //send pattern End
        if (patternPositionRTL == patternLength * 2 + 2 ) {
            sendCommand(COM_CMD_PATTERN_END, "1");
        }
    }

    //LTR
    if (currentDirection == DIRECTION_LEFT_RIGHT) {

        int patternPositionLTR = currentCursorPosition  - (firstNeedle + offsetCarriage_LTR);
        if (patternPositionLTR > 0 && patternPositionLTR <= patternLength * 2) {
            setNeedle_LTR(knitPattern[((patternLength * 2 - patternPositionLTR) / 2)]);
        }
        else {
            setNeedle_LTR(1);
        }
        //send pattern End
        if (patternPositionLTR == 0) {
            sendCommand(COM_CMD_PATTERN_END, "1");
        }
    }
}


/// interrupt

void interruptPinChangeEncoder() {
    unsigned long now = micros();

    if (now - lastCursorChange < 1000) {
//        lastCursorChange = now;
        return;
    }

    char CSENSE = digitalRead(PIN_CSENSE);
    char CREF = digitalRead(PIN_CREF);

    if (CSENSE == lastCSENSE && CREF == lastCREF) {
        return;
    }

    if (now - lastCursorChange >= 100) {
        char direction;
        if (CSENSE == CREF) {
            direction = DIRECTION_LEFT_RIGHT;
        } else {
            direction = DIRECTION_RIGHT_LEFT;
        }
        if (direction != currentDirection) {
            currentDirection = direction;
            Serial.print("D:");
            Serial.println(String(0+currentDirection));
        }
    }

//    if (CREF) {
    currentCursorPosition -= currentDirection;

    /* if (currentDirection == DIRECTION_RIGHT_LEFT) { */
    /*     setNeedle_RTL(0); */
    /*     setNeedle_LTR(1); */
    /* } */
    /* else { */
    /*     setNeedle_RTL(1); */
    /*     setNeedle_LTR(1); */
    /* } */

    if (CSENSE) {
    patternFront();
    }
//        setNeedle_RTL(1);
//        setNeedle_LTR(1);

//    }

    lastCSENSE = CSENSE;
    lastCREF = CREF;
    lastCursorChange = now;

#ifdef DEBUG_CSENSE
    Serial.print("S:");
    Serial.print(String(now));
    Serial.print(" ");
    Serial.print(String(CSENSE!=0));
    Serial.print(" ");
    Serial.print(String(CREF!=0));
    Serial.println("");
#endif

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
        sendCommand(COM_CMD_RESPONSE, "fc");
//        Serial.print("C:");
//        Serial.println(String(currentCursorPosition));
    }

    passapCalibratePointer = passapCalibratePointer + 2;
    if (passapCalibratePointer >= 8) {
        passapCalibratePointer = 0;
    }

}
