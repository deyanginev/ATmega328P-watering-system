#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/Org_01.h>

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


#ifdef ARDUINO_ARCH_ESP32

#include <Preferences.h>
#define MS_PREFERENCES_ID "msys"

#elif

#include <EEPROM.h>
#endif

#define MS_BUTTON1 1
#define MS_BUTTON2 2
#define MS_BUTTON3 3
#define MS_BUTTON4 4

#define SCREENS_COUNT 4

#define MS_HOME_SCREEN 0
#define MS_SETTINGS_SCREEN 1
#define MS_SENSOR_SETTINGS_SCREEN 2
#define MS_THRESHOLDS_SETTINGS_SCREEN 3

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
#define PWMFreq 800
#define PWMChannel 0
#define PWMRes 8

#define MS_BUZZER_VOLUME 15

#define MS_BUZZER_PIN 13

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


#define MS_BUZZER_PIN 3
#define MS_BUZZER_VOLUME 15

#define PIN_FAR A0
#define PIN_MID A1
#define PIN_NEAR A2

#define PIN_VALVE_FAR 10
#define PIN_VALVE_MID 11
#define PIN_VALVE_NEAR 12

#define BUTTONS_PIN A3

#endif

#define SENSORS_COUNT 3

// action defines
#define SENSORS_ACTION 0
#define BUZZER_ACTION 1
#define OUTLET_FAR_ACTION 2
#define OUTLET_MID_ACTION 3
#define OUTLET_NEAR_ACTION 4
#define PUMP_ACTION 5
#define DRAW_HOME_ACTION 6

#define ACTIONS_COUNT 7


// end of action defines

//#undef DEBUG
#define DEBUG

#define MS_SENSOR_FAR  0
#define MS_SENSOR_MID  1
#define MS_SENSOR_NEAR  2


enum ActionState {
	MS_NON_ACTIVE = 0,
	MS_SCHEDULED = 1,
	MS_PENDING = 2,
	MS_RUNNING = 3,
	MS_CHILD_RUNNING = 4,
	MS_CHILD_PENDING = 5
};

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
};

struct SystemState {
	int scr = MS_HOME_SCREEN;
	Sensor* s = nullptr;
	bool p = false;
	bool vn = false;
	bool vm = false;
	bool vf = false;
} state;

struct Action {
	Action* next = nullptr;
	Action* prev = nullptr;
	bool clear;
	bool frozen = false; // once scheduled the action never expires
	bool stopRequested = false;
	unsigned long ti = 0; // interval between executions
	unsigned long td = 1000; // maximum duration interval
	unsigned long lst = -1; // last stop time
	unsigned long to = 0; // first tick offset from start
	unsigned long st = 0; // last start time
	ActionState state = MS_NON_ACTIVE;
	void (*tick)(SystemState*, Action*);
	void (*start)(SystemState*, Action*);
	void (*stop)(SystemState*, Action*);
	Action* child = nullptr; // a child action; Child actions are activated after the master action has been activated;
	   // child actions are deactivated before the master action is deactivated;
	   // child actions are not part of the execution queue
};

// An array used for iterating over scheduled
// actions and executing them
Action* availableActions = nullptr;

struct MSScreen {
	void (*handleButtons)(SystemState* state, int buttonValue);
	void (*drawUI)(SystemState* state, Action* a);
	int code;
};


// An array containing the available screens
MSScreen* availableScreens;

struct ButtonState {
	int value = 0;
	bool hasChanged = false;
} button;

struct ActionsList {
	size_t count = 0;
	Action* first = nullptr;
	Action* last = nullptr;
} executionList;

// end of structures

// Queue

int indexOfAction(Action* a) {
	for (int i = 0; i < ACTIONS_COUNT; i++) {
		if (a == &availableActions[i]) {
			return i;
		}
	}

	return -1;
}

Action* find(ActionsList* list, Action* item) {
	Action* cur = (*list).first;

	while (cur != nullptr) {
		if (cur == item) {
			return cur;
		}
		cur = (*cur).next;
	}
	return nullptr;
}

#ifdef DEBUG

void printList(ActionsList* list) {
	Action* cur = (*list).first;
	while (cur != nullptr) {
		Serial.print(F(" Item: "));
		Serial.print(indexOfAction(cur));
		Serial.flush();
		cur = (*cur).next;
	}

	Serial.println();
}

#endif

Action* addListItem(ActionsList* list, Action* a) {

	if (a != nullptr) {

		(*a).next = nullptr;
		(*a).prev = (*list).last;

		if ((*list).last != nullptr) {
			(*(*list).last).next = a;
		}

		(*list).last = a;

		if ((*list).first == nullptr) {
			(*list).first = a;
		}

		(*list).count++;
#ifdef DEBUG
		printList(list);
#endif
		return a;
	}
	else {
#ifdef DEBUG
		Serial.println(F("ITEM: null"));
#endif
	}

	return nullptr;
}

