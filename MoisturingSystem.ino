#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <EEPROM.h>



// screen

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define DISPLAY_INIT_ADDRESS 0x3C

// end of screen

#define ACTIONS_COUNT 8
#define SENSORS_COUNT 3

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

// action defines

#define SENSORS_ACTION 0
#define SETTINGS_ACTION 1
#define BUZZER_ACTION 2
#define OUTLET_FAR_ACTION 3
#define OUTLET_MID_ACTION 4
#define OUTLET_NEAR_ACTION 5
#define PUMP_ACTION 6
#define DISPLAY_SPLASH_ACTION 7

// end of action defines

//#undef DEBUG
#define DEBUG

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

enum {
	far = 0,
	mid = 1,
	near = 2
};

enum ActionState {
	NON_ACTIVE = 0,
	SCHEDULED = 1,
	PENDING = 2,
	RUNNING = 3,
	CHILD_RUNNING = 4,
	CHILD_PENDING = 5
};

// Structures

struct MSysSettings {
	unsigned long siw = 5000; // 5 seconds for sensor interval while watering
	unsigned long sid = 60000 * 1; // 1 minute for sensor interval while in stand by
	unsigned long sd = 1000; // 1 second max duration for sensor activation;
	unsigned long pi = 60000 * 10; // 10 minutes between pump activations
	unsigned long pd = 60000 * 2; // 2 minutes max pump on duration
	long apv = 50; // % humidity threshold for pump activation
	long dapv = 80; // % humidity threshold for pump deactivation
} settings;

struct Sensor {
	int wet; // completely wet reading
	int dry; // completely dry reading
	int value; // current reading
	int ai; // action index
	float p; // humidity percentage
};

