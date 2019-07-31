SYSTEM_MODE(MANUAL);

void setup() {
    Serial.begin(9600);
    pinMode(D7, OUTPUT);
}

void loop() {
    Serial.println("BANG");
    digitalWrite(D7, HIGH);
    delay(1000);
    digitalWrite(D7, LOW);
    delay(1000);
}