bool removeListItem(ActionsList* list, Action* item) {

	Action* tr = find(list, item);

	if (tr == nullptr) {
		return false;
	}

	if ((*list).last == tr) {
		(*list).last = (*tr).prev;
	}

	if ((*list).first == tr) {
		(*list).first = (*tr).next;
	}

	Action* pr = (*tr).prev;
	Action* nx = (*tr).next;

	if (pr != nullptr) {
		(*pr).next = nx;
	}

	if (nx != nullptr) {
		(*nx).prev = pr;
	}

	(*tr).prev = nullptr;
	(*tr).next = nullptr;

	(*list).count--;
#ifdef DEBUG
	printList(list);
#endif
	return true;
}

// end of Queue

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

void drawStartingPromptScreen(SystemState* state, Adafruit_SSD1306* display) {
	char* prompt[] = { "Actions:", "B1 - start", "B2 - init" };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedTextStack(&canvas, prompt, 3, DEFAULT_TEXT_SIZE, MS_H_LEFT, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void drawDryValuesScreen(SystemState* state, Adafruit_SSD1306* display) {
	char nearA[15], midA[15], farA[15];
	sprintf(nearA, "Near: %d", (*state).s[MS_SENSOR_NEAR].dry);
	sprintf(midA, "Mid: %d", (*state).s[MS_SENSOR_MID].dry);
	sprintf(farA, "Far: %d", (*state).s[MS_SENSOR_FAR].dry);

	char* message[] = { "Dry values:", nearA, midA, farA };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedTextStack(&canvas, message, 4, DEFAULT_TEXT_SIZE, MS_H_LEFT, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void drawWetValuesScreen(SystemState* state, Adafruit_SSD1306* display) {
	char nearA[15], midA[15], farA[15];
	sprintf(nearA, "Near: %d", (*state).s[MS_SENSOR_NEAR].wet);
	sprintf(midA, "Mid: %d", (*state).s[MS_SENSOR_MID].wet);
	sprintf(farA, "Far: %d", (*state).s[MS_SENSOR_FAR].wet);

	char* message[] = { "Wet values:", nearA, midA, farA };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedTextStack(&canvas, message, 4, DEFAULT_TEXT_SIZE, MS_H_LEFT, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void drawStartingValuesScreen(SystemState* state, Adafruit_SSD1306* display) {
	drawDryValuesScreen(state, display);
	delay(5000);
	drawWetValuesScreen(state, display);
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

void showTextCaptionScreen(SystemState* state, Adafruit_SSD1306* display, const char* caption) {
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedText(&canvas, caption, 2, MS_V_CENTER | MS_H_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void showActionPromptScreen(SystemState* state, Adafruit_SSD1306* display, char* btn, char* action) {
	char btnstr[10];
	sprintf(btnstr, "Press %s to", btn);
	char* message[] = { btnstr, action };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedTextStack(&canvas, message, 3, DEFAULT_TEXT_SIZE, MS_H_CENTER, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void drawSplashScreen(SystemState* state, Adafruit_SSD1306* display) {
	char* texts[] = { "Irrigation", "System", "Version: 0.1" };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	canvas.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 10, SSD1306_WHITE);
	printAlignedTextStack(&canvas, texts, 3, DEFAULT_TEXT_SIZE, MS_H_CENTER, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

// end of Display

void populateScreens(SystemState* state, MSScreen* screens) {
	screens[MS_HOME_SCREEN].drawUI = &drawHomeScreen;
	screens[MS_HOME_SCREEN].handleButtons = &handleHomeScreen;

	screens[MS_SETTINGS_SCREEN].drawUI = &drawSettingsScreen;
	screens[MS_SETTINGS_SCREEN].handleButtons = &handleSettingsScreen;

	screens[MS_SENSOR_SETTINGS_SCREEN].drawUI = &drawSensorSettingsScreen;
	screens[MS_SENSOR_SETTINGS_SCREEN].handleButtons = &handleSensorSettingsScreen;

	screens[MS_THRESHOLDS_SETTINGS_SCREEN].drawUI = &drawThresholdsSettingsScreen;
	screens[MS_THRESHOLDS_SETTINGS_SCREEN].handleButtons = &handleThresholdsSettingsScreen;
}

void populateActions(SystemState* state, Action* actions) {


	// read sensors action
	actions[SENSORS_ACTION].tick = &tickSensors;
	actions[SENSORS_ACTION].frozen = true;
	actions[SENSORS_ACTION].stopRequested = false;
	actions[SENSORS_ACTION].start = &startSensors;
	actions[SENSORS_ACTION].stop = &stopSensors;
	actions[SENSORS_ACTION].ti = settings.siw;
	actions[SENSORS_ACTION].td = settings.sd;
	actions[SENSORS_ACTION].to = 200;
	actions[SENSORS_ACTION].clear = false;
	actions[SENSORS_ACTION].state = MS_NON_ACTIVE;
	actions[SENSORS_ACTION].child = nullptr;
	actions[SENSORS_ACTION].lst = 0;
	actions[SENSORS_ACTION].st = 0;

	// sound buzzer action
	actions[BUZZER_ACTION].tick = &tickBuzzer;
	actions[BUZZER_ACTION].frozen = false;
	actions[BUZZER_ACTION].stopRequested = false;
	actions[BUZZER_ACTION].start = &startBuzzer;
	actions[BUZZER_ACTION].stop = &stopBuzzer;
	actions[BUZZER_ACTION].ti = 0;
	actions[BUZZER_ACTION].td = 300;
	actions[BUZZER_ACTION].clear = false;
	actions[BUZZER_ACTION].to = 0;
	actions[BUZZER_ACTION].state = MS_NON_ACTIVE;
	actions[BUZZER_ACTION].child = nullptr;
	actions[BUZZER_ACTION].lst = 0;
	actions[BUZZER_ACTION].st = 0;

	// open far valve action
	actions[OUTLET_FAR_ACTION].tick = &tickFarOutlet;
	actions[OUTLET_FAR_ACTION].frozen = false;
	actions[OUTLET_FAR_ACTION].stopRequested = false;
	actions[OUTLET_FAR_ACTION].start = &startFarOutlet;
	actions[OUTLET_FAR_ACTION].stop = &stopFarOutlet;
	actions[OUTLET_FAR_ACTION].ti = settings.pi;
	actions[OUTLET_FAR_ACTION].td = settings.pd;
	actions[OUTLET_FAR_ACTION].clear = false;
	actions[OUTLET_FAR_ACTION].to = 0;
	actions[OUTLET_FAR_ACTION].state = MS_NON_ACTIVE;
	actions[OUTLET_FAR_ACTION].child = &actions[PUMP_ACTION];
	actions[OUTLET_FAR_ACTION].lst = 0;
	actions[OUTLET_FAR_ACTION].st = 0;


	(*state).s[MS_SENSOR_FAR].ai = OUTLET_FAR_ACTION;

	// open mid valve action
	actions[OUTLET_MID_ACTION].tick = &tickMidOutlet;
	actions[OUTLET_MID_ACTION].frozen = false;
	actions[OUTLET_MID_ACTION].stopRequested = false;
	actions[OUTLET_MID_ACTION].start = &startMidOutlet;
	actions[OUTLET_MID_ACTION].stop = &stopMidOutlet;
	actions[OUTLET_MID_ACTION].ti = settings.pi;
	actions[OUTLET_MID_ACTION].td = settings.pd;
	actions[OUTLET_MID_ACTION].clear = false;
	actions[OUTLET_MID_ACTION].to = 0;
	actions[OUTLET_MID_ACTION].state = MS_NON_ACTIVE;
	actions[OUTLET_MID_ACTION].child = &actions[PUMP_ACTION];
	actions[OUTLET_MID_ACTION].lst = 0;
	actions[OUTLET_MID_ACTION].st = 0;

	(*state).s[MS_SENSOR_MID].ai = OUTLET_MID_ACTION;

	// open near valve action
	actions[OUTLET_NEAR_ACTION].tick = &tickNearOutlet;
	actions[OUTLET_NEAR_ACTION].frozen = false;
	actions[OUTLET_NEAR_ACTION].stopRequested = false;
	actions[OUTLET_NEAR_ACTION].start = &startNearOutlet;
	actions[OUTLET_NEAR_ACTION].stop = &stopNearOutlet;
	actions[OUTLET_NEAR_ACTION].ti = settings.pi;
	actions[OUTLET_NEAR_ACTION].td = settings.pd;
	actions[OUTLET_NEAR_ACTION].clear = false;
	actions[OUTLET_NEAR_ACTION].to = 0;
	actions[OUTLET_NEAR_ACTION].state = MS_NON_ACTIVE;
	actions[OUTLET_NEAR_ACTION].child = &actions[PUMP_ACTION];
	actions[OUTLET_NEAR_ACTION].lst = 0;
	actions[OUTLET_NEAR_ACTION].st = 0;

	(*state).s[MS_SENSOR_NEAR].ai = OUTLET_NEAR_ACTION;

	// start pump action
	actions[PUMP_ACTION].tick = &tickPump;
	actions[PUMP_ACTION].frozen = false;
	actions[PUMP_ACTION].stopRequested = false;
	actions[PUMP_ACTION].start = &startPump;
	actions[PUMP_ACTION].stop = &stopPump;
	actions[PUMP_ACTION].ti = 0;
	actions[PUMP_ACTION].td = 0;
	actions[PUMP_ACTION].clear = false;
	actions[PUMP_ACTION].to = 0;
	actions[PUMP_ACTION].state = MS_NON_ACTIVE;
	actions[PUMP_ACTION].child = nullptr;
	actions[PUMP_ACTION].lst = 0;
	actions[PUMP_ACTION].st = 0;


	// draw home action
	actions[DRAW_HOME_ACTION].tick = &tickBuildScreen;
	actions[DRAW_HOME_ACTION].frozen = true;
	actions[DRAW_HOME_ACTION].stopRequested = false;
	actions[DRAW_HOME_ACTION].start = &startBuildScreen;
	actions[DRAW_HOME_ACTION].stop = &stopBuildScreen;
	actions[DRAW_HOME_ACTION].ti = 1000;
	actions[DRAW_HOME_ACTION].td = 100;
	actions[DRAW_HOME_ACTION].clear = false;
	actions[DRAW_HOME_ACTION].to = 0;
	actions[DRAW_HOME_ACTION].state = MS_NON_ACTIVE;
	actions[DRAW_HOME_ACTION].child = nullptr;
	actions[DRAW_HOME_ACTION].lst = 0;
	actions[DRAW_HOME_ACTION].st = 0;
}

void makeBeeps(int count) {
	for (int i = 0; i < count; i++) {
		soundBuzzer(300);
		delay(300);
	}
}

void scheduleBuzzerAction(int millis) {
	availableActions[BUZZER_ACTION].td = millis;
	scheduleAction(&executionList, &availableActions[BUZZER_ACTION]);
}

int readButton() {
	return analogRead(BUTTONS_PIN);
}

void soundBuzzer(int millis) {
#ifndef ARDUINO_ARCH_ESP32
	analogWrite(MS_BUZZER_PIN, MS_BUZZER_VOLUME);
	delay(millis);
	analogWrite(MS_BUZZER_PIN, 0);
#endif
#ifdef ARDUINO_ARCH_ESP32
	ledcWrite(PWMChannel, (uint32_t)(pow(2, PWMRes) - 120));
	delay(millis);
	ledcWrite(PWMChannel, (uint32_t)0);
#endif
}

bool descheduleAction(ActionsList* list, Action* a) {
	Action* found = find(list, a);
	if (found == nullptr) {
		return false;
	}

	if (found != nullptr && (*found).state != MS_RUNNING) {
		(*found).state = MS_NON_ACTIVE;

		removeListItem(&executionList, found);

		return true;
	}

	return false;
}

bool scheduleAction(ActionsList* list, Action* a) {
	Action* found = find(list, a);
	if (found != nullptr) {
		return false;
	}

	if ((*a).state != MS_NON_ACTIVE) {
		return false;
	}

	if (a != nullptr) {
		(*a).state = MS_SCHEDULED;
		addListItem(list, a);
		return true;
	}

	return false;
}

void setupInitialState(SystemState* state) {
	if (availableActions != nullptr) {
		populateScreens(state, availableScreens);
		populateActions(state, availableActions);
		(*state).scr = MS_HOME_SCREEN;
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

void startFarOutlet(SystemState* state, Action* a) {
	digitalWrite(PIN_VALVE_FAR, LOW);
	(*state).vf = true;
}

void tickFarOutlet(SystemState* state, Action* a) {
}

void stopFarOutlet(SystemState* state, Action* a) {
	digitalWrite(PIN_VALVE_FAR, HIGH);
	(*state).vf = false;
}

// end of Far

// Mid

void startMidOutlet(SystemState* state, Action* a) {
	digitalWrite(PIN_VALVE_MID, LOW);
	(*state).vm = true;
}

void tickMidOutlet(SystemState* state, Action* a) {
}

void stopMidOutlet(SystemState* state, Action* a) {
	digitalWrite(PIN_VALVE_MID, HIGH);
	(*state).vm = false;
}

// end of Mid

// Near

void startNearOutlet(SystemState* state, Action* a) {
	digitalWrite(PIN_VALVE_NEAR, LOW);
	(*state).vn = true;
}

void tickNearOutlet(SystemState* state, Action* a) {
}

void stopNearOutlet(SystemState* state, Action* a) {
	digitalWrite(PIN_VALVE_NEAR, HIGH);
	(*state).vn = false;
}

// end of Near
//

// Sensors

void startSensors(SystemState* state, Action* a) {
	digitalWrite(SENSOR_PIN, SENSOR_PIN_HIGH);
}

void stopSensors(SystemState* state, Action* a) {
	digitalWrite(SENSOR_PIN, SENSOR_PIN_LOW);
}

void tickSensors(SystemState* state, Action* a) {

	extractMedianPinValueForProperty(0, &(*state).s[MS_SENSOR_NEAR].value, &(*state).s[MS_SENSOR_MID].value, &(*state).s[MS_SENSOR_FAR].value);

	bool activate = false;
	Action** acandidates = (Action**)malloc(sizeof(Action*) * SENSORS_COUNT);
	int pc = 0;
	int ac = 0;
	for (int i = 0; i < SENSORS_COUNT; i++) {
		Sensor* se = &(*state).s[i];
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
		activate = (*se).active && (bool)((long)mp < ((*state).p ? settings.dapv : settings.apv));

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

char* _hsResolveSensorInfo(char* target, Sensor* s) {
	if ((*s).active) {
		sprintf(target, "%d%%", (*s).p);
	}
	else {
		sprintf(target, "OFF");
	}
	return target;
}

void drawHomeScreen(SystemState* state, Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	sprintf(actvalues, "PA: %d%% PD: %d%%", settings.apv, settings.dapv);
	sprintf(sensorp, "N: %s M: %s F: %s", _hsResolveSensorInfo(sinfo1, &(*state).s[MS_SENSOR_NEAR]), _hsResolveSensorInfo(sinfo2, &(*state).s[MS_SENSOR_MID]), _hsResolveSensorInfo(sinfo3, &(*state).s[MS_SENSOR_FAR]));
	sprintf(valvesm, "VN: %s VM: %s VF: %s", (*state).vn ? "ON" : "OFF", (*state).vm ? "ON" : "OFF", (*state).vf ? "ON" : "OFF");
	sprintf(pumpm, "PUMP: %s", (*state).p ? "ON" : "OFF");
	char* message[] = { actvalues, sensorp, valvesm, pumpm };
	printAlignedTextStack(&mainCanvas, message, 4, 1, MS_H_CENTER, MS_H_CENTER | MS_V_TOP);
	printAlignedText(&mainCanvas, "B1 - settings", 1, (MS_H_CENTER | MS_V_BOTTOM));
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleHomeScreen(SystemState* state, int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		(*state).scr = MS_SETTINGS_SCREEN;
	}
}

void drawThresholdsSettingsScreen(SystemState* state, Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	char apv[15], dapv[15];
	sprintf(apv, "APV: %d%%", settings.apv);
	sprintf(dapv, "DAPV: %d%%", settings.dapv);
	char* message[] = { apv, dapv };
	printAlignedTextStack(&mainCanvas, message, 2, MS_FONT_TEXT_SIZE_LARGE, MS_H_LEFT, MS_H_CENTER | MS_V_CENTER);

	printAlignedText(&mainCanvas, "B1 - Back, B2-B3 - edit", MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleThresholdsSettingsScreen(SystemState* state, int buttonValue) {

	preferences.begin(MS_PREFERENCES_ID, false);
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		(*state).scr = MS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		int upperLimit = settings.dapv - 5;
		int nv = _min((settings.apv + 5) % settings.dapv, upperLimit);
		settings.apv = nv;
		preferences.putInt("apv", nv);
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		int nv = _max(_min((settings.dapv + 5) % 105, 100), settings.apv + 5);
		settings.dapv = nv;
		preferences.putInt("dapv", nv);
	}
	preferences.end();
}

void drawSensorSettingsScreen(SystemState* state, Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	int circleRadius = 17;
	int spacing = 3;
	int boxWidth = 2 * spacing + 3 * (circleRadius * 2);
	int boxX = SCREEN_WIDTH / 2 - boxWidth / 2 + circleRadius;
	int boxY = circleRadius;

	for (int i = 0; i < SENSORS_COUNT; i++) {
		Sensor* s = &(*state).s[i];
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
		mainCanvas.getTextBounds(isActive ? sensorp : "OFF", 0, 0, &x, &y, &w, &h);
		mainCanvas.setCursor(boxX - w / 2 + w % 2, boxY - h / 2 + h % 2 + FONT_BASELINE_CORRECTION_NORMAL / 2);
		mainCanvas.setTextColor(isActive ? SSD1306_BLACK : SSD1306_WHITE);
		mainCanvas.setTextSize(MS_FONT_TEXT_SIZE_NORMAL);

		if (isActive) {
			mainCanvas.print(sensorp);
		}
		else {
			mainCanvas.print("OFF");
		}

		sprintf(sensorp, "S: %d", i + 1);
		mainCanvas.getTextBounds(sensorp, 0, 0, &x, &y, &w, &h);
		mainCanvas.setTextColor(SSD1306_WHITE);
		printPositionedText(&mainCanvas, sensorp, boxX - w / 2, boxY + circleRadius + spacing + FONT_BASELINE_CORRECTION_NORMAL);

		boxX += circleRadius * 2 + spacing;
	}

	printAlignedText(&mainCanvas, "B1 - Back, B2-B4 - edit", MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleSensorSettingsScreen(SystemState* state, int buttonValue) {
	preferences.begin(MS_PREFERENCES_ID, false);
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		(*state).scr = MS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		bool isActive = (*state).s[MS_SENSOR_FAR].active = !(*state).s[MS_SENSOR_FAR].active;
		if (!isActive) {
			availableActions[OUTLET_MID_ACTION].lst = 0;
			availableActions[OUTLET_MID_ACTION].st = 0;
		}
		preferences.putBool("far-active", isActive);
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		bool isActive = (*state).s[MS_SENSOR_MID].active = !(*state).s[MS_SENSOR_MID].active;
		if (!isActive) {
			availableActions[OUTLET_MID_ACTION].lst = 0;
			availableActions[OUTLET_MID_ACTION].st = 0;
		}
		preferences.putBool("mid-active", isActive);
	}
	else if (buttonValue > BUTTON_4_LOW && buttonValue < BUTTON_4_HIGH) {
		bool isActive = (*state).s[MS_SENSOR_NEAR].active = !(*state).s[MS_SENSOR_NEAR].active;
		if (!isActive) {
			availableActions[OUTLET_NEAR_ACTION].lst = 0;
			availableActions[OUTLET_NEAR_ACTION].st = 0;
		}

		preferences.putBool("near-active", isActive);
	}
	preferences.end();
}

void drawSettingsScreen(SystemState* state, Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	char* text[] = { "B2 - Intervals", "B3 - Sensors", "B4 - Thresholds" };
	printAlignedTextStack(&mainCanvas, text, 3, 1, MS_H_LEFT, MS_H_CENTER | MS_V_CENTER);
	printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleSettingsScreen(SystemState* state, int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		(*state).scr = MS_HOME_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		// (*state).scr = MS_THRESHOLDS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		(*state).scr = MS_SENSOR_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_4_LOW && buttonValue < BUTTON_4_HIGH) {
		(*state).scr = MS_THRESHOLDS_SETTINGS_SCREEN;
	}
}

void drawIntervalsSettingsScreen(SystemState* state, Action* a) {

}

void handleIntervalsSettingsScreen(SystemState* state, int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		(*state).scr = MS_SETTINGS_SCREEN;
	}
}

void startBuildScreen(SystemState* state, Action* a) {
	int newValue = readButton();
	if (newValue != button.value) {
		button.hasChanged = true;
	}

	button.value = newValue;
	display.clearDisplay();
}

void tickBuildScreen(SystemState* state, Action* a) {
	int screenIndex = (*state).scr;
	if (screenIndex < SCREENS_COUNT) {
		MSScreen* current = (MSScreen*)&availableScreens[screenIndex];
		(*current).drawUI(state, a);
		if (button.hasChanged) {
			(*current).handleButtons(state, button.value);
			button.hasChanged = false;
		}
	}
}

void stopBuildScreen(SystemState* state, Action* a) {
	button.hasChanged = false;
	button.value = 0;
}

// end of Display

// Pump
void startPump(SystemState* state, Action* a) {
	digitalWrite(PUMP_PIN, PUMP_PIN_HIGH);
	(*state).p = true;
}

void stopPump(SystemState* state, Action* a) {
	digitalWrite(PUMP_PIN, PUMP_PIN_LOW);
	(*state).p = false;
}

void tickPump(SystemState* state, Action* a) {
}

//end of pump

// sound buzzer

void startBuzzer(SystemState* state, Action* a) {
#ifdef ARDUINO_ARCH_ESP32
	ledcWrite(PWMChannel, (uint32_t)(pow(2, PWMRes) - 240));
#elif
	analogWrite(MS_BUZZER_PIN, MS_BUZZER_VOLUME);
#endif
}

void tickBuzzer(SystemState* state, Action* a) {
}

void stopBuzzer(SystemState* state, Action* a) {
#ifdef ARDUINO_ARCH_ESP32
	ledcWrite(PWMChannel, (uint32_t)(0));
#elif
	analogWrite(MS_BUZZER_PIN, LOW);
#endif
}

// end of sound buzzer

// end of Actions

void allocateMemPools(SystemState* state) {
	availableScreens = (MSScreen*)calloc(SCREENS_COUNT, sizeof(MSScreen));
	availableActions = (Action*)calloc(ACTIONS_COUNT, sizeof(Action));
	(*state).s = (Sensor*)calloc(SENSORS_COUNT, sizeof(Sensor));
}

void initDisplay(SystemState* state, Adafruit_SSD1306* display) {
	if (!(*display).begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
		Serial.println(F("SSD1306 failed"));
		for (;;); // Don't proceed, loop forever
	}
	(*display).clearDisplay();
}

void init(SystemState* state) {
	// init must be made with completely dry state values

	soundBuzzer(1000);

	initDisplay(state, &display);
	drawSplashScreen(state, &display);
	delay(5000);
	drawStartingPromptScreen(state, &display);

	int bs = 0;
	bool init = false;
	while (true) {
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

	// we start with stored values normally
	if (!init) {
		makeBeeps(3);


#ifdef ARDUINO_ARCH_ESP32
		(*state).s[MS_SENSOR_NEAR].dry = preferences.getInt(MS_NEAR_DRY_SETTING_KEY);
		(*state).s[MS_SENSOR_MID].dry = preferences.getInt(MS_MID_DRY_SETTING_KEY);
		(*state).s[MS_SENSOR_FAR].dry = preferences.getInt(MS_FAR_DRY_SETTING_KEY);

		(*state).s[MS_SENSOR_NEAR].wet = preferences.getInt(MS_NEAR_WET_SETTING_KEY);
		(*state).s[MS_SENSOR_MID].wet = preferences.getInt(MS_MID_WET_SETTING_KEY);
		(*state).s[MS_SENSOR_FAR].wet = preferences.getInt(MS_FAR_WET_SETTING_KEY);

		(*state).s[MS_SENSOR_NEAR].active = preferences.getBool(MS_NEAR_ACTIVE_SETTING_KEY, true);
		(*state).s[MS_SENSOR_MID].active = preferences.getBool(MS_MID_ACTIVE_SETTING_KEY, true);
		(*state).s[MS_SENSOR_FAR].active = preferences.getBool(MS_FAR_ACTIVE_SETTING_KEY, true);

		settings.apv = preferences.getInt(MS_APV_SETTING_KEY, 55);
		settings.dapv = preferences.getInt(MS_DAPV_SETTING_KEY, 80);
#elif
		int nd = 0, md = 0, fd = 0, nw = 0, mw = 0, fw = 0;
		EEPROM.get(0, nd);
		EEPROM.get(sizeof(int), md);
		EEPROM.get(sizeof(int) * 2, fd);
		EEPROM.get(sizeof(int) * 3, nw);
		EEPROM.get(sizeof(int) * 4, mw);
		EEPROM.get(sizeof(int) * 5, fw);
		(*state).s[near].dry = nd;
		(*state).s[mid].dry = md;
		(*state).s[far].dry = fd;

		(*state).s[near].wet = nw;
		(*state).s[mid].wet = mw;
		(*state).s[far].wet = fw;
#endif
		/*drawStartingValuesScreen(state, &display);
		delay(2000);*/
	}
	else {
		digitalWrite(SENSOR_PIN, SENSOR_PIN_HIGH);
		makeBeeps(2);
#ifdef ARDUINO_ARCH_ESP32
		preferences.clear();
		preferences.putInt(MS_APV_SETTING_KEY, settings.apv);
		preferences.putInt(MS_DAPV_SETTING_KEY, settings.dapv);
#elif
		for (int i = 0; i < EEPROM.length(); i++) {
			EEPROM.put(i, 0);
		}
#endif
		display.clearDisplay();
		showActionPromptScreen(state, &display, "B2", "get dry state");
		int br = readButton();
		while (!(br > BUTTON_2_LOW && br < BUTTON_2_HIGH)) {
			br = readButton();
		}
		display.clearDisplay();
		showTextCaptionScreen(state, &display, "Reading...");

		soundBuzzer(300);

		// waiting for completely wet values
		extractMedianPinValueForProperty(1000, &(*state).s[MS_SENSOR_NEAR].dry, &(*state).s[MS_SENSOR_MID].dry, &(*state).s[MS_SENSOR_FAR].dry);
#ifdef ARDUINO_ARCH_ESP32
		preferences.putInt(MS_NEAR_DRY_SETTING_KEY, (*state).s[MS_SENSOR_NEAR].dry);
		preferences.putInt(MS_MID_DRY_SETTING_KEY, (*state).s[MS_SENSOR_MID].dry);
		preferences.putInt(MS_FAR_DRY_SETTING_KEY, (*state).s[MS_SENSOR_FAR].dry);
#elif
		EEPROM.put(0, (*state).s[near].dry);
		EEPROM.put(sizeof(int), (*state).s[mid].dry);
		EEPROM.put(sizeof(int) * 2, (*state).s[far].dry);
#endif

		// fire buzzer to notify that dry values have been read
		soundBuzzer(1000);
		drawDryValuesScreen(state, &display);
		delay(5000);
		display.clearDisplay();
		showActionPromptScreen(state, &display, "B2", "get wet state");
		br = readButton();
		while (!(br > BUTTON_2_LOW && br < BUTTON_2_HIGH)) {
			br = readButton();
		}
		display.clearDisplay();
		showTextCaptionScreen(state, &display, MS_READING_PROMPT_TEXT);
		soundBuzzer(300);

		// waiting for completely wet values
		extractMedianPinValueForProperty(1000, &(*state).s[MS_SENSOR_NEAR].wet, &(*state).s[MS_SENSOR_MID].wet, &(*state).s[MS_SENSOR_FAR].wet);
#ifdef ARDUINO_ARCH_ESP32
		preferences.putInt(MS_NEAR_WET_SETTING_KEY, (*state).s[MS_SENSOR_NEAR].wet);
		preferences.putInt(MS_MID_WET_SETTING_KEY, (*state).s[MS_SENSOR_MID].wet);
		preferences.putInt(MS_FAR_WET_SETTING_KEY, (*state).s[MS_SENSOR_FAR].wet);
#elif
		EEPROM.put(sizeof(int) * 3, (*state).s[near].wet);
		EEPROM.put(sizeof(int) * 4, (*state).s[mid].wet);
		EEPROM.put(sizeof(int) * 5, (*state).s[far].wet);
#endif


		// wet values have been read
		soundBuzzer(1000);
		drawWetValuesScreen(state, &display);
		delay(5000);
		display.clearDisplay();
		showActionPromptScreen(state, &display, "B2", "start...");
		// click to continue
		br = readButton();
		while (!(br > BUTTON_2_LOW && br < BUTTON_2_HIGH)) {
			br = readButton();
		}

		// initiating standard routine
		soundBuzzer(2000);

		digitalWrite(SENSOR_PIN, SENSOR_PIN_LOW);
	}
#ifdef ARDUINO_ARCH_ESP32
	preferences.end();
#endif

}

bool requestStop(ActionsList* list, Action* a) {
	Action* item = find(list, a);
	if (item != nullptr) {
		(*a).stopRequested = true;
		return true;
	}

	return false;
}

bool canStart(Action* a, unsigned long time) {

	if ((*a).stopRequested) {
		return false;
	}

	if ((*a).state == MS_RUNNING) {
		return false;
	}

	if ((*a).ti > 0 && (*a).lst > 0 && (time - (*a).lst < (*a).ti)) {
		return false;
	}

	return true;
}

bool shouldStop(Action* a, unsigned long time) {

	if ((*a).stopRequested) {
#ifdef DEBUG
		Serial.print(F("Stop req"));
#endif
		return true;
	}

	if ((*a).state == MS_RUNNING) {
		if ((*a).td > 0 && (*a).st > 0 && (time - (*a).st >= (*a).td)) {
			return true;
		}
	}

	return false;
}

void doQueueActions(SystemState* state, ActionsList* list) {
	Action* action = (*list).first;
	unsigned long time = millis();
	int count = (*list).count;

	int i = 0;
	while (action != nullptr) {
		if (canStart(action, time)) {
#ifdef DEBUG
			Serial.print(F("Starting: "));
			Serial.print(indexOfAction(action));
			Serial.println();
#endif

			(*action).start(state, action);
			(*action).state = MS_RUNNING;
			(*action).st = time;

			Action* child = (*action).child;
			while (child != nullptr) {
#ifdef DEBUG
				Serial.print(F("Starting child: "));
				Serial.println(indexOfAction(child));
#endif
				(*child).start(state, child);
				(*child).state = MS_CHILD_RUNNING;
				child = (*child).child;
			}
		}
		else {
			if (shouldStop(action, time)) {

#ifdef DEBUG
				Serial.print(F("Stopping: "));
				Serial.print(indexOfAction(action));
				Serial.println();
#endif

				Action* child = (*action).child;
				while (child != nullptr) {
#ifdef DEBUG
					Serial.print(F("Stopping child: "));
					Serial.println(indexOfAction(child));
#endif
					(*child).stop(state, child);
					(*child).state = MS_NON_ACTIVE;
					child = (*child).child;
				}

				(*action).stop(state, action);
				(*action).lst = time;
				(*action).state = MS_PENDING;
				(*action).clear = !(*action).frozen;
				(*action).stopRequested = false;
			}
			else {
				if ((*action).state == MS_RUNNING) {
					if (time - (*action).st > (*action).to) {
						(*action).tick(state, action);
					}
				}
			}
		}

		action = (*action).next;
	}

	// Iterate over the available actions and stop the
	// appropriate ones
	for (int i = 0; i < ACTIONS_COUNT; i++) {
		Action* a = &availableActions[i];
		if ((*a).clear) {
			descheduleAction(&executionList, a);
			(*a).clear = false;
		}
	}
}


void setup() {
#ifdef ARDUINO_ARCH_ESP32
	ledcSetup(PWMChannel, PWMFreq, PWMRes);
	ledcAttachPin(MS_BUZZER_PIN, PWMChannel);
#endif
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
	pinMode(MS_BUZZER_PIN, OUTPUT); // buzzer

	pinMode(PUMP_PIN, OUTPUT); // pump relay
	digitalWrite(PUMP_PIN, PUMP_PIN_LOW);

	pinMode(SENSOR_PIN, OUTPUT); // sensor relay
	digitalWrite(SENSOR_PIN, SENSOR_PIN_LOW);
	Serial.begin(9600);

	allocateMemPools(&state);

	init(&state);
	setupInitialState(&state);
}

void loop() {
	doQueueActions(&state, &executionList);
}
