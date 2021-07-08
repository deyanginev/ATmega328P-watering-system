

// the setup function runs once when you press reset or power the board
void setup() {
	pinMode(8, OUTPUT);
	digitalWrite(8, HIGH);

}

// the loop function runs over and over again until power down or reset
void loop() {
	delay(5000);
	digitalWrite(8, LOW);
	delay(5000);
	digitalWrite(8, HIGH);
}
