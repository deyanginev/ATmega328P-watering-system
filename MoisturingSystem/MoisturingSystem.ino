
#include "ActionsList.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/Org_01.h>

#ifdef __AVR_ATmega328P__ || __AVR_ATmega168__
#define ARDUINO_ARCH_UNO
#endif

#define FONT_BASELINE_CORRECTION_NORMAL 6
#define FONT_BASELINE_CORRECTION_LARGE 12
#define MS_FONT_TEXT_SIZE_NORMAL 1
#define MS_FONT_TEXT_SIZE_LARGE 2

const char* MS_OFF_STRING = "OFF";
const char* MS_ON_STRING = "ON";
const char* MS_BACK_BUTTON_PROMPT = "B1 - Back";
const char* MS_READING_PROMPT_TEXT = "Reading...";

const char* MS_FAR_ACTIVE_SETTING_KEY = "far-active";
const char* MS_MID_ACTIVE_SETTING_KEY = "mid-active";
const char* MS_NEAR_ACTIVE_SETTING_KEY = "near-active";

const char* MS_FAR_WET_SETTING_KEY = "wet-far";
const char* MS_MID_WET_SETTING_KEY = "wet-mid";
const char* MS_NEAR_WET_SETTING_KEY = "wet-near";

const char* MS_FAR_DRY_SETTING_KEY = "dry-far";
const char* MS_MID_DRY_SETTING_KEY = "dry-mid";
const char* MS_NEAR_DRY_SETTING_KEY = "dry-near";

const char* MS_APV_SETTING_KEY = "apv";
const char* MS_DAPV_SETTING_KEY = "dapv";

const char* MS_PUMP_MAX_DURATION = "p-max";
const char* MS_PUMP_REACT_INT_DURATION = "p-react";

const char* MS_SENSOR_INTERVAL_DRY = "s-dry";
const char* MS_SENSOR_INTERVAL_PUMPING = "s-pumping";


#ifdef ARDUINO_ARCH_ESP32

#include <Preferences.h>
#define MS_PREFERENCES_ID "msys"

#else
#include <EEPROM.h>
#define _max max
#define _min min
#endif

#define MS_BUTTON1 1
#define MS_BUTTON2 2
#define MS_BUTTON3 3
#define MS_BUTTON4 4

#define SCREENS_COUNT 8

#define MS_HOME_SCREEN 0
#define MS_SETTINGS_SCREEN 1
#define MS_SENSOR_SETTINGS_SCREEN 2
#define MS_THRESHOLDS_SETTINGS_SCREEN 3
#define MS_INTERVALS_SETTINGS_SCREEN 4
#define MS_PUMP_INTERVALS_SETTINGS_SCREEN 5
#define MS_SENSOR_INTERVALS_SETTINGS_SCREEN 6
#define MS_STATS_SCREEN 7

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define DEFAULT_TEXT_SIZE 2

#define DEFAULT_FONT Org_01

#define MS_H_CENTER 1
#define MS_H_LEFT 2
#define MS_H_RIGHT 4
#define MS_V_CENTER 8
#define MS_V_TOP 16
#define MS_V_BOTTOM 32


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#ifdef ARDUINO_ARCH_ESP32

#define BUTTON_1_LOW 500
#define BUTTON_1_HIGH 1700

#define BUTTON_2_LOW 2000
#define BUTTON_2_HIGH 2400

#define BUTTON_3_LOW 2900
#define BUTTON_3_HIGH 3500

#define BUTTON_4_LOW 3800
#define BUTTON_4_HIGH 4500

Preferences preferences;

#define PUMP_PIN 27
#define SENSOR_PIN 14

#define SENSOR_PIN_HIGH LOW
#define PUMP_PIN_HIGH LOW
#define SENSOR_PIN_LOW HIGH
#define PUMP_PIN_LOW HIGH

#ifdef ARDUINO_ARCH_ESP32
#define PIN_FAR 34
#define PIN_MID 35
#define PIN_NEAR 32
#endif

#ifndef ARDUINO_ARCH_ESP32
#define PIN_FAR A0
#define PIN_MID A1
#define PIN_NEAR A2
#endif

#define PIN_VALVE_FAR 33
#define PIN_VALVE_MID 25
#define PIN_VALVE_NEAR 26

#define BUTTONS_PIN 12

#endif
#ifndef ARDUINO_ARCH_ESP32

#define BUTTON_1_LOW 100
#define BUTTON_1_HIGH 280

#define BUTTON_2_LOW 300
#define BUTTON_2_HIGH 450

#define BUTTON_3_LOW 500
#define BUTTON_3_HIGH 600

#define BUTTON_4_LOW 650
#define BUTTON_4_HIGH 1024

#define PUMP_PIN 4
#define SENSOR_PIN 2

#define SENSOR_PIN_HIGH LOW
#define PUMP_PIN_HIGH LOW
#define SENSOR_PIN_LOW HIGH
#define PUMP_PIN_LOW HIGH

#define PIN_FAR A0
#define PIN_MID A1
#define PIN_NEAR A2

#define PIN_VALVE_FAR 10
#define PIN_VALVE_MID 11
#define PIN_VALVE_NEAR 12

#define BUTTONS_PIN A3

#endif

#define SENSORS_COUNT 3

#define MS_SENSOR_FAR  0
#define MS_SENSOR_MID  1
#define MS_SENSOR_NEAR  2

#define ACTIONS_COUNT 6

// action defines
#define SENSORS_ACTION 0
#define OUTLET_FAR_ACTION 1
#define OUTLET_MID_ACTION 2
#define OUTLET_NEAR_ACTION 3
#define PUMP_ACTION 4
#define DRAW_HOME_ACTION 5


// end of action defines

//#undef DEBUG
#define DEBUG


// Structures

struct MSScreenBox {
	int x;
	int y;
	int w;
	int h;
};