struct SystemState {
	Sensor* s = nullptr;
	bool p = false;
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
	ActionState state = NON_ACTIVE;
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

int printAndReturnValue(int value) {
#ifdef DEBUG
	Serial.print(F("Value: "));
	Serial.print(value);
	Serial.println();
#endif
	return value;
}

void printDryWetValues(SystemState* state) {

	Serial.print(F("------\n"));
	Serial.print(F("Dry values: \n"));

	printAndReturnValue((*state).s[far].dry);
	printAndReturnValue((*state).s[mid].dry);
	printAndReturnValue((*state).s[near].dry);

	Serial.print(F("Wet values: \n"));

	printAndReturnValue((*state).s[far].wet);
	printAndReturnValue((*state).s[mid].wet);
	printAndReturnValue((*state).s[near].wet);

	Serial.print(F("------\n"));

	Serial.flush();
}

// Helpers

void populateSettings(SystemState* state, Action* actions) {


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
	actions[SENSORS_ACTION].state = NON_ACTIVE;
	actions[SENSORS_ACTION].child = nullptr;
	actions[SENSORS_ACTION].lst = 0;
	actions[SENSORS_ACTION].st = 0;

	// settings action
	actions[SETTINGS_ACTION].tick = &tickModifySettings;
	actions[SETTINGS_ACTION].frozen = true;
	actions[SETTINGS_ACTION].stopRequested = false;
	actions[SETTINGS_ACTION].start = &startModifySettings;
	actions[SETTINGS_ACTION].stop = &stopModifySettings;
	actions[SETTINGS_ACTION].ti = 100;
	actions[SETTINGS_ACTION].td = 300;
	actions[SETTINGS_ACTION].clear = false;
	actions[SETTINGS_ACTION].to = 0;
	actions[SETTINGS_ACTION].state = NON_ACTIVE;
	actions[SETTINGS_ACTION].child = nullptr;
	actions[SETTINGS_ACTION].lst = 0;
	actions[SETTINGS_ACTION].st = 0;

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
	actions[BUZZER_ACTION].state = NON_ACTIVE;
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
	actions[OUTLET_FAR_ACTION].state = NON_ACTIVE;
	actions[OUTLET_FAR_ACTION].child = &actions[6];
	actions[OUTLET_FAR_ACTION].lst = 0;
	actions[OUTLET_FAR_ACTION].st = 0;


	(*state).s[far].ai = OUTLET_FAR_ACTION;

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
	actions[OUTLET_MID_ACTION].state = NON_ACTIVE;
	actions[OUTLET_MID_ACTION].child = &actions[6];
	actions[OUTLET_MID_ACTION].lst = 0;
	actions[OUTLET_MID_ACTION].st = 0;

	(*state).s[mid].ai = OUTLET_MID_ACTION;

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
	actions[OUTLET_NEAR_ACTION].state = NON_ACTIVE;
	actions[OUTLET_NEAR_ACTION].child = &actions[6];
	actions[OUTLET_NEAR_ACTION].lst = 0;
	actions[OUTLET_NEAR_ACTION].st = 0;

	(*state).s[near].ai = OUTLET_NEAR_ACTION;

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
	actions[PUMP_ACTION].state = NON_ACTIVE;
	actions[PUMP_ACTION].child = nullptr;
	actions[PUMP_ACTION].lst = 0;
	actions[PUMP_ACTION].st = 0;

	// display SPLASH action
	actions[DISPLAY_SPLASH_ACTION].tick = &tickDisplaySplash;
	actions[DISPLAY_SPLASH_ACTION].frozen = false;
	actions[DISPLAY_SPLASH_ACTION].stopRequested = false;
	actions[DISPLAY_SPLASH_ACTION].start = &startDisplaySplash;
	actions[DISPLAY_SPLASH_ACTION].stop = &stopDisplaySplash;
	actions[DISPLAY_SPLASH_ACTION].ti = 1000;
	actions[DISPLAY_SPLASH_ACTION].td = 5000;
	actions[DISPLAY_SPLASH_ACTION].clear = false;
	actions[DISPLAY_SPLASH_ACTION].to = 0;
	actions[DISPLAY_SPLASH_ACTION].state = NON_ACTIVE;
	actions[DISPLAY_SPLASH_ACTION].child = nullptr;
	actions[DISPLAY_SPLASH_ACTION].lst = 0;
	actions[DISPLAY_SPLASH_ACTION].st = 0;
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

void soundBuzzer(int millis) {
	analogWrite(MS_BUZZER_PIN, MS_BUZZER_VOLUME);
	delay(millis);
	analogWrite(MS_BUZZER_PIN, 0);
}

bool descheduleAction(ActionsList* list, Action* a) {
	Action* found = find(list, a);
	if (found == nullptr) {
		return false;
	}

	if (found != nullptr && (*found).state != RUNNING) {
		(*found).state = NON_ACTIVE;

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

	if ((*a).state != NON_ACTIVE) {
		return false;
	}

	if (a != nullptr) {
		(*a).state = SCHEDULED;
		addListItem(list, a);
		return true;
	}

	return false;
}

void initActions(SystemState* state) {
	if (availableActions != nullptr) {
		populateSettings(state, availableActions);
		scheduleAction(&executionList, &availableActions[SENSORS_ACTION]);
		scheduleAction(&executionList, &availableActions[SETTINGS_ACTION]);
		scheduleAction(&executionList, &availableActions[DISPLAY_SPLASH_ACTION]);
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

	farV = (int)(farV / 10);
	midV = (int)(midV / 10);
	nearV = (int)(nearV / 10);

	(*far) = printAndReturnValue(farV);
	(*mid) = printAndReturnValue(midV);
	(*near) = printAndReturnValue(nearV);
}

// End of Helpers

// Actions

// Display

void startDisplaySplash(SystemState* state, Action* a) {
	oled.clearDisplay();
	oled.drawRoundRect(0, 0, 128, 64, 10, WHITE);
	oled.display();
}

void tickDisplaySplash(SystemState* state, Action* a) {

}

void stopDisplaySplash(SystemState* state, Action* a) {
	oled.clearDisplay();
}

void startDisplayHome(SystemState* state, Action* a) {
}

void tickDisplayHome(SystemState* state, Action* a) {

}

void stopDisplayHome(SystemState* state, Action* a) {

}

// end of Display

// Outlets

// Far

void startFarOutlet(SystemState* state, Action* a) {
	digitalWrite(PIN_VALVE_FAR, LOW);
}

void tickFarOutlet(SystemState* state, Action* a) {
}

void stopFarOutlet(SystemState* state, Action* a) {
	digitalWrite(PIN_VALVE_FAR, HIGH);
}

// end of Far

// Mid

void startMidOutlet(SystemState* state, Action* a) {
	digitalWrite(PIN_VALVE_MID, LOW);
}

void tickMidOutlet(SystemState* state, Action* a) {
}

void stopMidOutlet(SystemState* state, Action* a) {
	digitalWrite(PIN_VALVE_MID, HIGH);
}

// end of Mid

// Near

void startNearOutlet(SystemState* state, Action* a) {
	digitalWrite(PIN_VALVE_NEAR, LOW);
}

void tickNearOutlet(SystemState* state, Action* a) {
}

void stopNearOutlet(SystemState* state, Action* a) {
	digitalWrite(PIN_VALVE_NEAR, HIGH);
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

	extractMedianPinValueForProperty(0, &(*state).s[near].value, &(*state).s[mid].value, &(*state).s[far].value);

	bool activate = false;
	Action* toSchedule = nullptr;
	Action** acandidates = (Action**)malloc(sizeof(Action*) * SENSORS_COUNT);
	int pc = 0;
	int ac = 0;
	for (int i = 0; i < SENSORS_COUNT; i++) {
		Sensor* se = &(*state).s[i];
		float v = (float)(*se).value;
		float d = (float)(*se).dry;
		float w = (float)(*se).wet;
		float dwd = max(0.00001, d - w);

		float mp = ((d - min(max(v, w), d)) / dwd) * 100.0;
		(*se).p = mp;
#ifdef DEBUG
		Serial.print(F("%: "));
		Serial.print(mp);
		Serial.println();
#endif
		activate = (bool)((long)mp < ((*state).p ? settings.dapv : settings.apv));
		Action* ca = &availableActions[(*se).ai];

		acandidates[i] = nullptr;
		if (activate && ca != nullptr) {
			ac++;
			if ((*ca).state == NON_ACTIVE) {
				acandidates[i] = ca;
				pc++;
			}
			else if ((*ca).state == PENDING) {
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
	if (pc == ac) {

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

	(*a).ti = availableActions[PUMP_ACTION].state == CHILD_RUNNING ? settings.siw : settings.sid;
	free(acandidates);
}

//end of sensors

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
	analogWrite(MS_BUZZER_PIN, MS_BUZZER_VOLUME);
}

void tickBuzzer(SystemState* state, Action* a) {
}

void stopBuzzer(SystemState* state, Action* a) {
	analogWrite(MS_BUZZER_PIN, LOW);
}

// end of sound buzzer

// modify settings

void startModifySettings(SystemState* state, Action* a) {
	int newValue = analogRead(BUTTONS_PIN);

	if (newValue != button.value) {
		button.hasChanged = true;
	}

	button.value = newValue;
}

void stopModifySettings(SystemState* state, Action* a) {
	button.hasChanged = false;
	button.value = 0;
}

void tickModifySettings(SystemState* state, Action* a) {
	if (button.hasChanged) {
		int br = button.value;
		if (br > 100 && br < 280) {
			int nv = min(settings.apv + 5, settings.dapv - 5);
			int ov = settings.apv;
			settings.apv = nv;

			if (nv != ov) {
				Serial.print(settings.apv);
				Serial.println();
				scheduleBuzzerAction(100);
			}
		}
		else if (br > 280 && br < 450) {
			int nv = max(settings.apv - 5, 10);
			int ov = settings.apv;
			settings.apv = nv;

			if (nv != ov) {
				Serial.print(settings.apv);
				Serial.println();
				scheduleBuzzerAction(200);
			}
		}
		else if (br > 450 && br < 600) {
			int nv = min(settings.dapv + 5, 100);
			int ov = settings.dapv;
			settings.dapv = nv;

			if (nv != ov) {
				Serial.print(settings.dapv);
				Serial.println();
				scheduleBuzzerAction(300);
			}
		}
		else if (br > 600 && br < 1024) {
			int nv = max(settings.dapv - 5, settings.apv + 5);
			int ov = settings.dapv;
			settings.dapv = nv;

			if (nv != ov) {
				Serial.print(settings.dapv);
				Serial.println();
				scheduleBuzzerAction(400);
			}
		}
		button.hasChanged = false;
	}
}

// end of modify settings

// end of Actions

void allocateMemPools(SystemState* state) {
	availableActions = (Action*)calloc(ACTIONS_COUNT, sizeof(Action));
	(*state).s = (Sensor*)calloc(SENSORS_COUNT, sizeof(Sensor));
}

void initScreen(SystemState* state) {
	if (!oled.begin(SSD1306_SWITCHCAPVCC, DISPLAY_INIT_ADDRESS, -1)) {
		Serial.println(F("FAIL: Screen"));
		while (true);
	}

	oled.display();
}

void init(SystemState* state) {
	// init must be made with completely dry state values

	soundBuzzer(1000);

	delay(5000);
	int bs = analogRead(BUTTONS_PIN);

	// we start with stored values normally
	if (bs < 200) {
		makeBeeps(3);

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

		printDryWetValues(state);
#ifdef DEBUG
		digitalWrite(SENSOR_PIN, SENSOR_PIN_HIGH);
#endif
		//while (bs < 200 || bs > 280) {
		//	bs = analogRead(BUTTONS_PIN);
		//	//#ifdef DEBUG
		//	//			extractMedianPinValueForProperty(1000, &(*state).s.d[near].dry, &(*state).s.d[mid].dry, &(*state).s.d[far].dry);
		//	//			delay(1000);
		//	//#endif
		//}

#ifdef DEBUG
		digitalWrite(SENSOR_PIN, SENSOR_PIN_LOW);
#endif
		makeBeeps(1);
	}
	else if (bs > 200 && bs < 280) {
		digitalWrite(SENSOR_PIN, SENSOR_PIN_HIGH);
		makeBeeps(2);

		for (int i = 0; i < EEPROM.length(); i++) {
			EEPROM.put(i, 0);
		}
		int br = analogRead(BUTTONS_PIN);
		while (br < 300 || br > 450) {
			br = analogRead(BUTTONS_PIN);
		}

		soundBuzzer(300);

		// waiting for completely wet values
		extractMedianPinValueForProperty(1000, &(*state).s[near].dry, &(*state).s[mid].dry, &(*state).s[far].dry);

		EEPROM.put(0, (*state).s[near].dry);
		EEPROM.put(sizeof(int), (*state).s[mid].dry);
		EEPROM.put(sizeof(int) * 2, (*state).s[far].dry);

		// fire buzzer to notify that dry values have been read
		soundBuzzer(1000);

		br = analogRead(BUTTONS_PIN);
		while (br < 300 || br > 450) {
			br = analogRead(BUTTONS_PIN);
		}

		soundBuzzer(300);

		// waiting for completely wet values
		extractMedianPinValueForProperty(1000, &(*state).s[near].wet, &(*state).s[mid].wet, &(*state).s[far].wet);


		EEPROM.put(sizeof(int) * 3, (*state).s[near].wet);
		EEPROM.put(sizeof(int) * 4, (*state).s[mid].wet);
		EEPROM.put(sizeof(int) * 5, (*state).s[far].wet);

		// wet values have been read
		soundBuzzer(1000);

		// click to continue
		br = analogRead(BUTTONS_PIN);
		while (br < 300 || br > 450) {
			br = analogRead(BUTTONS_PIN);
		}

		// initiating standard routine
		soundBuzzer(2000);

		digitalWrite(SENSOR_PIN, SENSOR_PIN_LOW);
	}
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

	if ((*a).state == RUNNING) {
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

	if ((*a).state == RUNNING) {
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
			(*action).state = RUNNING;
			(*action).st = time;

			Action* child = (*action).child;
			while (child != nullptr) {
#ifdef DEBUG
				Serial.print(F("Starting child: "));
				Serial.println(indexOfAction(child));
#endif
				(*child).start(state, child);
				(*child).state = CHILD_RUNNING;
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
					(*child).state = NON_ACTIVE;
					child = (*child).child;
				}

				(*action).stop(state, action);
				(*action).lst = time;
				(*action).state = PENDING;
				(*action).clear = true;
			}
			else {
				if (time - (*action).st > (*action).to) {
					(*action).tick(state, action);
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

			if (!(*a).frozen) {
				descheduleAction(&executionList, a);
			}
			(*a).clear = false;
		}
	}
}

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char* __brkval;
#endif  // __arm__

int freeMemory() {
	char top;
#ifdef __arm__
	return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
	return &top - __brkval;
#else  // __arm__
	return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
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
	pinMode(MS_BUZZER_PIN, OUTPUT); // buzzer

	pinMode(PUMP_PIN, OUTPUT); // pump relay
	digitalWrite(PUMP_PIN, PUMP_PIN_LOW);

	pinMode(SENSOR_PIN, OUTPUT); // sensor relay
	digitalWrite(SENSOR_PIN, SENSOR_PIN_LOW);
	Serial.begin(9600);

	initScreen(&state);
	Serial.println(freeMemory());
	allocateMemPools(&state);

	Serial.println(freeMemory());

	init(&state);
	initActions(&state);
}

void loop() {
	doQueueActions(&state, &executionList);
}