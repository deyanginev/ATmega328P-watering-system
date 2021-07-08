#include <EEPROM.h>
#define ACTIONS_COUNT 3

#define PUMP_PIN 4
#define SENSOR_PIN 2

#define MS_BUZZER_PIN 3
#define MS_BUZZER_VOLUME 15

#define PIN_FAR A0
#define PIN_MID A1
#define PIN_NEAR A2

#define BUTTONS_PIN A3

#define BUTTON_PIN 8

enum {
	far = 0,
	mid = 1,
	near = 2
};

// Structures

struct Sensor {
	int wet;
	int dry;
	int value;
};

struct Sensors {
	size_t s;
	Sensor* d;
};

struct MSysSettings {
	unsigned long siw = 5000; // 5 seconds for sensor interval while watering
	unsigned long sid = 60000; // 1 minute for sensor interval while in stand by
	unsigned long sd = 1000; // 1 second max duration for sensor activation;
	unsigned long pi = 60000 * 10; // 10 minutes between pump activations
	unsigned long pd = 120000; // 2 minutes max pump on duration
	long apv = 10; // 10% humidity threshold for pump activation
	long dapv = 60; // 60% humidity threshold for pump deactivation
} settings;

struct SystemState {
	Sensors s = { 3, (Sensor*)malloc(sizeof(Sensor) * 3) };
	char line1[41]; // A display line has 40 characters
	char line2[41]; // A display line has 40 characters
	bool p = false;
} state;

struct Action {
	int code;
	bool frozen = false;
	unsigned long ti; // interval between executions
	unsigned long td = 1000; // maximum duration interval
	unsigned long ct = 0; // last stop time
	unsigned long to = 200; // first tick offset from start
	unsigned long st = 0; // last start time
	bool isActive = false;
	bool stopRequested = false;
	void (*tick)(SystemState*, Action*);
	void (*start)(SystemState*, Action*);
	void (*stop)(SystemState*, Action*);
};


struct ListItem {
	Action* value = nullptr;
	ListItem* prev = nullptr;
	ListItem* next = nullptr;
};

struct ButtonState {
	int value = 0;
	bool hasChanged = false;
} button;

struct ActionsList {
	size_t count = 0;
	ListItem* first = nullptr;
	ListItem* last = nullptr;
} actionsList;


struct ActionsArray {
	size_t s;
	Action* a = (Action*)malloc(sizeof(Action) * ACTIONS_COUNT);
} actions;

// end of structures

// Queue

ListItem* find(ActionsList* list, Action* item) {
	ListItem* cur = (*list).first;

	while (cur != nullptr) {
		if ((*cur).value == item) {
			return cur;
		}
		cur = (*cur).next;
	}
	return nullptr;
}

ListItem* addListItem(ActionsList* list, Action* a) {

	if (a != nullptr) {

		ListItem* item = (ListItem*)malloc(sizeof(ListItem));
		if (item != nullptr) {

			(*item).next = nullptr;
			(*item).prev = nullptr;

			(*item).value = a;
			(*item).prev = (*list).last;

			if ((*list).last != nullptr) {
				(*(*list).last).next = item;

			}

			(*list).last = item;

			if ((*list).first == nullptr) {
				(*list).first = item;
			}

			(*list).count++;

			return item;
		}
		else {
			Serial.print("ListItem");
		}
	}

	return nullptr;
}

int listCount(ActionsList* list) {
	ListItem* cur = (*list).first;
	int res = 0;
	while (cur != nullptr) {
		cur = (*cur).next;
		res++;
	}

	return res;
}

bool removeListItem(ActionsList* list, Action* item) {

	ListItem* tr = find(list, item);

	if (tr == nullptr) {
		return false;
	}

	if ((*list).last == tr) {
		(*list).last = (*tr).prev;
	}

	if ((*list).first == tr) {
		(*list).first = (*tr).next;
	}

	ListItem* pr = (*tr).prev;
	ListItem* nx = (*tr).next;

	if (pr != nullptr) {
		(*pr).next = nx;
	}

	if (nx != nullptr) {
		(*nx).prev = pr;
	}

	(*tr).prev = nullptr;
	(*tr).next = nullptr;

	free(tr);

	(*list).count--;

	return true;
}