struct MSysSettings {
	unsigned long siw = 5000; // 5 seconds for sensor interval while watering
	unsigned long sid = 60000 * 1; // 1 minute for sensor interval while in stand by
	unsigned long sd = 1000; // 1 second max duration for sensor activation;
	unsigned long pi = 60000 * 10; // 10 minutes between pump activations
	unsigned long pd = 60000 * 2; // 2 minutes max pump on duration
	long apv = 50; // % humidity threshold for pump activation
	long dapv = 85; // % humidity threshold for pump deactivation
} settings;

struct Sensor {
	int wet; // completely wet reading
	int dry; // completely dry reading
	int value; // current reading
	int ai; // action index
	int p; // humidity percentage
	bool active = true;
	char name[4];
};

struct SystemState {
	int scr = MS_HOME_SCREEN;
	Sensor* s = nullptr;
	bool p = false;
	bool vn = false;
	bool vm = false;
	bool vf = false;
} state;



// An array used for iterating over scheduled
// actions and executing them
Action* availableActions = nullptr;

ActionsList executionList;

struct MSScreen {
	void (*handleButtons)(int buttonValue);
	void (*drawUI)(Action* a);
	int code;
};


// An array containing the available screens
MSScreen* availableScreens;

struct ButtonState {
	int value = 0;
	bool hasChanged = false;
} button;
// end of structures

// Helpers

// Display

void initCanvas(GFXcanvas16* c) {
	(*c).setFont(&DEFAULT_FONT);
	(*c).setTextColor(SSD1306_WHITE);
}

void printPositionedText(GFXcanvas16* canvas, const char* text, int x, int y) {
	(*canvas).setCursor(x, y);
	(*canvas).print(text);
}

MSScreenBox printAlignedText(GFXcanvas16* canvas, const char* text, int textSize, int align = MS_H_CENTER | MS_V_CENTER) {
	int fontCorrection = textSize == MS_FONT_TEXT_SIZE_NORMAL ? FONT_BASELINE_CORRECTION_NORMAL : FONT_BASELINE_CORRECTION_LARGE;
	int16_t mx, my;
	uint16_t mw, mh;
	(*canvas).setTextSize(textSize);
	(*canvas).getTextBounds(text, 0, 0, &mx, &my, &mw, &mh);
	(*canvas).setTextColor(SSD1306_WHITE);
	int16_t x = 0;
	int16_t y = 0;

	if ((MS_H_CENTER & align) != 0) {
		x = (int16_t)(SCREEN_WIDTH / 2 - mw / 2);
	}
	else if ((MS_H_LEFT & align) != 0) {
		x = 0;
	}
	else if ((MS_H_RIGHT & align) != 0) {
		x = SCREEN_WIDTH - mw;
	}


	if ((MS_V_CENTER & align) != 0) {
		y = (int16_t)(SCREEN_HEIGHT / 2 - mh / 2);
	}
	else if ((MS_V_TOP & align) != 0) {
		y = fontCorrection;
	}
	else if ((MS_V_BOTTOM & align) != 0) {
		y = SCREEN_HEIGHT - (mh + fontCorrection);
	}
	printPositionedText(canvas, text, x, y);

	return { x, y, mw, mh };
}

MSScreenBox printAlignedTextStack(
	GFXcanvas16* canvas,
	char** text,
	int arraySize,
	int textSize,
	int align,
	int boxAlign = MS_H_CENTER | MS_V_CENTER,
	int spacing = 3) {
	int boxHeight = 0;
	int boxWidth = 0;


	int fontCorrection = textSize == MS_FONT_TEXT_SIZE_NORMAL ? FONT_BASELINE_CORRECTION_NORMAL : FONT_BASELINE_CORRECTION_LARGE;
	boxHeight += spacing * (arraySize - 1) + fontCorrection;
	(*canvas).setTextSize(textSize);

	char** cpointer = text;
	for (int i = 0; i < arraySize; i++) {
		int16_t cx, cy;
		uint16_t cw, ch;
		(*canvas).getTextBounds((*cpointer), 0, 0, &cx, &cy, &cw, &ch);
		boxHeight += ch;

		if (cw > boxWidth) {
			boxWidth = cw;
		}
		cpointer++;
	}

	GFXcanvas16 boxCanvas(boxWidth, boxHeight);
	initCanvas(&boxCanvas);
	boxCanvas.setTextSize(textSize);

	cpointer = text;
	int yCoord = fontCorrection; //offset correction due to font
	int xCoord = 0;
	for (int i = 0; i < arraySize; i++) {
		int16_t cx, cy;
		uint16_t cw, ch;
		boxCanvas.getTextBounds((*cpointer), 0, 0, &cx, &cy, &cw, &ch);

		switch (align) {
		case MS_H_LEFT:
			xCoord = boxWidth / 2 - boxWidth / 2;
			break;
		case MS_H_CENTER:
			xCoord = boxWidth / 2 - cw / 2;
			break;
		case MS_H_RIGHT:
			xCoord = boxWidth - cw;
			break;
		}

		printPositionedText(&boxCanvas, (*cpointer), xCoord, yCoord);
		cpointer++;
		yCoord += (ch + spacing);
	}


	int boxX = 0, boxY = 0;
	int screenWidth = (*canvas).width();
	int screenHeight = (*canvas).height();

	if ((MS_H_CENTER & boxAlign) != 0) {
		boxX = (int16_t)(screenWidth / 2 - boxWidth / 2);
	}
	else if ((MS_H_LEFT & boxAlign) != 0) {
		boxX = 0;
	}
	else if ((MS_H_RIGHT & boxAlign) != 0) {
		boxX = screenWidth - boxWidth;
	}


	if ((MS_V_CENTER & boxAlign) != 0) {
		boxY = (int16_t)(screenHeight / 2 - boxHeight / 2);
	}
	else if ((MS_V_TOP & boxAlign) != 0) {
		boxY = 0;
	}
	else if ((MS_V_BOTTOM & boxAlign) != 0) {
		boxY = screenHeight - boxHeight;
	}

	(*canvas).drawRGBBitmap(boxX, boxY, boxCanvas.getBuffer(), boxWidth, boxHeight);

	return { boxX, boxY, boxWidth, boxHeight };;
}

