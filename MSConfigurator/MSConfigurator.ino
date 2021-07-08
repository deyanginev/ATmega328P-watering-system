
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

SoftwareSerial serial(7, 8);

void setup() {
	serial.begin(38400);
	Serial.begin(9600);
}

void loop() {
	Serial.println("Writing...");
	//size_t bytes = serial.write("setPCI:2000;setSID:10000;setSIW:20000;setAPV:90;setDAPV:100;\n");
	if (serial.available()) {
		size_t bytes = serial.write("setAPV:90;setDAPV:100;\n");
		Serial.print("Written: ");
		Serial.println(bytes);
	}


	delay(5000);
}