// end of Queue

int printAndReturnValue(int value) {

	Serial.print("Value: ");
	Serial.print(value);
	Serial.println();

	return value;
}

void printDryWetValues(SystemState* state) {

	Serial.print("------\n");
	Serial.print("Dry values: \n");

	printAndReturnValue((*state).s.d[far].dry);
	printAndReturnValue((*state).s.d[mid].dry);
	printAndReturnValue((*state).s.d[near].dry);

	Serial.print("Wet values: \n");

	printAndReturnValue((*state).s.d[far].wet);
	printAndReturnValue((*state).s.d[mid].wet);
	printAndReturnValue((*state).s.d[near].wet);

	Serial.print("------\n");

	Serial.flush();

}

// Helpers

void populateSettings(ActionsArray* actions) {
	// read sensors action
	(*actions).a[0].tick = &tickSensors;
	(*actions).a[0].frozen = true;
	(*actions).a[0].start = &startSensors;
	(*actions).a[0].stop = &stopSensors;
	(*actions).a[0].ti = settings.siw;
	(*actions).a[0].td = settings.sd;
	(*actions).a[0].to = 200;
	(*actions).a[0].code = 1;
	(*actions).a[0].isActive = false;
	(*actions).a[0].stopRequested = false;

	// pump action
	(*actions).a[1].tick = &tickPump;
	(*actions).a[1].frozen = false;
	(*actions).a[1].start = &startPump;
	(*actions).a[1].stop = &stopPump;
	(*actions).a[1].ti = settings.pi;
	(*actions).a[1].td = settings.pd;
	(*actions).a[1].to = 0;
	(*actions).a[1].code = 2;
	(*actions).a[1].isActive = false;
	(*actions).a[1].stopRequested = false;


	// settings action
	(*actions).a[2].tick = &tickModifySettings;
	(*actions).a[2].frozen = true;
	(*actions).a[2].start = &startModifySettings;
	(*actions).a[2].stop = &stopModifySettings;
	(*actions).a[2].ti = 100;
	(*actions).a[2].td = 300;
	(*actions).a[2].code = 3;
	(*actions).a[2].to = 0;
	(*actions).a[2].isActive = false;
	(*actions).a[2].stopRequested = false;
}

unsigned long extractValue(String* instruction, String command, int defaultValue) {
	int cmdI = (*instruction).indexOf(command);
	if (cmdI != -1) {
		int cei = (*instruction).indexOf(';', cmdI);
		if (cei != -1) {
			int si = cmdI + command.length();
			String sv = (*instruction).substring(si, cei);
			unsigned long iv = sv.toInt();
			return iv != 0 ? iv : defaultValue;
		}
	}

	return defaultValue;
}

void makeBeeps(int count) {
	for (int i = 0; i < count; i++) {
		soundBuzzer(300);
		delay(300);
	}
}

void soundBuzzer(int millis) {
	analogWrite(MS_BUZZER_PIN, MS_BUZZER_VOLUME);
	delay(millis);
	analogWrite(MS_BUZZER_PIN, 0);
}

bool scheduleAction(ActionsList* list, Action* a) {
	ListItem* found = find(list, a);
	if (found != nullptr) {
		return false;
	}
	if (a != nullptr) {
		addListItem(list, a);
		return true;
	}
}

