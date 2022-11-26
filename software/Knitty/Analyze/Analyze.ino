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

void interruptPinChangeEncoder() {
    unsigned long now = micros();

    Serial.print("S:");
    Serial.print(String(now));
    Serial.print(" ");
    Serial.print(String(digitalRead(PIN_CSENSE)));
    Serial.print(" ");
    Serial.print(String(digitalRead(PIN_CREF)));
    Serial.println("");
}
