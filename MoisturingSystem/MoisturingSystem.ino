
#include "ActionsList.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/Org_01.h>
#include <string.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <soc/sens_reg.h>
#include <soc/soc.h>
#include <ArduinoJson.h>

#ifdef __AVR_ATmega328P__ || __AVR_ATmega168__
#define ARDUINO_ARCH_UNO
#endif

#define MS_SYSTEM_VERSION "0.23"

#define FONT_BASELINE_CORRECTION_NORMAL 6
#define FONT_BASELINE_CORRECTION_LARGE 12
#define MS_FONT_TEXT_SIZE_NORMAL 1
#define MS_FONT_TEXT_SIZE_LARGE 2

const char* MS_OFF_STRING = "OFF";
const char* MS_ON_STRING = "ON";
const char* MS_BACK_BUTTON_PROMPT = "B1 - Back";
const char* MS_READING_PROMPT_TEXT = "Reading...";
const char* MS_STARTING_PROMPT_TEXT = "Starting...";

const char* MS_IRRIGATE_UNTIL_EXPIRY_KEY = "iue";

const char* MS_FAR_ACTIVE_SETTING_KEY = "far-active";
const char* MS_MID_ACTIVE_SETTING_KEY = "mid-active";
const char* MS_NEAR_ACTIVE_SETTING_KEY = "near-active";

const char* MS_FAR_WET_SETTING_KEY = "wet-far";
const char* MS_MID_WET_SETTING_KEY = "wet-mid";
const char* MS_NEAR_WET_SETTING_KEY = "wet-near";

const char* MS_FAR_DRY_SETTING_KEY = "dry-far";
const char* MS_MID_DRY_SETTING_KEY = "dry-mid";
const char* MS_NEAR_DRY_SETTING_KEY = "dry-near";

const char* MS_APV_NEAR_SETTING_KEY = "n-apv";
const char* MS_DAPV_NEAR_SETTING_KEY = "n-dapv";

const char* MS_APV_MID_SETTING_KEY = "m-apv";
const char* MS_DAPV_MID_SETTING_KEY = "m-dapv";

const char* MS_APV_FAR_SETTING_KEY = "f-apv";
const char* MS_DAPV_FAR_SETTING_KEY = "f-dapv";

const char* MS_PUMP_MAX_DURATION_SETTING_KEY = "p-max";
const char* MS_PUMP_REACT_INT_DURATION_SETTING_KEY = "p-react";

const char* MS_SENSOR_INTERVAL_DRY_SETTING_KEY = "s-dry";
const char* MS_SENSOR_INTERVAL_ON_SETTING_KEY = "s-on";
const char* MS_SENSOR_INTERVAL_PUMPING_SETTING_KEY = "s-pumping";

const char* MS_WIFI_TOGGLE_SETTING_KEY = "wifi";

// Mempools

DynamicJsonDocument doc1(1024);
DynamicJsonDocument doc2(1024);
DynamicJsonDocument doc3(1024);
DynamicJsonDocument doc4(1024);


unsigned char stringPool1024b1[1024];
unsigned char stringPool1024b2[1024];
unsigned char stringPool1024b3[1024];
unsigned char stringPool1024b4[1024];

char stringPool50b1[50];
char stringPool50b2[50];
char stringPool50b3[50];
char stringPool50b4[50];

char stringPool30b1[30];
char stringPool30b2[30];
char stringPool30b3[30];
char stringPool30b4[30];

char stringPool20b1[20];
char stringPool20b2[20];
char stringPool20b3[20];
char stringPool20b4[20];

char stringPool10b1[10];
char stringPool10b2[10];
char stringPool10b3[10];
char stringPool10b4[10];

char stringPool5b1[5];
char stringPool5b2[5];
char stringPool5b3[5];
char stringPool5b4[5];

// end of Mempools


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

#define SCREENS_COUNT 19


#define MS_HOME_SCREEN 0
#define MS_SETTINGS_SCREEN 1
#define MS_SENSOR_POWER_SETTINGS_SCREEN 2
#define MS_THRESHOLDS_SETTINGS_SCREEN 3
#define MS_INTERVALS_SETTINGS_SCREEN 4
#define MS_PUMP_INTERVALS_SETTINGS_SCREEN 5
#define MS_SENSOR_INTERVALS_SETTINGS_SCREEN 6
#define MS_PROCESSES_SCREEN 7
#define MS_MENU_SCREEN 8
#define MS_CONNECTIVITY_SETTINGS_SCREEN 9
#define MS_CONNECTIVITY_INFO_SCREEN 10
#define MS_WIFI_TOGGLE_SCREEN 11
#define MS_THRESHOLDS_SETTINGS_MENU_SCREEN 12
#define MS_SENSOR_SETTINGS_MENU_SCREEN 13
#define MS_SENSOR_CALIBRATION_SETTINGS_SCREEN 14
#define MS_CALIBRATION_INFO_SCREEN 15
#define MS_EMPTY_SCREEN 16
#define MS_PUMP_SETTINGS_MENU_SCREEN 17
#define MS_PUMP_CLEANING_INFO_SCREEN 18

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

#define ACTIONS_COUNT 10

// action defines
#define READ_SENSORS_ACTION 0
#define OUTLET_FAR_ACTION 1
#define OUTLET_MID_ACTION 2
#define OUTLET_NEAR_ACTION 3
#define PUMP_ACTION 4
#define DRAW_UI_ACTION 5
#define WIFI_ACTION 6
#define INTERPRET_SENSOR_DATA_ACTION 7
#define CALIBRATE_SENSOR_ACTION 8
#define CLEAN_PUMP_ACTION 9


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

enum WIFIState {
	MS_WIFI_STOPPED = 0,
	MS_WIFI_CONNECTING = 1,
	MS_WIFI_CONNECTED = 2,
	MS_WIFI_DISCONNECTED = 3,
	MS_WIFI_FAILED = 4,
	MS_WIFI_LOST = 5,
	MS_WIFI_IDLE = 6,
	MS_WIFI_NO_SSID = 7,
	MS_WIFI_NO_SHIELD = 8,
	MS_WIFI_SCAN_COMPL = 9
};

struct WiFIState {
	char* ssid = "Mirwais";
	char* password = "importantpassword";
	int state = MS_WIFI_STOPPED;
	bool isActive = false;
} wifi;

const int MS_SENSOR_CALIBRATION_INITIAL_DRY_STATE = 1;
const int MS_SENSOR_CALIBRATION_READ_DRY_STATE = 2;
const int MS_SENSOR_CALIBRATION_INITIAL_WET_STATE = 3;
const int MS_SENSOR_CALIBRATION_READ_WET_STATE = 4;
const int MS_SENSOR_CALIBRATION_STORE_VALUES_STATE = 5;
const int MS_SENSOR_CALIBRATION_FINAL_STATE = 6;

struct SensorsThresholdEditState {
	int sensorCode = -1;
	int state = -1;
	int pin = -1;
	char* settingKey;
} sensorEditState;

struct MSysSettings {
	unsigned long siw = 15000; // 15 seconds for sensor interval while watering
	unsigned long sid = 60000 * 1; // 1 minute for sensor interval while in stand by
	unsigned long sd = 10000; // 10 second? max duration for sensor activation;
	unsigned long pi = 60000 * 10; // 10 minutes between pump activations
	unsigned long pd = 60000 * 2; // 2 minutes max pump on duration
	bool iue = false; //Irrigate until action expiry; false if pump is deactivated once a sensor returns signal; true otherwise;
} settings;

struct Sensor {
	int wet; // completely wet reading
	int dry; // completely dry reading
	int value; // current reading
	int ai; // action index
	int p; // humidity percentage
	int apv;
	int dapv;
	bool active = true;
	char name[4];
};