void initActions() {

	if (actions.a != nullptr) {
		populateSettings(&actions);
		scheduleAction(&actionsList, &actions.a[0]);
		scheduleAction(&actionsList, &actions.a[2]);
	}
	else {

		Serial.print("COULD NOT ALLOCATE MEMORY FOR ACTIONS");
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

// Sensors

void startSensors(SystemState* state, Action* a) {
	digitalWrite(SENSOR_PIN, HIGH);
}

void stopSensors(SystemState* state, Action* a) {
	digitalWrite(SENSOR_PIN, LOW);
}

void tickSensors(SystemState* state, Action* a) {

	extractMedianPinValueForProperty(0, &(*state).s.d[near].value, &(*state).s.d[mid].value, &(*state).s.d[far].value);

	bool activate = false;
	for (int i = 0; i < (*state).s.s; i++) {
		float v = (float)(*state).s.d[i].value;
		float d = (float)(*state).s.d[i].dry;
		float w = (float)(*state).s.d[i].wet;
		float dwd = max(0.00001, d - w);

		float mp = ((d - min(max(v, w), d)) / dwd) * 100.0;
		Serial.print("Current percentage: ");
		Serial.print(mp);
		Serial.println();
		activate = (bool)((long)mp < ((*state).p ? settings.dapv : settings.apv));
		if (activate) {
			break;
		}
	}

	(*state).p = activate;

	if ((*state).p) {
		bool s = scheduleAction(&actionsList, &actions.a[1]);
		if (s) {
			(*a).ti = settings.siw;
		}
	}
	else {
		(*a).ti = settings.sid;
		requestStop(&actions.a[1]);
	}
}

//end of sensors

// Pump
void startPump(SystemState* state, Action* a) {
	digitalWrite(PUMP_PIN, HIGH);
}

void stopPump(SystemState* state, Action* a) {
	digitalWrite(PUMP_PIN, LOW);
}

void tickPump(SystemState* state, Action* a) {
	// digitalWrite(PUMP_PIN, activate ? HIGH : LOW);
}

//end of pump

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
				soundBuzzer(100);
			}
		}
		else if (br > 280 && br < 450) {
			int nv = max(settings.apv - 5, 10);
			int ov = settings.apv;
			settings.apv = nv;

			if (nv != ov) {
				Serial.print(settings.apv);
				Serial.println();
				soundBuzzer(200);
			}
		}
		else if (br > 450 && br < 600) {
			int nv = min(settings.dapv + 5, 100);
			int ov = settings.dapv;
			settings.dapv = nv;

			if (nv != ov) {
				Serial.print(settings.dapv);
				Serial.println();
				soundBuzzer(300);
			}
		}
		else if (br > 600 && br < 1024) {
			int nv = max(settings.dapv - 5, settings.apv + 5);
			int ov = settings.dapv;
			settings.dapv = nv;

			if (nv != ov) {
				Serial.print(settings.dapv);
				Serial.println();
				soundBuzzer(400);
			}
		}
		button.hasChanged = false;
	}
}

// end of modify settings

// end of Actions

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

		(*state).s.d[near].dry = nd;
		(*state).s.d[mid].dry = md;
		(*state).s.d[far].dry = fd;

		(*state).s.d[near].wet = nw;
		(*state).s.d[mid].wet = mw;
		(*state).s.d[far].wet = fw;

		printDryWetValues(state);

		while (bs < 200 || bs > 280) {
			bs = analogRead(BUTTONS_PIN);
		}
		makeBeeps(1);
	}
	else if (bs > 200 && bs < 280) {
		digitalWrite(SENSOR_PIN, HIGH);
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
		extractMedianPinValueForProperty(1000, &(*state).s.d[near].dry, &(*state).s.d[mid].dry, &(*state).s.d[far].dry);

		EEPROM.put(0, (*state).s.d[near].dry);
		EEPROM.put(sizeof(int), (*state).s.d[mid].dry);
		EEPROM.put(sizeof(int) * 2, (*state).s.d[far].dry);

		// fire buzzer to notify that dry values have been read
		soundBuzzer(1000);

		br = analogRead(BUTTONS_PIN);
		while (br < 300 || br > 450) {
			br = analogRead(BUTTONS_PIN);
		}

		soundBuzzer(300);

		// waiting for completely wet values
		extractMedianPinValueForProperty(1000, &(*state).s.d[near].wet, &(*state).s.d[mid].wet, &(*state).s.d[far].wet);


		EEPROM.put(sizeof(int) * 3, (*state).s.d[near].wet);
		EEPROM.put(sizeof(int) * 4, (*state).s.d[mid].wet);
		EEPROM.put(sizeof(int) * 5, (*state).s.d[far].wet);

		// wet values have been read
		soundBuzzer(1000);

		// click to continue
		br = analogRead(BUTTONS_PIN);
		while (br < 300 || br > 450) {
			br = analogRead(BUTTONS_PIN);
		}

		// initiating standard routine
		soundBuzzer(2000);

		digitalWrite(SENSOR_PIN, LOW);
	}
}

