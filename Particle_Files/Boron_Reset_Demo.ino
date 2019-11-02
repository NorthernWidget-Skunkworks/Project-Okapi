SYSTEM_MODE(MANUAL);

void setup() {
    Serial.begin(9600);
    // pinMode(D4, INPUT);
    // delay(3000);
    pinMode(D4, OUTPUT);
    digitalWrite(D4, LOW);
}

void loop() {
	delay(60000);
    Serial.println("BANG");
    digitalWrite(D4, HIGH);
    delay(1000);
    digitalWrite(D4, LOW);
    delay(1000);
}