struct SystemState {
	int scr = MS_HOME_SCREEN; // current screen
	Sensor* s = nullptr;      // array of sensors and their state
	bool p = false;			  // indicates whether the pump is active
	bool sa = false;          // indicates whether the sensors are active
	bool vn = false;          // indicates whether the near outlet is open
	bool vm = false;          // indicates whether the mid outlet is open
	bool vf = false;          // indicates whether the far outlet is open
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

void joinStrings(char** source, int size, char* glue, char* target, int skipGlue) {
	int i = 0;
	while (i < size) {

		if (i > skipGlue) {
			strcat(target, glue);
		}

		strcat(target, source[i]);
		i += 1;
	}
}

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

void drawStartingPromptScreen(Adafruit_SSD1306* display, int remaining) {
	sprintf(stringPool10b1, "(B2 in: %ds)", remaining);
	char* prompt[] = { "Actions:", "B1 - init", "B2 - start", stringPool10b1 };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedTextStack(&canvas, prompt, 4, DEFAULT_TEXT_SIZE, MS_H_LEFT, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void drawDryValuesScreen(Adafruit_SSD1306* display) {
	sprintf(stringPool20b1, "Near: %d", state.s[MS_SENSOR_NEAR].dry);
	sprintf(stringPool20b2, "Mid: %d", state.s[MS_SENSOR_MID].dry);
	sprintf(stringPool20b3, "Far: %d", state.s[MS_SENSOR_FAR].dry);

	char* message[] = { "Dry values:", stringPool20b1, stringPool20b2, stringPool20b3 };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedTextStack(&canvas, message, 4, DEFAULT_TEXT_SIZE, MS_H_LEFT, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void drawWetValuesScreen(Adafruit_SSD1306* display) {
	sprintf(stringPool20b1, "Near: %d", state.s[MS_SENSOR_NEAR].wet);
	sprintf(stringPool20b2, "Mid: %d", state.s[MS_SENSOR_MID].wet);
	sprintf(stringPool20b3, "Far: %d", state.s[MS_SENSOR_FAR].wet);

	char* message[] = { "Wet values:", stringPool20b1, stringPool20b2, stringPool20b3 };
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
}

void showTextCaptionScreen(Adafruit_SSD1306* display, const char* caption) {
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedText(&canvas, caption, MS_FONT_TEXT_SIZE_LARGE, MS_V_CENTER | MS_H_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void showActionPromptScreen(Adafruit_SSD1306* display, char* btn, char* action) {
	sprintf(stringPool20b1, "Press %s", btn);
	char* message[] = { stringPool20b1, action };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	printAlignedTextStack(&canvas, message, 2, DEFAULT_TEXT_SIZE, MS_H_CENTER, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

void drawSplashScreen(Adafruit_SSD1306* display) {
	sprintf(stringPool10b1, "Version: %s", MS_SYSTEM_VERSION);
	char* texts[] = { "Irrigation", "System", stringPool10b1 };
	GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&canvas);
	canvas.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 10, SSD1306_WHITE);
	printAlignedTextStack(&canvas, texts, 3, DEFAULT_TEXT_SIZE, MS_H_CENTER, MS_H_CENTER | MS_V_CENTER);
	(*display).drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
	(*display).display();
}

// end of Display


void extractMedianPinValueForProperty(int delayInterval, int* target, int sensorPin) {
	int value = 0;
	int iterations = 10;

	for (int i = 0; i < iterations; i++) {
		value += fixedAnalogRead(sensorPin);
		if (delayInterval > 0) {
			delay(delayInterval);
		}
	}

	(*target) = (int)(value / iterations);
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

bool canStartOutlet(Action* a) {
	return availableActions[CLEAN_PUMP_ACTION].state != MS_RUNNING && availableActions[CALIBRATE_SENSOR_ACTION].state != MS_RUNNING;
}

//

// wifi
WebServer server(80);

char* _resolveWiFIStatusString(char* target, int status) {
	char* message;
	switch (wifi.state) {
	case MS_WIFI_CONNECTED:
		message = "connected";
		break;
	case MS_WIFI_DISCONNECTED:
		message = "disconnected";
		break;
	case MS_WIFI_FAILED:
		message = "failed";
		break;
	case MS_WIFI_LOST:
		message = "lost";
		break;
	case MS_WIFI_IDLE:
		message = "idle";
		break;
	case MS_WIFI_NO_SSID:
		message = "no ssid";
		break;
	case MS_WIFI_SCAN_COMPL:
		message = "scan compl";
		break;
	case MS_WIFI_NO_SHIELD:
		message = "no shield";
		break;
	default:
		message = "off";
		break;
	}

	sprintf(target, "%s%s", message);
	return target;
}


void connectWiFi() {
	WiFi.mode(WIFI_STA);
	WiFi.setAutoConnect(true);
	WiFi.persistent(true);
	WiFi.setHostname("MS-System");
	WiFi.begin(wifi.ssid, wifi.password);
}
bool serverActivated = false;

void updateWiFiStatus() {
	switch (WiFi.status()) {
	case WL_CONNECTED:
		wifi.state = MS_WIFI_CONNECTED;
		break;
	case WL_DISCONNECTED:
		wifi.state = MS_WIFI_DISCONNECTED;
		break;
	case WL_CONNECT_FAILED:
		wifi.state = MS_WIFI_FAILED;
		break;
	case WL_CONNECTION_LOST:
		wifi.state = MS_WIFI_LOST;
		break;
	case WL_IDLE_STATUS:
		wifi.state = MS_WIFI_IDLE;
		break;
	case WL_NO_SSID_AVAIL:
		wifi.state = MS_WIFI_NO_SSID;
		break;
	case WL_SCAN_COMPLETED:
		wifi.state = MS_WIFI_SCAN_COMPL;
		break;
	case WL_NO_SHIELD:
		wifi.state = MS_WIFI_NO_SHIELD;
		break;
	}
}

bool _requestAuth() {
	if (!server.authenticate("ms-system-admin", "importantpassword")) {
		server.requestAuthentication();
		return false;
	}
	return true;
}

unsigned long  _calculateOnBeforeTime(unsigned long curTime, Action* a) {
	int lastStopTime = (*a).lst;
	int onBeforeTime = lastStopTime > 0 ? curTime - lastStopTime : 0;
	return onBeforeTime;
}

void _getActionStateString(int state, char* target) {
	switch (state) {
	case MS_NON_ACTIVE:
		sprintf(target, "non-active");
		break;
	case MS_PENDING:
		sprintf(target, "pending");
		break;
	case MS_SCHEDULED:
		sprintf(target, "scheduled");
		break;
	case MS_RUNNING:
		sprintf(target, "running");
		break;
	case MS_CHILD_RUNNING:
		sprintf(target, "child-running");
		break;
	case MS_CHILD_PENDING:
		sprintf(target, "child-pending");
		break;
	case MS_CHILD_SCHEDULED:
		sprintf(target, "child-scheduled");
		break;
	}
}

void _generateStatus(DynamicJsonDocument* doc) {
	(*doc)["status"] = "OK";
	unsigned long time = millis();
	(*doc)["pump"]["active"] = state.p;
	(*doc)["pump"]["di_sec"] = settings.pd / 1000;
	(*doc)["pump"]["offtime_sec"] = settings.pi / 1000;
	(*doc)["pump"]["iue"] = settings.iue;
	(*doc)["sensors"]["di_sec"] = settings.sid / 1000;
	(*doc)["sensors"]["p_di_sec"] = settings.siw / 1000;
	(*doc)["sensors"]["ontime_sec"] = availableActions[READ_SENSORS_ACTION].td / 1000;
	(*doc)["sensors"]["on_before_mins"] = (int)(_calculateOnBeforeTime(time, &availableActions[READ_SENSORS_ACTION]) / 60000);

	for (int i = 0; i < SENSORS_COUNT; i++) {
		Sensor* cur = &state.s[i];
		bool isActive = (*cur).active;
		(*doc)["sensors"][(*cur).name]["active"] = (*cur).active;
		if (isActive) {
			(*doc)["sensors"][(*cur).name]["hum_perc"] = (*cur).p;
			(*doc)["sensors"][(*cur).name]["readings"]["dry_val"] = (*cur).dry;
			(*doc)["sensors"][(*cur).name]["readings"]["wet_val"] = (*cur).wet;
			(*doc)["sensors"][(*cur).name]["readings"]["cur_val"] = (*cur).value;
			(*doc)["sensors"][(*cur).name]["on_before_mins"] = (int)(_calculateOnBeforeTime(time, &availableActions[(*cur).ai]) / 60000);
		}

		(*doc)["sensors"][(*cur).name]["thresholds"]["apv"] = (*cur).apv;
		(*doc)["sensors"][(*cur).name]["thresholds"]["dapv"] = (*cur).dapv;
	}

	for (int i = 0; i < ACTIONS_COUNT; i++) {
		Action* cur = &availableActions[i];
		_getActionStateString((*cur).state, stringPool20b1);
		(*doc)["actions"][(*cur).name] = stringPool20b1;
	}
}

void handleStatus() {
	_generateStatus(&doc1);
	serializeJson(doc1, stringPool1024b1);
	server.send(200, "application/json", (char*)stringPool1024b1);
	doc1.clear();
	memset(stringPool1024b1, 0, 1024);
}

void handleModifySetting() {
	if (_requestAuth()) {

		String body = server.arg("plain");
		body.getBytes(stringPool1024b1, 1024, 0);
		deserializeJson(doc1, stringPool1024b1);

		if (doc1.containsKey(MS_APV_NEAR_SETTING_KEY)) {
			state.s[MS_SENSOR_NEAR].apv = max(0, (int)min((int)doc1[MS_APV_NEAR_SETTING_KEY], state.s[MS_SENSOR_NEAR].dapv - 5));
		}

		if (doc1.containsKey(MS_DAPV_NEAR_SETTING_KEY)) {
			state.s[MS_SENSOR_NEAR].dapv = max(state.s[MS_SENSOR_NEAR].apv + 5, min((int)doc1[MS_DAPV_NEAR_SETTING_KEY], 100));
		}

		if (doc1.containsKey(MS_APV_MID_SETTING_KEY)) {
			state.s[MS_SENSOR_MID].apv = max(0, (int)min((int)doc1[MS_APV_MID_SETTING_KEY], state.s[MS_SENSOR_MID].dapv - 5));
		}

		if (doc1.containsKey(MS_DAPV_MID_SETTING_KEY)) {
			state.s[MS_SENSOR_MID].dapv = max(state.s[MS_SENSOR_MID].apv + 5, min((int)doc1[MS_DAPV_MID_SETTING_KEY], 100));
		}

		if (doc1.containsKey(MS_APV_FAR_SETTING_KEY)) {
			state.s[MS_SENSOR_FAR].apv = max(0, (int)min((int)doc1[MS_APV_FAR_SETTING_KEY], state.s[MS_SENSOR_FAR].dapv - 5));
		}

		if (doc1.containsKey(MS_DAPV_FAR_SETTING_KEY)) {
			state.s[MS_SENSOR_FAR].dapv = max(state.s[MS_SENSOR_FAR].apv + 5, min((int)doc1[MS_DAPV_FAR_SETTING_KEY], 100));
		}

		if (doc1.containsKey(MS_PUMP_MAX_DURATION_SETTING_KEY)) {
			settings.pd = min(5 * 60000, max((int)doc1[MS_PUMP_MAX_DURATION_SETTING_KEY], 60000));
		}

		if (doc1.containsKey(MS_PUMP_REACT_INT_DURATION_SETTING_KEY)) {
			settings.pi = min(20 * 60000, max((int)doc1[MS_PUMP_REACT_INT_DURATION_SETTING_KEY], 60000));
		}

		if (doc1.containsKey(MS_NEAR_ACTIVE_SETTING_KEY)) {
			state.s[MS_SENSOR_NEAR].active = (doc1[MS_NEAR_ACTIVE_SETTING_KEY] == true ? true : false);
		}

		if (doc1.containsKey(MS_MID_ACTIVE_SETTING_KEY)) {
			state.s[MS_SENSOR_MID].active = (doc1[MS_MID_ACTIVE_SETTING_KEY] == true ? true : false);
		}

		if (doc1.containsKey(MS_FAR_ACTIVE_SETTING_KEY)) {
			state.s[MS_SENSOR_FAR].active = (doc1[MS_FAR_ACTIVE_SETTING_KEY] == true ? true : false);
		}

		if (doc1.containsKey(MS_IRRIGATE_UNTIL_EXPIRY_KEY)) {
			settings.iue = doc1[MS_IRRIGATE_UNTIL_EXPIRY_KEY] == true ? true : false;
		}

		_generateStatus(&doc2);
		serializeJson(doc2, stringPool1024b2);
		server.send(200, "application/json", (char*)stringPool1024b2);

		doc1.clear();
		doc2.clear();
		memset(stringPool1024b1, 0, 1024);
		memset(stringPool1024b2, 0, 1024);

		storeSetPreferences();
	}
}

void handleCommandRestart() {
	if (_requestAuth()) {
		server.send(200, "text/plain", "Restaring...");
		delay(2000);
#ifdef ARDUINO_ARCH_ESP32
		ESP.restart();
#endif
	}
}


void setupWebServer() {
	server.on("/api/status", HTTP_GET, &handleStatus);
	server.on("/api/modify/setting", HTTP_POST, &handleModifySetting);
	server.on("/api/command/restart", HTTP_POST, &handleCommandRestart);
	server.onNotFound([]() {server.send(404, "text/plain", "Sorry ;)"); });
	server.begin();
}

void startWifi(Action* a) {
	connectWiFi();
	setupWebServer();
}

void tickWifi(Action* a) {
	updateWiFiStatus();

	if (wifi.state == MS_WIFI_IDLE) {
		WiFi.reconnect();
	}

	server.handleClient();
}

void stopWifi(Action* a) {
	server.stop();
	WiFi.disconnect();
	WiFi.mode(WIFI_OFF);
	wifi.state = MS_WIFI_STOPPED;
}

// end of wifi

// Sensors

void startInterpret(Action* a) {
}

void stopInterpret(Action* a) {
}

void tickInterpret(Action* a) {
	bool activate = false;
	Action** acandidates = (Action**)malloc(sizeof(Action*) * SENSORS_COUNT);
	if (acandidates != nullptr) {
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

			activate = (*se).active && (bool)((long)mp < ((*ca).state == MS_RUNNING ? (*se).dapv : (*se).apv));

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
			else if (!settings.iue && ca != nullptr) {
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
		bool isPumpOpen = availableActions[PUMP_ACTION].state == MS_CHILD_RUNNING || availableActions[PUMP_ACTION].state == MS_CHILD_SCHEDULED;
		availableActions[READ_SENSORS_ACTION].ti = isPumpOpen ? settings.siw : settings.sid;

		free(acandidates);
	}
}


bool calibrateSensorCanStart(Action* a) {
	return availableActions[READ_SENSORS_ACTION].state != MS_RUNNING && availableActions[PUMP_ACTION].state != MS_RUNNING;
}

void startCalibrateSensor(Action* a) {
	digitalWrite(SENSOR_PIN, SENSOR_PIN_HIGH);
	state.sa = true;
}

void tickCalibrateSensor(Action* a) {
	int startTime = (*a).st;
	int currentTime = millis();

	switch (sensorEditState.state)
	{
	case MS_SENSOR_CALIBRATION_READ_DRY_STATE:
		if (currentTime - startTime < 10000) {
			extractMedianPinValueForProperty(1, &state.s[sensorEditState.sensorCode].dry, sensorEditState.pin);
		}
		else {
			sensorEditState.state = MS_SENSOR_CALIBRATION_INITIAL_WET_STATE;
			requestStop(&executionList, a);
		}
		break;
	case MS_SENSOR_CALIBRATION_READ_WET_STATE:
		if (currentTime - startTime < 10000) {
			extractMedianPinValueForProperty(0, &state.s[sensorEditState.sensorCode].wet, sensorEditState.pin);
		}
		else {
			sensorEditState.state = MS_SENSOR_CALIBRATION_STORE_VALUES_STATE;
		}
		break;

	case MS_SENSOR_CALIBRATION_STORE_VALUES_STATE:
	{
		preferences.begin(MS_PREFERENCES_ID, false);
		preferences.putInt(MS_NEAR_DRY_SETTING_KEY, state.s[MS_SENSOR_NEAR].dry);
		preferences.putInt(MS_MID_DRY_SETTING_KEY, state.s[MS_SENSOR_MID].dry);
		preferences.putInt(MS_FAR_DRY_SETTING_KEY, state.s[MS_SENSOR_FAR].dry);
		preferences.putInt(MS_NEAR_WET_SETTING_KEY, state.s[MS_SENSOR_NEAR].wet);
		preferences.putInt(MS_MID_WET_SETTING_KEY, state.s[MS_SENSOR_MID].wet);
		preferences.putInt(MS_FAR_WET_SETTING_KEY, state.s[MS_SENSOR_FAR].wet);
		preferences.end();
		sensorEditState.state = MS_SENSOR_CALIBRATION_FINAL_STATE;
		requestStop(&executionList, a);
	}
	break;
	}
}

void stopCalibrateSensor(Action* a) {
	digitalWrite(SENSOR_PIN, SENSOR_PIN_LOW);
	state.sa = false;
}


bool readSensorsCanStart(Action* a) {
	return availableActions[CALIBRATE_SENSOR_ACTION].state != MS_RUNNING;
}

void startSensors(Action* a) {
	digitalWrite(SENSOR_PIN, SENSOR_PIN_HIGH);
	state.sa = true;
}

void stopSensors(Action* a) {
	digitalWrite(SENSOR_PIN, SENSOR_PIN_LOW);
	state.sa = false;
	scheduleAction(&executionList, &availableActions[INTERPRET_SENSOR_DATA_ACTION]);
}

void tickSensors(Action* a) {
	extractMedianPinValueForProperty(0, &state.s[MS_SENSOR_NEAR].value, PIN_NEAR);
	extractMedianPinValueForProperty(0, &state.s[MS_SENSOR_MID].value, PIN_MID);
	extractMedianPinValueForProperty(0, &state.s[MS_SENSOR_FAR].value, PIN_FAR);
}

//end of sensors

// Display

const char* _hsResolveSensorInfo(char* target, Sensor* s) {
	if ((*s).active) {
		sprintf(target, "%d%%", (*s).p);
	}
	else {
		return MS_OFF_STRING;
	}
	return target;
}

void drawEmptyScreen(Action* a) {
	display.clearDisplay();
	display.display();
}

void handleEmptyScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_HOME_SCREEN;
	}
}

void drawHomeScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	sprintf(stringPool30b1, "N: %s M: %s F: %s", _hsResolveSensorInfo(stringPool5b1, &state.s[MS_SENSOR_NEAR]), _hsResolveSensorInfo(stringPool5b2, &state.s[MS_SENSOR_MID]), _hsResolveSensorInfo(stringPool5b3, &state.s[MS_SENSOR_FAR]));
	sprintf(stringPool30b2, "VN: %s VM: %s VF: %s", state.vn ? MS_ON_STRING : MS_OFF_STRING, state.vm ? MS_ON_STRING : MS_OFF_STRING, state.vf ? MS_ON_STRING : MS_OFF_STRING);
	sprintf(stringPool30b3, "PUMP: %s SENSORS: %s", state.p ? MS_ON_STRING : MS_OFF_STRING, state.sa ? MS_ON_STRING : MS_OFF_STRING);
	sprintf(stringPool20b1, "WIFI: %s", _resolveWiFIStatusString(stringPool20b2, wifi.state));
	char* message[] = { stringPool30b1, stringPool30b2, stringPool30b3, stringPool20b1 };
	printAlignedTextStack(&mainCanvas, message, 4, 1, MS_H_CENTER, MS_H_CENTER | MS_V_TOP);
	printAlignedText(&mainCanvas, "B1 - menu, B2 - dim", 1, (MS_H_CENTER | MS_V_BOTTOM));
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleHomeScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_MENU_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		state.scr = MS_EMPTY_SCREEN;
	}
}

void drawThresholdsSettingsMenuScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	char* text[] = { "B2 - Edit NEAR", "B3 - Edit MID", "B4 - Edit FAR" };
	printAlignedTextStack(&mainCanvas, text, 3, 1, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);
	printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleThresholdsSettingsMenuScreen(int buttonValue) {

	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_SETTINGS_SCREEN;
		sensorEditState.sensorCode = -1;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		sensorEditState.sensorCode = MS_SENSOR_NEAR;
		state.scr = MS_THRESHOLDS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		sensorEditState.sensorCode = MS_SENSOR_MID;
		state.scr = MS_THRESHOLDS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_4_LOW && buttonValue < BUTTON_4_HIGH) {
		sensorEditState.sensorCode = MS_SENSOR_FAR;
		state.scr = MS_THRESHOLDS_SETTINGS_SCREEN;
	}
}

void drawThresholdsSettingsScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);

	Sensor* current = &state.s[sensorEditState.sensorCode];

	sprintf(stringPool20b3, "Editing: %s", (*current).name);
	sprintf(stringPool20b1, "APV: %d%%", (*current).apv);
	sprintf(stringPool20b2, "DAPV: %d%%", (*current).dapv);
	char* message[] = { stringPool20b3, stringPool20b1, stringPool20b2 };
	printAlignedTextStack(&mainCanvas, message, 3, MS_FONT_TEXT_SIZE_LARGE, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);

	printAlignedText(&mainCanvas, "B1 - Back, B2-B3 - edit", MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleThresholdsSettingsScreen(int buttonValue) {
	Sensor* current = &state.s[sensorEditState.sensorCode];
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_THRESHOLDS_SETTINGS_MENU_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		int upperLimit = (*current).dapv - 5;
		int nv = _min(((*current).apv + 5) % (*current).dapv, upperLimit);
		(*current).apv = nv;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		int nv = _max(_min(((*current).dapv + 5) % 105, 100), (*current).apv + 5);
		(*current).dapv = nv;

	}

	storeSetPreferences();
}

void drawCalibrationInfoScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	Sensor* s = &state.s[sensorEditState.sensorCode];
	switch (sensorEditState.state)
	{
	case MS_SENSOR_CALIBRATION_INITIAL_DRY_STATE:
	{
		char* message1[] = { "Press B2", "to read", "DRY value" };
		printAlignedTextStack(&mainCanvas, message1, 3, MS_FONT_TEXT_SIZE_LARGE, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);
		printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	}
	break;
	case MS_SENSOR_CALIBRATION_READ_DRY_STATE:
	{
		sprintf(stringPool20b2, "Sensor: %s", (*s).name);
		sprintf(stringPool20b3, "Value: %d", (*s).dry);
		char* message2[] = { stringPool20b2,"Type: DRY", stringPool20b3 };
		printAlignedTextStack(&mainCanvas, message2, 3, MS_FONT_TEXT_SIZE_LARGE, MS_H_LEFT, MS_H_CENTER | MS_V_CENTER);
	}
	break;
	case MS_SENSOR_CALIBRATION_INITIAL_WET_STATE:
	{
		char* message3[] = { "Press B2", "to read", "WET value" };
		printAlignedTextStack(&mainCanvas, message3, 3, MS_FONT_TEXT_SIZE_LARGE, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);
		printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	}
	break;
	case MS_SENSOR_CALIBRATION_READ_WET_STATE:
	{
		sprintf(stringPool20b2, "Sensor: %s", (*s).name);
		sprintf(stringPool20b3, "Value: %d", (*s).wet);
		char* message4[] = { stringPool20b2, "Type: WET", stringPool20b3 };
		printAlignedTextStack(&mainCanvas, message4, 3, MS_FONT_TEXT_SIZE_LARGE, MS_H_LEFT, MS_H_CENTER | MS_V_CENTER);
	}
	break;
	case MS_SENSOR_CALIBRATION_FINAL_STATE:
	{
		char* message5[] = { "Press B1", "to exit" };
		printAlignedTextStack(&mainCanvas, message5, 2, MS_FONT_TEXT_SIZE_LARGE, MS_H_LEFT, MS_H_CENTER | MS_V_CENTER);
	}
	break;
	}
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleCalibrationInfoScreen(int buttonValue) {

	if (sensorEditState.state == MS_SENSOR_CALIBRATION_INITIAL_DRY_STATE) {
		if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
			sensorEditState.state = MS_SENSOR_CALIBRATION_READ_DRY_STATE;
			scheduleAction(&executionList, &availableActions[CALIBRATE_SENSOR_ACTION]);
		}
		else if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
			state.scr = MS_SENSOR_CALIBRATION_SETTINGS_SCREEN;
		}
	}

	if (sensorEditState.state == MS_SENSOR_CALIBRATION_INITIAL_WET_STATE) {
		if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
			sensorEditState.state = MS_SENSOR_CALIBRATION_READ_WET_STATE;
			scheduleAction(&executionList, &availableActions[CALIBRATE_SENSOR_ACTION]);
		}
		else if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
			state.scr = MS_SENSOR_CALIBRATION_SETTINGS_SCREEN;
		}
	}

	if (sensorEditState.state == MS_SENSOR_CALIBRATION_FINAL_STATE) {
		if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
			state.scr = MS_SENSOR_CALIBRATION_SETTINGS_SCREEN;
		}
	}
}

void drawSensorSettingsCalibrationScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	char* text[] = { "B2 - Near", "B3 - Mid", "B4 - Far" };
	printAlignedTextStack(&mainCanvas, text, 3, 1, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);
	printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleSensorSettingsCalibrationScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_SENSOR_SETTINGS_MENU_SCREEN;
		sensorEditState.sensorCode = -1;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		sensorEditState.sensorCode = MS_SENSOR_NEAR;
		sensorEditState.pin = PIN_NEAR;
		sensorEditState.state = MS_SENSOR_CALIBRATION_INITIAL_DRY_STATE;
		state.scr = MS_CALIBRATION_INFO_SCREEN;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		sensorEditState.sensorCode = MS_SENSOR_MID;
		sensorEditState.pin = PIN_MID;
		sensorEditState.state = MS_SENSOR_CALIBRATION_INITIAL_DRY_STATE;
		state.scr = MS_CALIBRATION_INFO_SCREEN;
	}
	else if (buttonValue > BUTTON_4_LOW && buttonValue < BUTTON_4_HIGH) {
		sensorEditState.sensorCode = MS_SENSOR_FAR;
		sensorEditState.pin = PIN_FAR;
		sensorEditState.state = MS_SENSOR_CALIBRATION_INITIAL_DRY_STATE;
		state.scr = MS_CALIBRATION_INFO_SCREEN;
	}
}

void drawSensorSettingsMenuScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	char* text[] = { "B2 - Turn On/Off", "B3 - Calibrate", "B4 - Intervals" };
	printAlignedTextStack(&mainCanvas, text, 3, 1, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);
	printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleSensorSettingsMenuScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_SETTINGS_SCREEN;
		sensorEditState.sensorCode = -1;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		sensorEditState.sensorCode = MS_SENSOR_NEAR;
		state.scr = MS_SENSOR_POWER_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		sensorEditState.sensorCode = MS_SENSOR_MID;
		state.scr = MS_SENSOR_CALIBRATION_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_4_LOW && buttonValue < BUTTON_4_HIGH) {
		sensorEditState.sensorCode = MS_SENSOR_MID;
		state.scr = MS_SENSOR_INTERVALS_SETTINGS_SCREEN;
	}
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
		sprintf(stringPool10b1, "%d%%", (*s).p);
		mainCanvas.getTextBounds(isActive ? stringPool10b1 : MS_OFF_STRING, 0, 0, &x, &y, &w, &h);
		mainCanvas.setCursor(boxX - w / 2 + w % 2, boxY - h / 2 + h % 2 + FONT_BASELINE_CORRECTION_NORMAL / 2);
		mainCanvas.setTextColor(isActive ? SSD1306_BLACK : SSD1306_WHITE);
		mainCanvas.setTextSize(MS_FONT_TEXT_SIZE_NORMAL);

		if (isActive) {
			mainCanvas.print(stringPool10b1);
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
		state.scr = MS_SENSOR_SETTINGS_MENU_SCREEN;
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

	storeSetPreferences();
}

void drawWIFIToggleScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	const char* wifiCaption = "WIFI";
	int circleRadius = 17;
	int spacing = 3;
	int boxWidth = 2 * spacing + (circleRadius * 2);
	int boxX = SCREEN_WIDTH / 2 - boxWidth / 2 + circleRadius;
	int boxY = circleRadius;

	bool isActive = availableActions[WIFI_ACTION].state == MS_RUNNING ? true : false;
	if (isActive) {
		mainCanvas.fillCircle(boxX, boxY, circleRadius, SSD1306_WHITE);
	}
	else {
		mainCanvas.drawCircle(boxX, boxY, circleRadius, SSD1306_WHITE);
	}
	int16_t x, y;
	uint16_t w, h;
	sprintf(stringPool10b1, "%s", isActive ? "on" : "off");
	mainCanvas.getTextBounds(stringPool10b1, 0, 0, &x, &y, &w, &h);
	mainCanvas.setCursor(boxX - w / 2 + w % 2, boxY - h / 2 + h % 2 + FONT_BASELINE_CORRECTION_NORMAL / 2);
	mainCanvas.setTextColor(isActive ? SSD1306_BLACK : SSD1306_WHITE);
	mainCanvas.setTextSize(MS_FONT_TEXT_SIZE_NORMAL);
	mainCanvas.print(stringPool10b1);
	mainCanvas.getTextBounds(wifiCaption, 0, 0, &x, &y, &w, &h);
	mainCanvas.setTextColor(SSD1306_WHITE);
	printPositionedText(&mainCanvas, wifiCaption, boxX - w / 2, boxY + circleRadius + spacing + FONT_BASELINE_CORRECTION_NORMAL);

	boxX += circleRadius * 2 + spacing;

	printAlignedText(&mainCanvas, "B1 - Back, B3 - toggle", MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleWIFIToggleScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_CONNECTIVITY_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		if (availableActions[WIFI_ACTION].state == MS_RUNNING) {
			requestStop(&executionList, &availableActions[WIFI_ACTION]);
			wifi.isActive = false;
		}
		else if (availableActions[WIFI_ACTION].state == MS_NON_ACTIVE) {
			scheduleAction(&executionList, &availableActions[WIFI_ACTION]);
			wifi.isActive = true;
		}
	}

	storeSetPreferences();
}

void drawConnectivityInfoScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	if (wifi.state != MS_WIFI_STOPPED) {
		String ip = WiFi.localIP().toString();
		ip.getBytes((unsigned char*)stringPool20b4, 20, 0);
		sprintf(stringPool20b1, "IP: %s", stringPool20b4);
		sprintf(stringPool20b2, "SSID: %s", WiFi.SSID());
		sprintf(stringPool20b3, "Host: %s", WiFi.getHostname());
		char* text[] = { stringPool20b1, stringPool20b2, stringPool20b3 };
		printAlignedTextStack(&mainCanvas, text, 3, 1, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);
	}
	else {
		printAlignedText(&mainCanvas, "WIFI: OFF", MS_FONT_TEXT_SIZE_LARGE, MS_H_CENTER | MS_V_CENTER);
	}
	printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleConnectivityInfoScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_CONNECTIVITY_SETTINGS_SCREEN;
	}
}

void drawConnectivitySettingsScreen(Action* a) {

	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	char* text[] = { "B2 - WiFi On/Off", "B3 - Network info" };
	printAlignedTextStack(&mainCanvas, text, 2, 1, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);
	printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleConnectivitySettingsScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_MENU_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		state.scr = MS_WIFI_TOGGLE_SCREEN;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		state.scr = MS_CONNECTIVITY_INFO_SCREEN;
	}
}

void drawMenuScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	char* text[] = { "B2 - Settings", "B3 - Processes", "B4 - Connectivity" };
	printAlignedTextStack(&mainCanvas, text, 3, 1, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);
	printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleMenuScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_HOME_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		state.scr = MS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		state.scr = MS_PROCESSES_SCREEN;
	}
	else if (buttonValue > BUTTON_4_LOW && buttonValue < BUTTON_4_HIGH) {
		state.scr = MS_CONNECTIVITY_SETTINGS_SCREEN;
	}
}


char** running = (char**)calloc(sizeof(char*), ACTIONS_COUNT);
char** stopped = (char**)calloc(sizeof(char*), ACTIONS_COUNT);
char** pending = (char**)calloc(sizeof(char*), ACTIONS_COUNT);
char** scheduled = (char**)calloc(sizeof(char*), ACTIONS_COUNT);

void drawProcessesScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);

	int stoppedCount = 0;
	int pendingCount = 0;
	int scheduledCount = 0;
	int runningCount = 0;

	pending[pendingCount++] = "P: ";
	scheduled[scheduledCount++] = "SC: ";
	running[runningCount++] = "R: ";
	stopped[stoppedCount++] = "S: ";

	for (int i = 0; i < ACTIONS_COUNT; i++) {
		Action* current = &availableActions[i];
		switch ((*current).state) {
		case MS_PENDING:
		case MS_CHILD_PENDING:
			pending[pendingCount++] = (*current).name;
			break;
		case MS_SCHEDULED:
			scheduled[scheduledCount++] = (*current).name;
			break;
		case MS_RUNNING:
		case MS_CHILD_RUNNING:
			running[runningCount++] = (*current).name;
			break;
		case MS_NON_ACTIVE:
			stopped[stoppedCount++] = (*current).name;
			break;
		}
	}
	joinStrings(pending, pendingCount, ", ", stringPool50b1, 1);
	joinStrings(stopped, stoppedCount, ", ", stringPool50b2, 1);
	joinStrings(scheduled, scheduledCount, ", ", stringPool50b3, 1);
	joinStrings(running, runningCount, ", ", stringPool50b4, 1);

	char* message[] = { stringPool50b1, stringPool50b2, stringPool50b3, stringPool50b4 };

	printAlignedTextStack(&mainCanvas, message, 4, MS_FONT_TEXT_SIZE_NORMAL, MS_H_LEFT, MS_H_LEFT | MS_V_TOP);

	memset(stringPool50b1, 0, pendingCount);
	memset(stringPool50b2, 0, stoppedCount);
	memset(stringPool50b3, 0, scheduledCount);
	memset(stringPool50b4, 0, runningCount);

	printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleProcessesScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_MENU_SCREEN;
	}
}

void drawSettingsScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	char* text[] = { "B2 - Pump", "B3 - Sensors", "B4 - Thresholds" };
	printAlignedTextStack(&mainCanvas, text, 3, 1, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);
	printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleSettingsScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_MENU_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		state.scr = MS_PUMP_SETTINGS_MENU_SCREEN;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		state.scr = MS_SENSOR_SETTINGS_MENU_SCREEN;
	}
	else if (buttonValue > BUTTON_4_LOW && buttonValue < BUTTON_4_HIGH) {
		state.scr = MS_THRESHOLDS_SETTINGS_MENU_SCREEN;
	}
}

void drawSensorIntervalsSettingsScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);

	sprintf(stringPool20b1, "Pump off: %d s", settings.sid / 1000);
	sprintf(stringPool20b2, "Pump on: %d s", settings.siw / 1000);
	sprintf(stringPool20b3, "ON duration: %d s", availableActions[READ_SENSORS_ACTION].td / 1000);
	char* message[] = { stringPool20b1, stringPool20b2, stringPool20b3 };
	printAlignedTextStack(&mainCanvas, message, 3, MS_FONT_TEXT_SIZE_NORMAL, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);

	printAlignedText(&mainCanvas, "B1 - Back, B2-B4 - Edit", MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handleSensorIntervalsSettingsScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_SENSOR_SETTINGS_MENU_SCREEN;
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
	else if (buttonValue > BUTTON_4_LOW && buttonValue < BUTTON_4_HIGH) {
		int step = 1000;
		int upperLimit = 15 * step;
		unsigned long* td = &availableActions[READ_SENSORS_ACTION].td;
		int nv = _max(((*td) + step) % (upperLimit + step), step);
		(*td) = nv;
	}

	storeSetPreferences();
}

void drawPumpSettingsScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	char* text[] = { "B2 - Intervals", "B3 - Clean" };
	printAlignedTextStack(&mainCanvas, text, 2, 1, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);
	printAlignedText(&mainCanvas, MS_BACK_BUTTON_PROMPT, MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handlePumpSettingsScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		state.scr = MS_PUMP_INTERVALS_SETTINGS_SCREEN;
	}
	else if (buttonValue > BUTTON_3_LOW && buttonValue < BUTTON_3_HIGH) {
		state.scr = MS_PUMP_CLEANING_INFO_SCREEN;
	}
}

void drawPumpIntervalsSettingsScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	sprintf(stringPool20b1, "max(T): %d min", settings.pd / 60000);
	sprintf(stringPool20b2, "Re-act in: %d min", settings.pi / 60000);
	char* message[] = { stringPool20b1, stringPool20b2 };
	printAlignedTextStack(&mainCanvas, message, 2, MS_FONT_TEXT_SIZE_NORMAL, MS_H_LEFT, MS_H_CENTER | MS_V_TOP);

	printAlignedText(&mainCanvas, "B1 - Back, B2-B3 - edit", MS_FONT_TEXT_SIZE_NORMAL, MS_H_CENTER | MS_V_BOTTOM);
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handlePumpIntervalsSettingsScreen(int buttonValue) {
	if (buttonValue > BUTTON_1_LOW && buttonValue < BUTTON_1_HIGH) {
		state.scr = MS_PUMP_SETTINGS_MENU_SCREEN;
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


	storeSetPreferences();
}


void drawPumpCleaningInfoScreen(Action* a) {
	GFXcanvas16 mainCanvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
	initCanvas(&mainCanvas);
	Action* ca = &availableActions[CLEAN_PUMP_ACTION];
	if ((*ca).state == MS_NON_ACTIVE) {
		char* message1[] = { "Press B2", "to", "start" };
		printAlignedTextStack(&mainCanvas, message1, 3, MS_FONT_TEXT_SIZE_LARGE, MS_H_CENTER, MS_H_CENTER | MS_V_CENTER);
	}
	else {
		char* message1[] = { "Press B2", "to", "stop" };
		printAlignedTextStack(&mainCanvas, message1, 3, MS_FONT_TEXT_SIZE_LARGE, MS_H_CENTER, MS_H_CENTER | MS_V_CENTER);
	}
	display.drawRGBBitmap(0, 0, mainCanvas.getBuffer(), mainCanvas.width(), mainCanvas.height());
	display.display();
}

void handlePumpCleaningInfoScreen(int buttonValue) {
	Action* ca = &availableActions[CLEAN_PUMP_ACTION];

	if (buttonValue > BUTTON_2_LOW && buttonValue < BUTTON_2_HIGH) {
		if ((*ca).state == MS_NON_ACTIVE) {
			scheduleAction(&executionList, ca);
		}
		else {
			state.scr = MS_PUMP_SETTINGS_MENU_SCREEN;
			requestStop(&executionList, ca);
		}
	}
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
	if (screenIndex >= 0 && screenIndex < SCREENS_COUNT) {
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
bool cleanPumpCanStart(Action* a) {
	return availableActions[PUMP_ACTION].state != MS_RUNNING;
}

void startCleanPump(Action* a) {
	digitalWrite(PUMP_PIN, PUMP_PIN_HIGH);
	state.p = true;
}

void tickCleanPump(Action* a) {
}

void stopCleanPump(Action* a) {
	digitalWrite(PUMP_PIN, PUMP_PIN_LOW);
	state.p = false;
}

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


void populateScreens() {
	availableScreens[MS_HOME_SCREEN].drawUI = &drawHomeScreen;
	availableScreens[MS_HOME_SCREEN].handleButtons = &handleHomeScreen;

	availableScreens[MS_EMPTY_SCREEN].drawUI = &drawEmptyScreen;
	availableScreens[MS_EMPTY_SCREEN].handleButtons = &handleEmptyScreen;

	availableScreens[MS_SETTINGS_SCREEN].drawUI = &drawSettingsScreen;
	availableScreens[MS_SETTINGS_SCREEN].handleButtons = &handleSettingsScreen;

	availableScreens[MS_SENSOR_POWER_SETTINGS_SCREEN].drawUI = &drawSensorSettingsScreen;
	availableScreens[MS_SENSOR_POWER_SETTINGS_SCREEN].handleButtons = &handleSensorSettingsScreen;

	availableScreens[MS_SENSOR_SETTINGS_MENU_SCREEN].drawUI = &drawSensorSettingsMenuScreen;
	availableScreens[MS_SENSOR_SETTINGS_MENU_SCREEN].handleButtons = &handleSensorSettingsMenuScreen;

	availableScreens[MS_SENSOR_CALIBRATION_SETTINGS_SCREEN].drawUI = &drawSensorSettingsCalibrationScreen;
	availableScreens[MS_SENSOR_CALIBRATION_SETTINGS_SCREEN].handleButtons = &handleSensorSettingsCalibrationScreen;

	availableScreens[MS_CALIBRATION_INFO_SCREEN].drawUI = &drawCalibrationInfoScreen;
	availableScreens[MS_CALIBRATION_INFO_SCREEN].handleButtons = &handleCalibrationInfoScreen;

	availableScreens[MS_THRESHOLDS_SETTINGS_SCREEN].drawUI = &drawThresholdsSettingsScreen;
	availableScreens[MS_THRESHOLDS_SETTINGS_SCREEN].handleButtons = &handleThresholdsSettingsScreen;

	availableScreens[MS_THRESHOLDS_SETTINGS_MENU_SCREEN].drawUI = &drawThresholdsSettingsMenuScreen;
	availableScreens[MS_THRESHOLDS_SETTINGS_MENU_SCREEN].handleButtons = &handleThresholdsSettingsMenuScreen;

	availableScreens[MS_INTERVALS_SETTINGS_SCREEN].drawUI = &drawIntervalsSettingsScreen;
	availableScreens[MS_INTERVALS_SETTINGS_SCREEN].handleButtons = &handleIntervalsSettingsScreen;

	availableScreens[MS_PUMP_SETTINGS_MENU_SCREEN].drawUI = &drawPumpSettingsScreen;
	availableScreens[MS_PUMP_SETTINGS_MENU_SCREEN].handleButtons = &handlePumpSettingsScreen;

	availableScreens[MS_PUMP_INTERVALS_SETTINGS_SCREEN].drawUI = &drawPumpIntervalsSettingsScreen;
	availableScreens[MS_PUMP_INTERVALS_SETTINGS_SCREEN].handleButtons = &handlePumpIntervalsSettingsScreen;

	availableScreens[MS_PUMP_CLEANING_INFO_SCREEN].drawUI = &drawPumpCleaningInfoScreen;
	availableScreens[MS_PUMP_CLEANING_INFO_SCREEN].handleButtons = &handlePumpCleaningInfoScreen;

	availableScreens[MS_SENSOR_INTERVALS_SETTINGS_SCREEN].drawUI = &drawSensorIntervalsSettingsScreen;
	availableScreens[MS_SENSOR_INTERVALS_SETTINGS_SCREEN].handleButtons = &handleSensorIntervalsSettingsScreen;

	availableScreens[MS_PROCESSES_SCREEN].drawUI = &drawProcessesScreen;
	availableScreens[MS_PROCESSES_SCREEN].handleButtons = &handleProcessesScreen;

	availableScreens[MS_MENU_SCREEN].drawUI = &drawMenuScreen;
	availableScreens[MS_MENU_SCREEN].handleButtons = &handleMenuScreen;

	availableScreens[MS_CONNECTIVITY_SETTINGS_SCREEN].drawUI = &drawConnectivitySettingsScreen;
	availableScreens[MS_CONNECTIVITY_SETTINGS_SCREEN].handleButtons = &handleConnectivitySettingsScreen;

	availableScreens[MS_CONNECTIVITY_INFO_SCREEN].drawUI = &drawConnectivityInfoScreen;
	availableScreens[MS_CONNECTIVITY_INFO_SCREEN].handleButtons = &handleConnectivityInfoScreen;

	availableScreens[MS_WIFI_TOGGLE_SCREEN].drawUI = &drawWIFIToggleScreen;
	availableScreens[MS_WIFI_TOGGLE_SCREEN].handleButtons = &handleWIFIToggleScreen;
}

void populateActions() {

	// calibrate sensor action
	availableActions[CALIBRATE_SENSOR_ACTION].canStart = &calibrateSensorCanStart;
	availableActions[CALIBRATE_SENSOR_ACTION].tick = &tickCalibrateSensor;
	availableActions[CALIBRATE_SENSOR_ACTION].frozen = false; // when stopped the action will be removed from the list
	availableActions[CALIBRATE_SENSOR_ACTION].stopRequested = false;
	availableActions[CALIBRATE_SENSOR_ACTION].start = &startCalibrateSensor;
	availableActions[CALIBRATE_SENSOR_ACTION].stop = &stopCalibrateSensor;
	availableActions[CALIBRATE_SENSOR_ACTION].ti = 1;
	availableActions[CALIBRATE_SENSOR_ACTION].td = 0; // duration of 0 means we never stop
	availableActions[CALIBRATE_SENSOR_ACTION].to = 0;
	availableActions[CALIBRATE_SENSOR_ACTION].clear = false;
	availableActions[CALIBRATE_SENSOR_ACTION].state = MS_NON_ACTIVE;
	availableActions[CALIBRATE_SENSOR_ACTION].child = nullptr;
	availableActions[CALIBRATE_SENSOR_ACTION].lst = 0;
	availableActions[CALIBRATE_SENSOR_ACTION].st = 0;
	availableActions[CALIBRATE_SENSOR_ACTION].name = "calibrate";

	// wifi action
	availableActions[WIFI_ACTION].tick = &tickWifi;
	availableActions[WIFI_ACTION].frozen = false; // when stopped the action will be removed from the list
	availableActions[WIFI_ACTION].stopRequested = false;
	availableActions[WIFI_ACTION].start = &startWifi;
	availableActions[WIFI_ACTION].stop = &stopWifi;
	availableActions[WIFI_ACTION].ti = 1;
	availableActions[WIFI_ACTION].td = 0; // duration of 0 means we never stop
	availableActions[WIFI_ACTION].to = 0;
	availableActions[WIFI_ACTION].clear = false;
	availableActions[WIFI_ACTION].state = MS_NON_ACTIVE;
	availableActions[WIFI_ACTION].child = nullptr;
	availableActions[WIFI_ACTION].lst = 0;
	availableActions[WIFI_ACTION].st = 0;
	availableActions[WIFI_ACTION].name = "wifi";

	// interpret sensor data action
	availableActions[INTERPRET_SENSOR_DATA_ACTION].tick = &tickInterpret;
	availableActions[INTERPRET_SENSOR_DATA_ACTION].frozen = false;
	availableActions[INTERPRET_SENSOR_DATA_ACTION].stopRequested = false;
	availableActions[INTERPRET_SENSOR_DATA_ACTION].start = &startInterpret;
	availableActions[INTERPRET_SENSOR_DATA_ACTION].stop = &stopInterpret;
	availableActions[INTERPRET_SENSOR_DATA_ACTION].ti = 1;
	availableActions[INTERPRET_SENSOR_DATA_ACTION].td = 1;
	availableActions[INTERPRET_SENSOR_DATA_ACTION].to = 0;
	availableActions[INTERPRET_SENSOR_DATA_ACTION].clear = false;
	availableActions[INTERPRET_SENSOR_DATA_ACTION].state = MS_NON_ACTIVE;
	availableActions[INTERPRET_SENSOR_DATA_ACTION].child = nullptr;
	availableActions[INTERPRET_SENSOR_DATA_ACTION].lst = 0;
	availableActions[INTERPRET_SENSOR_DATA_ACTION].st = 0;
	availableActions[INTERPRET_SENSOR_DATA_ACTION].name = "interpret";


	// read sensors action
	availableActions[READ_SENSORS_ACTION].canStart = &readSensorsCanStart;
	availableActions[READ_SENSORS_ACTION].tick = &tickSensors;
	availableActions[READ_SENSORS_ACTION].frozen = true;
	availableActions[READ_SENSORS_ACTION].stopRequested = false;
	availableActions[READ_SENSORS_ACTION].start = &startSensors;
	availableActions[READ_SENSORS_ACTION].stop = &stopSensors;
	availableActions[READ_SENSORS_ACTION].ti = settings.siw;
	availableActions[READ_SENSORS_ACTION].td = settings.sd;
	availableActions[READ_SENSORS_ACTION].to = 200;
	availableActions[READ_SENSORS_ACTION].clear = false;
	availableActions[READ_SENSORS_ACTION].state = MS_NON_ACTIVE;
	availableActions[READ_SENSORS_ACTION].child = nullptr;
	availableActions[READ_SENSORS_ACTION].lst = 0;
	availableActions[READ_SENSORS_ACTION].st = 0;
	availableActions[READ_SENSORS_ACTION].name = "sensors";

	// open far valve action
	availableActions[OUTLET_FAR_ACTION].canStart = &canStartOutlet;
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
	availableActions[OUTLET_FAR_ACTION].name = "valve-far";


	state.s[MS_SENSOR_FAR].ai = OUTLET_FAR_ACTION;

	// open mid valve action
	availableActions[OUTLET_MID_ACTION].canStart = &canStartOutlet;
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
	availableActions[OUTLET_MID_ACTION].name = "valve-mid";

	state.s[MS_SENSOR_MID].ai = OUTLET_MID_ACTION;

	// open near valve action
	availableActions[OUTLET_NEAR_ACTION].canStart = &canStartOutlet;
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
	availableActions[OUTLET_NEAR_ACTION].name = "valve-near";

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
	availableActions[PUMP_ACTION].name = "pump";

	// clean pump action
	availableActions[CLEAN_PUMP_ACTION].canStart = &cleanPumpCanStart;
	availableActions[CLEAN_PUMP_ACTION].tick = &tickCleanPump;
	availableActions[CLEAN_PUMP_ACTION].frozen = false;
	availableActions[CLEAN_PUMP_ACTION].stopRequested = false;
	availableActions[CLEAN_PUMP_ACTION].start = &startCleanPump;
	availableActions[CLEAN_PUMP_ACTION].stop = &stopCleanPump;
	availableActions[CLEAN_PUMP_ACTION].ti = 0;
	availableActions[CLEAN_PUMP_ACTION].td = 0;
	availableActions[CLEAN_PUMP_ACTION].clear = false;
	availableActions[CLEAN_PUMP_ACTION].to = 0;
	availableActions[CLEAN_PUMP_ACTION].state = MS_NON_ACTIVE;
	availableActions[CLEAN_PUMP_ACTION].child = nullptr;
	availableActions[CLEAN_PUMP_ACTION].lst = 0;
	availableActions[CLEAN_PUMP_ACTION].st = 0;
	availableActions[CLEAN_PUMP_ACTION].name = "clean-pump";


	// draw home action
	availableActions[DRAW_UI_ACTION].tick = &tickBuildScreen;
	availableActions[DRAW_UI_ACTION].frozen = true;
	availableActions[DRAW_UI_ACTION].stopRequested = false;
	availableActions[DRAW_UI_ACTION].start = &startBuildScreen;
	availableActions[DRAW_UI_ACTION].stop = &stopBuildScreen;
	availableActions[DRAW_UI_ACTION].ti = 1000;
	availableActions[DRAW_UI_ACTION].td = 100;
	availableActions[DRAW_UI_ACTION].clear = false;
	availableActions[DRAW_UI_ACTION].to = 0;
	availableActions[DRAW_UI_ACTION].state = MS_NON_ACTIVE;
	availableActions[DRAW_UI_ACTION].child = nullptr;
	availableActions[DRAW_UI_ACTION].lst = 0;
	availableActions[DRAW_UI_ACTION].st = 0;
	availableActions[DRAW_UI_ACTION].name = "ui";
}

int readButton() {
	return fixedAnalogRead(BUTTONS_PIN);
}

void storeSetPreferences() {
	preferences.begin(MS_PREFERENCES_ID, false);
	preferences.putBool(MS_IRRIGATE_UNTIL_EXPIRY_KEY, settings.iue);
	preferences.putBool(MS_FAR_ACTIVE_SETTING_KEY, state.s[MS_SENSOR_FAR].active);
	preferences.putBool(MS_MID_ACTIVE_SETTING_KEY, state.s[MS_SENSOR_MID].active);
	preferences.putBool(MS_NEAR_ACTIVE_SETTING_KEY, state.s[MS_SENSOR_NEAR].active);

	preferences.putInt(MS_APV_NEAR_SETTING_KEY, state.s[MS_SENSOR_NEAR].apv);
	preferences.putInt(MS_DAPV_NEAR_SETTING_KEY, state.s[MS_SENSOR_NEAR].dapv);

	preferences.putInt(MS_APV_MID_SETTING_KEY, state.s[MS_SENSOR_MID].apv);
	preferences.putInt(MS_DAPV_MID_SETTING_KEY, state.s[MS_SENSOR_MID].dapv);

	preferences.putInt(MS_APV_FAR_SETTING_KEY, state.s[MS_SENSOR_FAR].apv);
	preferences.putInt(MS_DAPV_FAR_SETTING_KEY, state.s[MS_SENSOR_FAR].dapv);

	preferences.putBool(MS_WIFI_TOGGLE_SETTING_KEY, wifi.isActive);

	preferences.putULong(MS_SENSOR_INTERVAL_PUMPING_SETTING_KEY, settings.siw);
	preferences.putULong(MS_SENSOR_INTERVAL_DRY_SETTING_KEY, settings.sid);
	preferences.putULong(MS_SENSOR_INTERVAL_ON_SETTING_KEY, availableActions[READ_SENSORS_ACTION].td);

	preferences.putULong(MS_PUMP_MAX_DURATION_SETTING_KEY, settings.pd);
	preferences.putULong(MS_PUMP_REACT_INT_DURATION_SETTING_KEY, settings.pi);

	preferences.end();
}

void readStoredPreferences() {
	preferences.begin(MS_PREFERENCES_ID, false);

	state.s[MS_SENSOR_NEAR].dry = preferences.getInt(MS_NEAR_DRY_SETTING_KEY);
	state.s[MS_SENSOR_MID].dry = preferences.getInt(MS_MID_DRY_SETTING_KEY);
	state.s[MS_SENSOR_FAR].dry = preferences.getInt(MS_FAR_DRY_SETTING_KEY);

	state.s[MS_SENSOR_NEAR].wet = preferences.getInt(MS_NEAR_WET_SETTING_KEY);
	state.s[MS_SENSOR_MID].wet = preferences.getInt(MS_MID_WET_SETTING_KEY);
	state.s[MS_SENSOR_FAR].wet = preferences.getInt(MS_FAR_WET_SETTING_KEY);

	state.s[MS_SENSOR_NEAR].active = preferences.getBool(MS_NEAR_ACTIVE_SETTING_KEY, true);
	state.s[MS_SENSOR_MID].active = preferences.getBool(MS_MID_ACTIVE_SETTING_KEY, true);
	state.s[MS_SENSOR_FAR].active = preferences.getBool(MS_FAR_ACTIVE_SETTING_KEY, true);


	state.s[MS_SENSOR_NEAR].apv = preferences.getInt(MS_APV_NEAR_SETTING_KEY, 50);
	state.s[MS_SENSOR_NEAR].dapv = preferences.getInt(MS_DAPV_NEAR_SETTING_KEY, 85);

	state.s[MS_SENSOR_MID].apv = preferences.getInt(MS_APV_MID_SETTING_KEY, 50);
	state.s[MS_SENSOR_MID].dapv = preferences.getInt(MS_DAPV_MID_SETTING_KEY, 85);

	state.s[MS_SENSOR_FAR].apv = preferences.getInt(MS_APV_FAR_SETTING_KEY, 50);
	state.s[MS_SENSOR_FAR].dapv = preferences.getInt(MS_DAPV_FAR_SETTING_KEY, 85);

	settings.pd = preferences.getULong(MS_PUMP_MAX_DURATION_SETTING_KEY, settings.pd);
	settings.pi = preferences.getULong(MS_PUMP_REACT_INT_DURATION_SETTING_KEY, settings.pi);

	settings.iue = preferences.getBool(MS_IRRIGATE_UNTIL_EXPIRY_KEY, settings.iue);

	settings.sid = preferences.getULong(MS_SENSOR_INTERVAL_DRY_SETTING_KEY, settings.sid);
	settings.siw = preferences.getULong(MS_SENSOR_INTERVAL_PUMPING_SETTING_KEY, settings.siw);
	settings.sd = preferences.getULong(MS_SENSOR_INTERVAL_ON_SETTING_KEY, settings.sd);
	wifi.isActive = preferences.getBool(MS_WIFI_TOGGLE_SETTING_KEY, wifi.isActive);

	availableActions[OUTLET_NEAR_ACTION].ti = settings.pi;
	availableActions[OUTLET_NEAR_ACTION].td = settings.pd;

	availableActions[OUTLET_MID_ACTION].ti = settings.pi;
	availableActions[OUTLET_MID_ACTION].td = settings.pd;

	availableActions[OUTLET_FAR_ACTION].ti = settings.pi;
	availableActions[OUTLET_FAR_ACTION].td = settings.pd;

	availableActions[READ_SENSORS_ACTION].ti = settings.siw;
	availableActions[READ_SENSORS_ACTION].td = settings.sd;

	preferences.end();
}

void scheduleDefaultActions() {
	scheduleAction(&executionList, &availableActions[READ_SENSORS_ACTION]);
	scheduleAction(&executionList, &availableActions[DRAW_UI_ACTION]);
	if (wifi.isActive) {
		scheduleAction(&executionList, &availableActions[WIFI_ACTION]);
	}
}

void setupInitialState() {
	if (availableActions != nullptr) {
		// init the Actions library
		initActionsList();

		// populate the available screens
		populateScreens();

		// populate the available actions
		populateActions();

		// set initial screen to draw
		state.scr = MS_HOME_SCREEN;

		// schedule sensors and UI actions
		scheduleDefaultActions();
	}
	else {
#ifdef DEBUG
		Serial.println(F("MEM: Actions"));
#endif
	}
}

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


	bool init = false;
	int seconds = 20;
	while (true) {
		drawStartingPromptScreen(&display, seconds);
		delay(1000);
		if (seconds-- == 0) {
			break;
		}
		int bs = readButton();

		if (bs > BUTTON_1_LOW && bs < BUTTON_1_HIGH) {
			init = true;
			break;
		}
		else if (bs > BUTTON_2_LOW && bs < BUTTON_2_HIGH) {
			init = false;
			break;
		}
	}

	sprintf(state.s[MS_SENSOR_NEAR].name, "near");
	sprintf(state.s[MS_SENSOR_MID].name, "mid");
	sprintf(state.s[MS_SENSOR_FAR].name, "far");
	// we start with stored values normally
	if (!init) {

#ifdef ARDUINO_ARCH_ESP32
		readStoredPreferences();

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
		showTextCaptionScreen(&display, MS_STARTING_PROMPT_TEXT);
		// drawStartingValuesScreen(&display);
		delay(2000);
	}
	else {
		digitalWrite(SENSOR_PIN, SENSOR_PIN_HIGH);
#ifdef ARDUINO_ARCH_ESP32
		preferences.begin(MS_PREFERENCES_ID, false);
		preferences.clear();
#else
		for (int i = 0; i < EEPROM.length(); i++) {
			EEPROM.put(i, 0);
		}
#endif
		display.clearDisplay();
		showActionPromptScreen(&display, "B2 for", "dry state");
		int br = readButton();
		while (!(br > BUTTON_2_LOW && br < BUTTON_2_HIGH)) {
			br = readButton();
		}
		display.clearDisplay();
		showTextCaptionScreen(&display, MS_READING_PROMPT_TEXT);

		// waiting for completely wet values
		extractMedianPinValueForProperty(1000, &state.s[MS_SENSOR_NEAR].dry, PIN_NEAR);
		extractMedianPinValueForProperty(1000, &state.s[MS_SENSOR_MID].dry, PIN_MID);
		extractMedianPinValueForProperty(1000, &state.s[MS_SENSOR_FAR].dry, PIN_FAR);
#ifdef ARDUINO_ARCH_ESP32
		preferences.putInt(MS_NEAR_DRY_SETTING_KEY, state.s[MS_SENSOR_NEAR].dry);
		preferences.putInt(MS_MID_DRY_SETTING_KEY, state.s[MS_SENSOR_MID].dry);
		preferences.putInt(MS_FAR_DRY_SETTING_KEY, state.s[MS_SENSOR_FAR].dry);
#else
		EEPROM.put(0, state.s[MS_SENSOR_NEAR].dry);
		EEPROM.put(sizeof(int), state.s[MS_SENSOR_MID].dry);
		EEPROM.put(sizeof(int) * 2, state.s[MS_SENSOR_FAR].dry);
#endif


		drawDryValuesScreen(&display);
		delay(5000);
		display.clearDisplay();
		showActionPromptScreen(&display, "B2 for", "wet state");
		br = readButton();
		while (!(br > BUTTON_2_LOW && br < BUTTON_2_HIGH)) {
			br = readButton();
		}
		display.clearDisplay();
		showTextCaptionScreen(&display, MS_READING_PROMPT_TEXT);

		// waiting for completely wet values
		extractMedianPinValueForProperty(1000, &state.s[MS_SENSOR_NEAR].wet, PIN_NEAR);
		extractMedianPinValueForProperty(1000, &state.s[MS_SENSOR_MID].wet, PIN_MID);
		extractMedianPinValueForProperty(1000, &state.s[MS_SENSOR_FAR].wet, PIN_FAR);
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
		showTextCaptionScreen(&display, MS_STARTING_PROMPT_TEXT);
		delay(2000);
		digitalWrite(SENSOR_PIN, SENSOR_PIN_LOW);
#ifdef ARDUINO_ARCH_ESP32
		preferences.end();
#endif
		}
	}

// Fix for WIFI + analogRead from ADC2 issue

uint64_t reg_a;
uint64_t reg_b;
uint64_t reg_c;

void storeADC2ConfigRegisters() {
	//reg_a = READ_PERI_REG(SENS_SAR_START_FORCE_REG);
	reg_b = READ_PERI_REG(SENS_SAR_READ_CTRL2_REG);
	//reg_c = READ_PERI_REG(SENS_SAR_MEAS_START2_REG);
}

void restoreADC2ConfigRegisters() {
	//WRITE_PERI_REG(SENS_SAR_START_FORCE_REG, reg_a);
	WRITE_PERI_REG(SENS_SAR_READ_CTRL2_REG, reg_b);
	//WRITE_PERI_REG(SENS_SAR_MEAS_START2_REG, reg_c);
}

// Read the following threads for more information:
// https://github.com/espressif/arduino-esp32/issues/102
// https://github.com/espressif/arduino-esp32/issues/440
int fixedAnalogRead(int pin) {
	restoreADC2ConfigRegisters();
	SET_PERI_REG_MASK(SENS_SAR_READ_CTRL2_REG, SENS_SAR2_DATA_INV);
	return analogRead(pin);
}

//

void setup() {
	storeADC2ConfigRegisters();
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
	initActionsList(ACTIONS_COUNT);
	ms_init();

	setupInitialState();
}

void loop() {
	doQueueActions(&executionList);
}