void drawStartingPromptScreen(Adafruit_SSD1306* display) {
	char* prompt[] = { "Actions:", "B1 - start", "B2 - init" };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedTextStack(&canvas, prompt, 3, DEFAULT_TEXT_SIZE, MS_H_LEFT, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void drawDryValuesScreen(Adafruit_SSD1306* display) {
	char nearA[15], midA[15], farA[15];
	sprintf(nearA, "Near: %d", state.s[MS_SENSOR_NEAR].dry);
	sprintf(midA, "Mid: %d", state.s[MS_SENSOR_MID].dry);
	sprintf(farA, "Far: %d", state.s[MS_SENSOR_FAR].dry);

	char* message[] = { "Dry values:", nearA, midA, farA };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedTextStack(&canvas, message, 4, DEFAULT_TEXT_SIZE, MS_H_LEFT, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void drawWetValuesScreen(Adafruit_SSD1306* display) {
	char nearA[15], midA[15], farA[15];
	sprintf(nearA, "Near: %d", state.s[MS_SENSOR_NEAR].wet);
	sprintf(midA, "Mid: %d", state.s[MS_SENSOR_MID].wet);
	sprintf(farA, "Far: %d", state.s[MS_SENSOR_FAR].wet);

	char* message[] = { "Wet values:", nearA, midA, farA };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedTextStack(&canvas, message, 4, DEFAULT_TEXT_SIZE, MS_H_LEFT, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void drawStartingValuesScreen(Adafruit_SSD1306* display) {
	drawDryValuesScreen(display);
	delay(5000);
	drawWetValuesScreen(display);
	delay(5000);

	(*display).clearDisplay();
	char apv[15], dapv[15];
	sprintf(apv, "APV: %d%%", settings.apv);
	sprintf(dapv, "DAPV: %d%%", settings.dapv);

	char* message[] = { "Thresholds:", apv, dapv };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedTextStack(&canvas, message, 3, DEFAULT_TEXT_SIZE, MS_H_LEFT, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void showTextCaptionScreen(Adafruit_SSD1306* display, const char* caption) {
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedText(&canvas, caption, 2, MS_V_CENTER | MS_H_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void showActionPromptScreen(Adafruit_SSD1306* display, char* btn, char* action) {
	char btnstr[10];
	sprintf(btnstr, "Press %s to", btn);
	char* message[] = { btnstr, action };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedTextStack(&canvas, message, 3, DEFAULT_TEXT_SIZE, MS_H_CENTER, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void drawSplashScreen(Adafruit_SSD1306* display) {
	char* texts[] = { "Irrigation", "System", "Version: 0.1" };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	canvas.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 10, SSD1306_WHITE);
	printAlignedTextStack(&canvas, texts, 3, DEFAULT_TEXT_SIZE, MS_H_CENTER, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

// end of Display

void populateScreens() {
	availableScreens[MS_HOME_SCREEN].drawUI = &drawHomeScreen;
	availableScreens[MS_HOME_SCREEN].handleButtons = &handleHomeScreen;

	availableScreens[MS_SETTINGS_SCREEN].drawUI = &drawSettingsScreen;
	availableScreens[MS_SETTINGS_SCREEN].handleButtons = &handleSettingsScreen;

	availableScreens[MS_SENSOR_SETTINGS_SCREEN].drawUI = &drawSensorSettingsScreen;
	availableScreens[MS_SENSOR_SETTINGS_SCREEN].handleButtons = &handleSensorSettingsScreen;

	availableScreens[MS_THRESHOLDS_SETTINGS_SCREEN].drawUI = &drawThresholdsSettingsScreen;
	availableScreens[MS_THRESHOLDS_SETTINGS_SCREEN].handleButtons = &handleThresholdsSettingsScreen;

	availableScreens[MS_INTERVALS_SETTINGS_SCREEN].drawUI = &drawIntervalsSettingsScreen;
	availableScreens[MS_INTERVALS_SETTINGS_SCREEN].handleButtons = &handleIntervalsSettingsScreen;

	availableScreens[MS_PUMP_INTERVALS_SETTINGS_SCREEN].drawUI = &drawPumpIntervalsSettingsScreen;
	availableScreens[MS_PUMP_INTERVALS_SETTINGS_SCREEN].handleButtons = &handlePumpIntervalsSettingsScreen;

	availableScreens[MS_SENSOR_INTERVALS_SETTINGS_SCREEN].drawUI = &drawSensorIntervalsSettingsScreen;
	availableScreens[MS_SENSOR_INTERVALS_SETTINGS_SCREEN].handleButtons = &handleSensorIntervalsSettingsScreen;

	availableScreens[MS_STATS_SCREEN].drawUI = &drawStatisticsScreen;
	availableScreens[MS_STATS_SCREEN].handleButtons = &handleStatisticsScreen;
}

void populateActions() {


	// read sensors action
	availableActions[SENSORS_ACTION].tick = &tickSensors;
	availableActions[SENSORS_ACTION].frozen = true;
	availableActions[SENSORS_ACTION].stopRequested = false;
	availableActions[SENSORS_ACTION].start = &startSensors;
	availableActions[SENSORS_ACTION].stop = &stopSensors;
	availableActions[SENSORS_ACTION].ti = settings.siw;
	availableActions[SENSORS_ACTION].td = settings.sd;
	availableActions[SENSORS_ACTION].to = 200;
	availableActions[SENSORS_ACTION].clear = false;
	availableActions[SENSORS_ACTION].state = MS_NON_ACTIVE;
	availableActions[SENSORS_ACTION].child = nullptr;
	availableActions[SENSORS_ACTION].lst = 0;
	availableActions[SENSORS_ACTION].st = 0;

	// open far valve action
	availableActions[OUTLET_FAR_ACTION].tick = &tickFarOutlet;
	availableActions[OUTLET_FAR_ACTION].frozen = false;
	availableActions[OUTLET_FAR_ACTION].stopRequested = false;
	availableActions[OUTLET_FAR_ACTION].start = &startFarOutlet;
	availableActions[OUTLET_FAR_ACTION].stop = &stopFarOutlet;
	availableActions[OUTLET_FAR_ACTION].ti = settings.pi;
	availableActions[OUTLET_FAR_ACTION].td = settings.pd;
	availableActions[OUTLET_FAR_ACTION].clear = false;
	availableActions[OUTLET_FAR_ACTION].to = 0;
	availableActions[OUTLET_FAR_ACTION].state = MS_NON_ACTIVE;
	availableActions[OUTLET_FAR_ACTION].child = &availableActions[PUMP_ACTION];
	availableActions[OUTLET_FAR_ACTION].lst = 0;
	availableActions[OUTLET_FAR_ACTION].st = 0;


	state.s[MS_SENSOR_FAR].ai = OUTLET_FAR_ACTION;

	// open mid valve action
	availableActions[OUTLET_MID_ACTION].tick = &tickMidOutlet;
	availableActions[OUTLET_MID_ACTION].frozen = false;
	availableActions[OUTLET_MID_ACTION].stopRequested = false;
	availableActions[OUTLET_MID_ACTION].start = &startMidOutlet;
	availableActions[OUTLET_MID_ACTION].stop = &stopMidOutlet;
	availableActions[OUTLET_MID_ACTION].ti = settings.pi;
	availableActions[OUTLET_MID_ACTION].td = settings.pd;
	availableActions[OUTLET_MID_ACTION].clear = false;
	availableActions[OUTLET_MID_ACTION].to = 0;
	availableActions[OUTLET_MID_ACTION].state = MS_NON_ACTIVE;
	availableActions[OUTLET_MID_ACTION].child = &availableActions[PUMP_ACTION];
	availableActions[OUTLET_MID_ACTION].lst = 0;
	availableActions[OUTLET_MID_ACTION].st = 0;

	state.s[MS_SENSOR_MID].ai = OUTLET_MID_ACTION;

	// open near valve action
	availableActions[OUTLET_NEAR_ACTION].tick = &tickNearOutlet;
	availableActions[OUTLET_NEAR_ACTION].frozen = false;
	availableActions[OUTLET_NEAR_ACTION].stopRequested = false;
	availableActions[OUTLET_NEAR_ACTION].start = &startNearOutlet;
	availableActions[OUTLET_NEAR_ACTION].stop = &stopNearOutlet;
	availableActions[OUTLET_NEAR_ACTION].ti = settings.pi;
	availableActions[OUTLET_NEAR_ACTION].td = settings.pd;
	availableActions[OUTLET_NEAR_ACTION].clear = false;
	availableActions[OUTLET_NEAR_ACTION].to = 0;
	availableActions[OUTLET_NEAR_ACTION].state = MS_NON_ACTIVE;
	availableActions[OUTLET_NEAR_ACTION].child = &availableActions[PUMP_ACTION];
	availableActions[OUTLET_NEAR_ACTION].lst = 0;
	availableActions[OUTLET_NEAR_ACTION].st = 0;

	state.s[MS_SENSOR_NEAR].ai = OUTLET_NEAR_ACTION;

	// start pump action
	availableActions[PUMP_ACTION].tick = &tickPump;
	availableActions[PUMP_ACTION].frozen = false;
	availableActions[PUMP_ACTION].stopRequested = false;
	availableActions[PUMP_ACTION].start = &startPump;
	availableActions[PUMP_ACTION].stop = &stopPump;
	availableActions[PUMP_ACTION].ti = 0;
	availableActions[PUMP_ACTION].td = 0;
	availableActions[PUMP_ACTION].clear = false;
	availableActions[PUMP_ACTION].to = 0;
	availableActions[PUMP_ACTION].state = MS_NON_ACTIVE;
	availableActions[PUMP_ACTION].child = nullptr;
	availableActions[PUMP_ACTION].lst = 0;
	availableActions[PUMP_ACTION].st = 0;


	// draw home action
	availableActions[DRAW_HOME_ACTION].tick = &tickBuildScreen;
	availableActions[DRAW_HOME_ACTION].frozen = true;
	availableActions[DRAW_HOME_ACTION].stopRequested = false;
	availableActions[DRAW_HOME_ACTION].start = &startBuildScreen;
	availableActions[DRAW_HOME_ACTION].stop = &stopBuildScreen;
	availableActions[DRAW_HOME_ACTION].ti = 1000;
	availableActions[DRAW_HOME_ACTION].td = 100;
	availableActions[DRAW_HOME_ACTION].clear = false;
	availableActions[DRAW_HOME_ACTION].to = 0;
	availableActions[DRAW_HOME_ACTION].state = MS_NON_ACTIVE;
	availableActions[DRAW_HOME_ACTION].child = nullptr;
	availableActions[DRAW_HOME_ACTION].lst = 0;
	availableActions[DRAW_HOME_ACTION].st = 0;
}

int readButton() {
	return analogRead(BUTTONS_PIN);
}

void setupInitialState() {
	if (availableActions != nullptr) {
		populateScreens();
		populateActions();
		state.scr = MS_HOME_SCREEN;
		scheduleAction(&executionList, &availableActions[SENSORS_ACTION]);
		scheduleAction(&executionList, &availableActions[DRAW_HOME_ACTION]);
	}
	else {
#ifdef DEBUG
		Serial.println(F("MEM: Actions"));
#endif
	}
}


void extractMedianPinValueForProperty(int delayInterval, int* near, int* mid, int* far) {
	int farV = 0;
	int midV = 0;
	int nearV = 0;

	for (int i = 0; i < 10; i++) {
		farV += analogRead(PIN_FAR);
		midV += analogRead(PIN_MID);
		nearV += analogRead(PIN_NEAR);
		if (delayInterval > 0) {
			delay(delayInterval);
		}
	}

	(*far) = (int)(farV / 10);
	(*mid) = (int)(midV / 10);
	(*near) = (int)(nearV / 10);
}

// End of Helpers

// Actions

// Outlets

// Far

void startFarOutlet(Action* a) {
	digitalWrite(PIN_VALVE_FAR, LOW);
	state.vf = true;
}

void tickFarOutlet(Action* a) {
}

void stopFarOutlet(Action* a) {
	digitalWrite(PIN_VALVE_FAR, HIGH);
	state.vf = false;
}

// end of Far

// Mid

void startMidOutlet(Action* a) {
	digitalWrite(PIN_VALVE_MID, LOW);
	state.vm = true;
}

void tickMidOutlet(Action* a) {
}

void stopMidOutlet(Action* a) {
	digitalWrite(PIN_VALVE_MID, HIGH);
	state.vm = false;
}

// end of Mid

// Near

void startNearOutlet(Action* a) {
	digitalWrite(PIN_VALVE_NEAR, LOW);
	state.vn = true;
}

void tickNearOutlet(Action* a) {
}

void stopNearOutlet(Action* a) {
	digitalWrite(PIN_VALVE_NEAR, HIGH);
	state.vn = false;
}

// end of Near
//

// Sensors

void startSensors(Action* a) {
	digitalWrite(SENSOR_PIN, SENSOR_PIN_HIGH);
}

void stopSensors(Action* a) {
	digitalWrite(SENSOR_PIN, SENSOR_PIN_LOW);
}

void tickSensors(Action* a) {

	extractMedianPinValueForProperty(0, &state.s[MS_SENSOR_NEAR].value, &state.s[MS_SENSOR_MID].value, &state.s[MS_SENSOR_FAR].value);

	bool activate = false;
	Action** acandidates = (Action**)malloc(sizeof(Action*) * SENSORS_COUNT);
	int pc = 0;
	int ac = 0;
	for (int i = 0; i < SENSORS_COUNT; i++) {
		Sensor* se = &state.s[i];
		Action* ca = &availableActions[(*se).ai];
		float v = (float)(*se).value;
		float d = (float)(*se).dry;
		float w = (float)(*se).wet;
		float dwd = _max(0.00001, d - w);

		float mp = ((d - _min(_max(v, w), d)) / dwd) * 100.0;
		(*se).p = (int)mp;
#ifdef DEBUG
		Serial.print(F("%: "));
		Serial.print(mp);
		Serial.println();
#endif
		activate = (*se).active && (bool)((long)mp < (state.p ? settings.dapv : settings.apv));

		acandidates[i] = nullptr;
		if (activate && ca != nullptr) {
			ac++;
			if ((*ca).state == MS_NON_ACTIVE) {
				acandidates[i] = ca;
				pc++;
			}
			else if ((*ca).state == MS_PENDING) {
				requestStop(&executionList, ca);
			}
		}
		else if (ca != nullptr) {
			// Stop the action for which activate is false
			requestStop(&executionList, ca);
		}
	}

	// if the available outlets are equal to the 
	// ones needed to be activated - we select the 
	// one that was activated the earliest
	if (pc > 0 && pc == ac) {

		Action* ts = nullptr;
		for (int i = 0; i < SENSORS_COUNT; i++) {
			Action* c = acandidates[i];

			if (c != nullptr) {
				if (ts != nullptr) {
					if ((*c).st < (*ts).st) {
						ts = c;
					}
				}
				else {
					ts = c;
				}
			}
		}

		if (ts != nullptr) {
			scheduleAction(&executionList, ts);
		}
	}

	(*a).ti = availableActions[PUMP_ACTION].state == MS_CHILD_RUNNING ? settings.siw : settings.sid;
	free(acandidates);
}

//end of sensors

// Display
char* actvalues = (char*)malloc(sizeof(char) * 18);
char* sensorp = (char*)malloc(sizeof(char) * 30);
char* pumpm = (char*)malloc(sizeof(char) * 10);
char* valvesm = (char*)malloc(sizeof(char) * 25);
char* sinfo1 = (char*)malloc(sizeof(char) * 5);
char* sinfo2 = (char*)malloc(sizeof(char) * 5);
char* sinfo3 = (char*)malloc(sizeof(char) * 5);

const char* _hsResolveSensorInfo(char* target, Sensor* s) {
	if ((*s).active) {
		sprintf(target, "%d%%", (*s).p);
	}
	else {
		return MS_OFF_STRING;
	}
	return target;
}

void drawHomeScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	sprintf(actvalues, "PA: %d%% PD: %d%%", settings.apv, settings.dapv);
	sprintf(sensorp, "N: %s M: %s F: %s", _hsResolveSensorInfo(sinfo1, &state.s[MS_SENSOR_NEAR]), _hsResolveSensorInfo(sinfo2, &state.s[MS_SENSOR_MID]), _hsResolveSensorInfo(sinfo3, &state.s[MS_SENSOR_FAR]));
	sprintf(valvesm, "VN: %s VM: %s VF: %s", state.vn ? MS_ON_STRING : MS_OFF_STRING, state.vm ? MS_ON_STRING : MS_OFF_STRING, state.vf ? MS_ON_STRING : MS_OFF_STRING);
	sprintf(pumpm, "PUMP: %s", state.p ? MS_ON_STRING : MS_OFF_STRING);
	char* message[] = { actvalues, sensorp, valvesm, pumpm };
	printAlignedTextStack(&mainCanvas, message, 4, 1, MS_H_CENTER, MS_H_CENTER | MS_V_TOP);
	printAlignedText(&mainCanvas, "B1 - settings, B2 - stats", 1, (MS_H_CENTER | MS_V_BOTTOM));
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleHomeScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		state.scr = MS_STATS_SCREEN;
	}
}

void drawThresholdsSettingsScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	char apv[15], dapv[15];
	sprintf(apv, "APV: %d%%", settings.apv);
	sprintf(dapv, "DAPV: %d%%", settings.dapv);
	char* message[] = { apv, dapv };
	printAlignedTextStack(&mainCanvas, message, 2, MS_FONT_TEXT_SIZE_LARGE, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);

	printAlignedText(&mainCanvas, "B1 - Back, B2-B3 - edit", MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleThresholdsSettingsScreen(int buttonValue) {

	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		int upperLimit = settings.dapv - 5;
		int nv = _min((settings.apv + 5) % settings.dapv, upperLimit);
		settings.apv = nv;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		int nv = _max(_min((settings.dapv + 5) % 105, 100), settings.apv + 5);
		settings.dapv = nv;

	}

#ifdef ARDUINO_ARCH_ESP32
	preferences.begin(MS_PREFERENCES_ID, false);
	preferences.putInt(MS_APV_SETTING_KEY, settings.apv);
	preferences.putInt(MS_DAPV_SETTING_KEY, settings.dapv);
	preferences.end();
#endif
}

void drawSensorSettingsScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	int circleRadius = 17;
	int spacing = 3;
	int boxWidth = 2 * spacing + 3 * (circleRadius * 2);
	int boxX = SCREEN_WIDTH / 2 - boxWidth / 2 + circleRadius;
	int boxY = circleRadius;

	for (int i = 0; i < SENSORS_COUNT; i++) {
		Sensor* s = &state.s[i];
		bool isActive = (*s).active;
		if ((*s).active) {
			mainCanvas.fillCircle(boxX, boxY, circleRadius, SSD1306_WHITE);
		}
		else {

			mainCanvas.drawCircle(boxX, boxY, circleRadius, SSD1306_WHITE);
		}
		int16_t x, y;
		uint16_t w, h;
		sprintf(sensorp, "%d%%", (*s).p);
		mainCanvas.getTextBounds(isActive ? sensorp : MS_OFF_STRING, 0, 0, &x, &y, &w, &h);
		mainCanvas.setCursor(boxX - w / 2 + w % 2, boxY - h / 2 + h % 2 + FONT_BASELINE_CORRECTION_NORMAL / 2);
		mainCanvas.setTextColor(isActive ? SSD1306_BLACK : SSD1306_WHITE);
		mainCanvas.setTextSize(MS_FONT_TEXT_SIZE_NORMAL);

		if (isActive) {
			mainCanvas.print(sensorp);
		}
		else {
			mainCanvas.print(MS_OFF_STRING);
		}

		mainCanvas.getTextBounds((*s).name, 0, 0, &x, &y, &w, &h);
		mainCanvas.setTextColor(SSD1306_WHITE);
		printPositionedText(&mainCanvas, (*s).name, boxX - w / 2, boxY + circleRadius + spacing + FONT_BASELINE_CORRECTION_NORMAL);

		boxX += circleRadius * 2 + spacing;
	}

	printAlignedText(&mainCanvas, "B1 - Back, B2-B4 - edit", MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleSensorSettingsScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		bool isActive = state.s[MS_SENSOR_FAR].active = !state.s[MS_SENSOR_FAR].active;
		if (!isActive) {
			availableActions[OUTLET_MID_ACTION].lst = 0;
			availableActions[OUTLET_MID_ACTION].st = 0;
		}
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		bool isActive = state.s[MS_SENSOR_MID].active = !state.s[MS_SENSOR_MID].active;
		if (!isActive) {
			availableActions[OUTLET_MID_ACTION].lst = 0;
			availableActions[OUTLET_MID_ACTION].st = 0;
		}
	}
	else if (buttonValue > BUTTON_4_LOW && buttonValue < BUTTON_4_HIGH) {
		bool isActive = state.s[MS_SENSOR_NEAR].active = !state.s[MS_SENSOR_NEAR].active;
		if (!isActive) {
			availableActions[OUTLET_NEAR_ACTION].lst = 0;
			availableActions[OUTLET_NEAR_ACTION].st = 0;
		}

	}

#ifdef ARDUINO_ARCH_ESP32
	preferences.begin(MS_PREFERENCES_ID, false);
	preferences.putInt(MS_FAR_ACTIVE_SETTING_KEY, state.s[MS_SENSOR_FAR].active);
	preferences.putInt(MS_MID_ACTIVE_SETTING_KEY, state.s[MS_SENSOR_MID].active);
	preferences.putInt(MS_NEAR_ACTIVE_SETTING_KEY, state.s[MS_SENSOR_NEAR].active);
	preferences.end();
#endif
}

void drawStatisticsScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);

	printAlignedText(&mainCanvas, "Not implemented", MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_CENTER);

	printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleStatisticsScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_HOME_SCREEN;
	}
}

void drawSettingsScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	char* text[] = { "B2 - Intervals", "B3 - Sensors", "B4 - Thresholds" };
	printAlignedTextStack(&mainCanvas, text, 3, 1, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);
	printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleSettingsScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_HOME_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		state.scr = MS_INTERVALS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		state.scr = MS_SENSOR_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_4_LOW && buttonValue < BUTTON_4_HIGH) {
		state.scr = MS_THRESHOLDS_SETTINGS_SCREEN;
	}
}

void drawSensorIntervalsSettingsScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);

	char pm1[20], pm2[20];
	sprintf(pm1, "Pump off: %d s", settings.sid / 1000);
	sprintf(pm2, "Pump on: %d s", settings.siw / 1000);
	char* message[] = { pm1, pm2 };
	printAlignedTextStack(&mainCanvas, message, 2, MS_FONT_TEXT_SIZE_NORMAL, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);

	printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleSensorIntervalsSettingsScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_INTERVALS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		int step = 10000;
		int upperLimit = 5 * 6 * step;
		int nv = _max((settings.sid + step) % (upperLimit + step), step);
		settings.sid = nv;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		int step = 5000;
		int upperLimit = 6 * step;
		int nv = _max((settings.siw + step) % (upperLimit + step), step);
		settings.siw = nv;
	}

#ifdef ARDUINO_ARCH_ESP32
	preferences.begin(MS_PREFERENCES_ID, false);
	preferences.putInt(MS_SENSOR_INTERVAL_PUMPING, settings.siw);
	preferences.putInt(MS_SENSOR_INTERVAL_DRY, settings.sid);
	preferences.end();
#endif
}

void drawPumpIntervalsSettingsScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	char pm1[20], pm2[20];
	sprintf(pm1, "max(T): %d min", settings.pd / 60000);
	sprintf(pm2, "Re-act in: %d min", settings.pi / 60000);
	char* message[] = { pm1, pm2 };
	printAlignedTextStack(&mainCanvas, message, 2, MS_FONT_TEXT_SIZE_NORMAL, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);

	printAlignedText(&mainCanvas, "B1 - Back, B2-B3 - edit", MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handlePumpIntervalsSettingsScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_INTERVALS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		int minute = 60000;
		int upperLimit = 5 * minute;
		int nv = _max((settings.pd + minute) % (upperLimit + minute), minute);
		settings.pd = nv;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		int minute = 60000;
		int upperLimit = 20 * minute;
		int nv = _max((settings.pi + minute) % (upperLimit + minute), minute);
		settings.pi = nv;
	}

#ifdef ARDUINO_ARCH_ESP32
	preferences.begin(MS_PREFERENCES_ID, false);
	preferences.putInt(MS_PUMP_MAX_DURATION, settings.pd);
	preferences.putInt(MS_PUMP_REACT_INT_DURATION, settings.pi);
	preferences.end();
#endif
}

void drawIntervalsSettingsScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	char* text[] = { "B2 - Pump intervals", "B3 - Sensor intervals" };
	printAlignedTextStack(&mainCanvas, text, 2, 1, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);
	printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleIntervalsSettingsScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		state.scr = MS_PUMP_INTERVALS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		state.scr = MS_SENSOR_INTERVALS_SETTINGS_SCREEN;
	}
}