bool requestStop(Action* a) {
	if ((*a).isActive && !(*a).stopRequested) {
		(*a).stopRequested = true;
		return true;
	}

	return false;
}

bool canStart(Action* a, unsigned long time) {

	if ((*a).isActive) {
		return false;
	}

	if (time - (*a).ct < (*a).ti) {
		return false;
	}

	return true;
}

bool shouldStop(Action* a, unsigned long time) {

	if ((*a).stopRequested) {
		Serial.print("Stop requested");
		return true;
	}

	if (time - (*a).st >= (*a).td) {
		return true;
	}

	return false;
}

void doActions(SystemState* state) {
	unsigned long cm = millis();
	for (int i = 0; i < actions.s; i++) {
		Action* a = &actions.a[i];
		if (cm - (*a).ct >= (*a).ti) {
			(*a).tick(state, a);
			(*a).ct = cm;
		}
	}
}

void doQueueActions(SystemState* state, ActionsList* list) {
	ListItem* current = (*list).first;
	unsigned long time = millis();
	int count = (*list).count;

	ListItem** scheduled = (ListItem**)malloc(sizeof(ListItem*) * count);

	if (scheduled != nullptr) {
		int i = 0;

		while (current != nullptr) {
			scheduled[i++] = current;
			current = (*current).next;
		}

		for (int k = 0; k < count; k++) {
			ListItem* item = scheduled[k];
			Action* action = (*item).value;
			if (canStart(action, time)) {
#ifdef DEBUG
				Serial.print("Starting: ");
				Serial.print((*action).code);
				Serial.println();
#endif

				(*action).start(state, action);
				(*action).isActive = true;
				(*action).st = time;
				if (time - (*action).st >= (*action).to) {
					(*action).tick(state, action);
				}
		}
			else {
				if ((*action).isActive) {
					if (shouldStop(action, time)) {

#ifdef DEBUG
						Serial.print("Stopping: ");
						Serial.print((*action).code);
						Serial.println();
#endif

						(*action).stop(state, action);
						(*action).isActive = false;
						(*action).stopRequested = false;
						(*action).ct = time;
						if (!(*action).frozen) {
							removeListItem(&actionsList, action);
						}
			}
					else {
						if (time - (*action).st > (*action).to) {
							(*action).tick(state, action);
						}
					}
	}
}
		}
	}
	else {

		Serial.print("COULD NOT ALLOCATE MEMORY FOR SCHEDULED ACTIONS!!!");

	}

	free(scheduled);
}

void setup() {
	pinMode(PIN_FAR, INPUT);
	pinMode(PIN_MID, INPUT);
	pinMode(PIN_NEAR, INPUT);
	pinMode(BUTTONS_PIN, INPUT);
	pinMode(MS_BUZZER_PIN, OUTPUT); // buzzer

	pinMode(PUMP_PIN, OUTPUT); // pump relay
	digitalWrite(PUMP_PIN, LOW);

	pinMode(SENSOR_PIN, OUTPUT); // sensor relay
	digitalWrite(SENSOR_PIN, LOW);
	Serial.begin(9600);
	init(&state);
	initActions();
}

void loop() {
	doQueueActions(&state, &actionsList);
}