void startBuildScreen(Action* a) {
	int newValue = readButton();
	if (newValue != button.value) {
		button.hasChanged = true;
	}

	button.value = newValue;
	display.clearDisplay();
}

void tickBuildScreen(Action* a) {
	int screenIndex = state.scr;
	if (screenIndex < SCREENS_COUNT) {
		MSScreen* current = (MSScreen*)&availableScreens[screenIndex];
		(*current).drawUI(a);
		if (button.hasChanged) {
			(*current).handleButtons(button.value);
			button.hasChanged = false;
		}
	}
}

void stopBuildScreen(Action* a) {
	button.hasChanged = false;
	button.value = 0;
}

// end of Display

// Pump
void startPump(Action* a) {
	digitalWrite(PUMP_PIN, PUMP_PIN_HIGH);
	state.p = true;
}

void stopPump(Action* a) {
	digitalWrite(PUMP_PIN, PUMP_PIN_LOW);
	state.p = false;
}

void tickPump(Action* a) {
}

//end of pump

// end of Actions

void initActionsList() {
	executionList.availableActions = availableActions;
	executionList.availableActionsCount = ACTIONS_COUNT;
}

void allocateMemPools() {
	availableScreens = (MSScreen*)calloc(SCREENS_COUNT, sizeof(MSScreen));
	availableActions = (Action*)calloc(ACTIONS_COUNT, sizeof(Action));
	state.s = (Sensor*)calloc(SENSORS_COUNT, sizeof(Sensor));
}

void initDisplay(Adafruit_SSD1306* display) {
	if (!(*display).begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
		Serial.println(F("SSD1306 failed"));
		for (;;); // Don't proceed, loop forever
	}
	(*display).clearDisplay();
}

void ms_init() {
	// init must be made with completely dry state values

	initDisplay(&display);
	drawSplashScreen(&display);
	delay(5000);
	drawStartingPromptScreen(&display);

	int bs = 0;
	bool init = false;
	while (true) {
		Serial.println("WAITING FOR VALUE:");
		bs = readButton();
		if (bs > BUTTON_1_LOW && bs < BUTTON_1_HIGH) {
			init = false;
			break;
		}
		else if (bs > BUTTON_2_LOW && bs < BUTTON_2_HIGH) {
			init = true;
			break;
		}
	}
#ifdef ARDUINO_ARCH_ESP32
	preferences.begin(MS_PREFERENCES_ID, false);
#endif
	sprintf(state.s[MS_SENSOR_NEAR].name, "near");
	sprintf(state.s[MS_SENSOR_MID].name, "mid");
	sprintf(state.s[MS_SENSOR_FAR].name, "far");
	// we start with stored values normally
	if (!init) {

#ifdef ARDUINO_ARCH_ESP32
		state.s[MS_SENSOR_NEAR].dry = preferences.getInt(MS_NEAR_DRY_SETTING_KEY);
		state.s[MS_SENSOR_MID].dry = preferences.getInt(MS_MID_DRY_SETTING_KEY);
		state.s[MS_SENSOR_FAR].dry = preferences.getInt(MS_FAR_DRY_SETTING_KEY);

		state.s[MS_SENSOR_NEAR].wet = preferences.getInt(MS_NEAR_WET_SETTING_KEY);
		state.s[MS_SENSOR_MID].wet = preferences.getInt(MS_MID_WET_SETTING_KEY);
		state.s[MS_SENSOR_FAR].wet = preferences.getInt(MS_FAR_WET_SETTING_KEY);

		state.s[MS_SENSOR_NEAR].active = preferences.getBool(MS_NEAR_ACTIVE_SETTING_KEY, true);
		state.s[MS_SENSOR_MID].active = preferences.getBool(MS_MID_ACTIVE_SETTING_KEY, true);
		state.s[MS_SENSOR_FAR].active = preferences.getBool(MS_FAR_ACTIVE_SETTING_KEY, true);

		settings.apv = preferences.getInt(MS_APV_SETTING_KEY, settings.apv);
		settings.dapv = preferences.getInt(MS_DAPV_SETTING_KEY, settings.dapv);

		settings.pd = preferences.getInt(MS_PUMP_MAX_DURATION, settings.pd);
		settings.pi = preferences.getInt(MS_PUMP_REACT_INT_DURATION, settings.pi);

		settings.sid = preferences.getInt(MS_SENSOR_INTERVAL_DRY, settings.sid);
		settings.siw = preferences.getInt(MS_SENSOR_INTERVAL_PUMPING, settings.siw);
#else
		int nd = 0, md = 0, fd = 0, nw = 0, mw = 0, fw = 0;
		EEPROM.get(0, nd);
		EEPROM.get(sizeof(int), md);
		EEPROM.get(sizeof(int) * 2, fd);
		EEPROM.get(sizeof(int) * 3, nw);
		EEPROM.get(sizeof(int) * 4, mw);
		EEPROM.get(sizeof(int) * 5, fw);
		state.s[MS_SENSOR_NEAR].dry = nd;
		state.s[MS_SENSOR_MID].dry = md;
		state.s[MS_SENSOR_FAR].dry = fd;

		state.s[MS_SENSOR_NEAR].wet = nw;
		state.s[MS_SENSOR_MID].wet = mw;
		state.s[MS_SENSOR_FAR].wet = fw;
#endif
		drawStartingValuesScreen(&display);
		delay(2000);
	}
	else {
		digitalWrite(SENSOR_PIN, SENSOR_PIN_HIGH);
#ifdef ARDUINO_ARCH_ESP32
		preferences.clear();
#else
		for (int i = 0; i < EEPROM.length(); i++) {
			EEPROM.put(i, 0);
		}
#endif
		display.clearDisplay();
		showActionPromptScreen(&display, "B2", "get dry state");
		int br = readButton();
		while (!(br > BUTTON_2_LOW && br < BUTTON_2_HIGH)) {
			br = readButton();
		}
		display.clearDisplay();
		showTextCaptionScreen(&display, MS_READING_PROMPT_TEXT);

		// waiting for completely wet values
		extractMedianPinValueForProperty(1000, &state.s[MS_SENSOR_NEAR].dry, &state.s[MS_SENSOR_MID].dry, &state.s[MS_SENSOR_FAR].dry);
#ifdef ARDUINO_ARCH_ESP32
		preferences.putInt(MS_NEAR_DRY_SETTING_KEY, state.s[MS_SENSOR_NEAR].dry);
		preferences.putInt(MS_MID_DRY_SETTING_KEY, state.s[MS_SENSOR_MID].dry);
		preferences.putInt(MS_FAR_DRY_SETTING_KEY, state.s[MS_SENSOR_FAR].dry);
#else
		EEPROM.put(0, state.s[MS_SENSOR_NEAR].dry);
		EEPROM.put(sizeof(int), state.s[MS_SENSOR_MID].dry);
		EEPROM.put(sizeof(int) * 2, state.s[MS_SENSOR_FAR].dry);
#endif

		// fire buzzer to notify that dry values have been read
		drawDryValuesScreen(&display);
		delay(5000);
		display.clearDisplay();
		showActionPromptScreen(&display, "B2", "get wet state");
		br = readButton();
		while (!(br > BUTTON_2_LOW && br < BUTTON_2_HIGH)) {
			br = readButton();
		}
		display.clearDisplay();
		showTextCaptionScreen(&display, MS_READING_PROMPT_TEXT);

		// waiting for completely wet values
		extractMedianPinValueForProperty(1000, &state.s[MS_SENSOR_NEAR].wet, &state.s[MS_SENSOR_MID].wet, &state.s[MS_SENSOR_FAR].wet);
#ifdef ARDUINO_ARCH_ESP32
		preferences.putInt(MS_NEAR_WET_SETTING_KEY, state.s[MS_SENSOR_NEAR].wet);
		preferences.putInt(MS_MID_WET_SETTING_KEY, state.s[MS_SENSOR_MID].wet);
		preferences.putInt(MS_FAR_WET_SETTING_KEY, state.s[MS_SENSOR_FAR].wet);
#else
		EEPROM.put(sizeof(int) * 3, state.s[MS_SENSOR_NEAR].wet);
		EEPROM.put(sizeof(int) * 4, state.s[MS_SENSOR_MID].wet);
		EEPROM.put(sizeof(int) * 5, state.s[MS_SENSOR_FAR].wet);
#endif


		// wet values have been read
		drawWetValuesScreen(&display);
		delay(5000);
		display.clearDisplay();
		showActionPromptScreen(&display, "B2", "start...");
		// click to continue
		br = readButton();
		while (!(br > BUTTON_2_LOW && br < BUTTON_2_HIGH)) {
			br = readButton();
		}

		digitalWrite(SENSOR_PIN, SENSOR_PIN_LOW);
	}
#ifdef ARDUINO_ARCH_ESP32
	preferences.end();
#endif

}

void setup() {
	pinMode(PIN_FAR, INPUT);
	pinMode(PIN_MID, INPUT);
	pinMode(PIN_NEAR, INPUT);

	pinMode(PIN_VALVE_FAR, OUTPUT);
	pinMode(PIN_VALVE_MID, OUTPUT);
	pinMode(PIN_VALVE_NEAR, OUTPUT);

	digitalWrite(PIN_VALVE_FAR, HIGH);
	digitalWrite(PIN_VALVE_MID, HIGH);
	digitalWrite(PIN_VALVE_NEAR, HIGH);

	pinMode(BUTTONS_PIN, INPUT);

	pinMode(PUMP_PIN, OUTPUT); // pump relay
	digitalWrite(PUMP_PIN, PUMP_PIN_LOW);

	pinMode(SENSOR_PIN, OUTPUT); // sensor relay
	digitalWrite(SENSOR_PIN, SENSOR_PIN_LOW);
	Serial.begin(9600);

	allocateMemPools();
	
	ms_init();

	initActionsList();
	setupInitialState();
}

void loop() {
	doQueueActions(&executionList);
}
