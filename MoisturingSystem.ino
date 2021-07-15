#include <EEPROM.h>
#define ACTIONS_COUNT 7

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
#define PIN_VALVE_EON 9

#define BUTTONS_PIN A3

//#undef DEBUG
#define DEBUG

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
	FINISHING = 4,
	CHILD_RUNNING = 5,
	CHILD_PENDING = 6
};

// Structures

struct MSysSettings {
	unsigned long siw = 5000; // 5 seconds for sensor interval while watering
	unsigned long sid = 60000 * 1; // 1 minute for sensor interval while in stand by
	unsigned long sd = 1000; // 1 second max duration for sensor activation;
	unsigned long pi = 60000 * 10; // 10 minutes between pump activations
	unsigned long pd = 60000 * 2; // 2 minutes max pump on duration
	long apv = 10; // 10% humidity threshold for pump activation
	long dapv = 60; // 60% humidity threshold for pump deactivation
} settings;

struct Sensor {
	int wet;
	int dry;
	int value;
	int ac;
};

struct Sensors {
	size_t s;
	Sensor* d;
};

struct SystemState {
	Sensors s = { 3, (Sensor*)malloc(sizeof(Sensor) * 3) };
	bool p = false;
} state;

struct Action {
	int code;
	bool frozen = false;
	bool stopRequested = false;
	unsigned long ti = 0; // interval between executions
	unsigned long td = 1000; // maximum duration interval
	unsigned long lst = -1; // last stop time
	unsigned long to = 200; // first tick offset from start
	unsigned long st = 0; // last start time
	ActionState state = NON_ACTIVE;
	void (*tick)(SystemState*, Action*);
	void (*start)(SystemState*, Action*);
	void (*stop)(SystemState*, Action*);
	Action* child = nullptr;
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

#ifdef DEBUG

void printList(ActionsList* list) {
	ListItem* cur = (*list).first;

	while (cur != nullptr) {
		Serial.print(" Item: ");
		Serial.print((*(*cur).value).code);
		Serial.flush();
		cur = (*cur).next;
	}

	Serial.println();
}

#endif

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
#ifdef DEBUG
			printList(list);
#endif
			return item;
		}
		else {
#ifdef DEBUG
			Serial.print("Could not allocate memory for a new ListItem");
#endif
		}
	}
	Serial.println("Item to add null");

	return nullptr;
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
#ifdef DEBUG
	printList(list);
#endif
	return true;
}

// end of Queue

int printAndReturnValue(int value) {
#ifdef DEBUG
	Serial.print("Value: ");
	Serial.print(value);
	Serial.println();
#endif
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

void populateSettings(SystemState* state, ActionsArray* actions) {


	// read sensors action
	(*actions).a[0].tick = &tickSensors;
	(*actions).a[0].frozen = true;
	(*actions).a[0].stopRequested = false;
	(*actions).a[0].start = &startSensors;
	(*actions).a[0].stop = &stopSensors;
	(*actions).a[0].ti = settings.siw;
	(*actions).a[0].td = settings.sd;
	(*actions).a[0].to = 200;
	(*actions).a[0].code = 1;
	(*actions).a[0].state = NON_ACTIVE;
	(*actions).a[0].child = nullptr;
	(*actions).a[0].lst = 0;
	(*actions).a[0].st = 0;

	// settings action
	(*actions).a[1].tick = &tickModifySettings;
	(*actions).a[1].frozen = true;
	(*actions).a[1].stopRequested = false;
	(*actions).a[1].start = &startModifySettings;
	(*actions).a[1].stop = &stopModifySettings;
	(*actions).a[1].ti = 100;
	(*actions).a[1].td = 300;
	(*actions).a[1].code = 2;
	(*actions).a[1].to = 0;
	(*actions).a[1].state = NON_ACTIVE;
	(*actions).a[1].child = nullptr;
	(*actions).a[1].lst = 0;
	(*actions).a[1].st = 0;

	// sound buzzer action
	(*actions).a[2].tick = &tickBuzzer;
	(*actions).a[2].frozen = false;
	(*actions).a[2].stopRequested = false;
	(*actions).a[2].start = &startBuzzer;
	(*actions).a[2].stop = &stopBuzzer;
	(*actions).a[2].ti = 0;
	(*actions).a[2].td = 300;
	(*actions).a[2].code = 3;
	(*actions).a[2].to = 0;
	(*actions).a[2].state = NON_ACTIVE;
	(*actions).a[2].child = nullptr;
	(*actions).a[2].lst = 0;
	(*actions).a[2].st = 0;

	// open far valve action
	(*actions).a[3].tick = &tickFarOutlet;
	(*actions).a[3].frozen = false;
	(*actions).a[3].stopRequested = false;
	(*actions).a[3].start = &startFarOutlet;
	(*actions).a[3].stop = &stopFarOutlet;
	(*actions).a[3].ti = settings.pi;
	(*actions).a[3].td = settings.pd;
	(*actions).a[3].code = 4;
	(*actions).a[3].to = 0;
	(*actions).a[3].state = NON_ACTIVE;
	(*actions).a[3].child = &(*actions).a[6];
	(*actions).a[3].lst = 0;
	(*actions).a[3].st = 0;


	(*state).s.d[far].ac = (*actions).a[3].code;

	// open mid valve action
	(*actions).a[4].tick = &tickMidOutlet;
	(*actions).a[4].frozen = false;
	(*actions).a[4].stopRequested = false;
	(*actions).a[4].start = &startMidOutlet;
	(*actions).a[4].stop = &stopMidOutlet;
	(*actions).a[4].ti = settings.pi;
	(*actions).a[4].td = settings.pd;
	(*actions).a[4].code = 5;
	(*actions).a[4].to = 0;
	(*actions).a[4].state = NON_ACTIVE;
	(*actions).a[4].child = &(*actions).a[6];
	(*actions).a[4].lst = 0;
	(*actions).a[4].st = 0;

	(*state).s.d[mid].ac = (*actions).a[4].code;

	// open near valve action
	(*actions).a[5].tick = &tickNearOutlet;
	(*actions).a[5].frozen = false;
	(*actions).a[5].stopRequested = false;
	(*actions).a[5].start = &startNearOutlet;
	(*actions).a[5].stop = &stopNearOutlet;
	(*actions).a[5].ti = settings.pi;
	(*actions).a[5].td = settings.pd;
	(*actions).a[5].code = 6;
	(*actions).a[5].to = 0;
	(*actions).a[5].state = NON_ACTIVE;
	(*actions).a[5].child = &(*actions).a[6];
	(*actions).a[5].lst = 0;
	(*actions).a[5].st = 0;

	(*state).s.d[near].ac = (*actions).a[5].code;

	// start pump action
	(*actions).a[6].tick = &tickPump;
	(*actions).a[6].frozen = false;
	(*actions).a[6].stopRequested = false;
	(*actions).a[6].start = &startPump;
	(*actions).a[6].stop = &stopPump;
	(*actions).a[6].ti = 0;
	(*actions).a[6].td = 0;
	(*actions).a[6].code = 7;
	(*actions).a[6].to = 0;
	(*actions).a[6].state = NON_ACTIVE;
	(*actions).a[6].child = nullptr;
	(*actions).a[6].lst = 0;
	(*actions).a[6].st = 0;
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

void scheduleBuzzerAction(int millis) {
	actions.a[2].td = millis;
	scheduleAction(&actionsList, &actions.a[2]);
}

void soundBuzzer(int millis) {
	analogWrite(MS_BUZZER_PIN, MS_BUZZER_VOLUME);
	delay(millis);
	analogWrite(MS_BUZZER_PIN, 0);
}

bool descheduleAction(ActionsList* list, Action* a) {
	ListItem* found = find(list, a);
	if (found == nullptr) {
		return false;
	}

	Action* foundAction = (*found).value;

	if (foundAction != nullptr && (*foundAction).state != RUNNING) {
		(*foundAction).state = NON_ACTIVE;

		removeListItem(&actionsList, foundAction);

		return true;
	}

	return false;
}

Action* findActionByCode(int code) {
	for (int i = 0; i < ACTIONS_COUNT; i++) {
		Action* current = &actions.a[i];
		if ((*current).code == code) {
			return current;
		}
	}

	return nullptr;
}


bool scheduleAction(ActionsList* list, Action* a) {
	ListItem* found = find(list, a);
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
	if (actions.a != nullptr) {
		populateSettings(state, &actions);
		scheduleAction(&actionsList, &actions.a[0]);
		scheduleAction(&actionsList, &actions.a[1]);
	}
	else {
#ifdef DEBUG
		Serial.print("COULD NOT ALLOCATE MEMORY FOR ACTIONS");
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

	extractMedianPinValueForProperty(0, &(*state).s.d[near].value, &(*state).s.d[mid].value, &(*state).s.d[far].value);

	bool activate = false;
	Action* toSchedule = nullptr;
	Action** acandidates = (Action**)malloc(sizeof(Action*) * (*state).s.s);
	int pc = 0;
	int ac = 0;
	for (int i = 0; i < (*state).s.s; i++) {
		Sensor* se = &(*state).s.d[i];
		float v = (float)(*se).value;
		float d = (float)(*se).dry;
		float w = (float)(*se).wet;
		float dwd = max(0.00001, d - w);

		float mp = ((d - min(max(v, w), d)) / dwd) * 100.0;
#ifdef DEBUG
		Serial.print("Current percentage: ");
		Serial.print(mp);
		Serial.println();
#endif
		activate = (bool)((long)mp < ((*state).p ? settings.dapv : settings.apv));
		Action* ca = findActionByCode((*se).ac);

		acandidates[i] = nullptr;
		if (activate && ca != nullptr) {
			ac++;
			if ((*ca).state == NON_ACTIVE) {
				acandidates[i] = ca;
				pc++;
			}
			else if ((*ca).state == PENDING) {
				descheduleAction(&actionsList, ca);
			}
		}
		else if (ca != nullptr) {
			// Stop the action for which activate is false
			requestStop(&actionsList, ca);
		}
	}

	// if the available outlets are equal to the 
	// ones needed to be activate - we select the 
	// one that was activated the earliest
	if (pc == ac) {

		Action* ts = nullptr;
		for (int i = 0; i < (*state).s.s; i++) {
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
			scheduleAction(&actionsList, ts);
		}
	}

	(*a).ti = actions.a[6].state == CHILD_RUNNING ? settings.siw : settings.sid;
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
#ifdef DEBUG
		digitalWrite(SENSOR_PIN, SENSOR_PIN_HIGH);
#endif
		while (bs < 200 || bs > 280) {
			bs = analogRead(BUTTONS_PIN);
			//#ifdef DEBUG
			//			extractMedianPinValueForProperty(1000, &(*state).s.d[near].dry, &(*state).s.d[mid].dry, &(*state).s.d[far].dry);
			//			delay(1000);
			//#endif
		}

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

		digitalWrite(SENSOR_PIN, SENSOR_PIN_LOW);
	}
}

bool requestStop(ActionsList* list, Action* a) {
	ListItem* item = find(list, a);
	if (item != nullptr) {
		(*a).stopRequested = true;
		return true;
	}

	return false;
}

bool canStart(Action* a, unsigned long time) {

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
		Serial.print("Stop requested");
#endif
		return true;
	}

	if ((*a).td > 0 && (*a).st > 0 && (time - (*a).st >= (*a).td)) {
		return true;
	}

	return false;
}

void doQueueActions(SystemState* state, ActionsList* list) {
	ListItem* current = (*list).first;
	unsigned long time = millis();
	int count = (*list).count;

	Action** scheduled = (Action**)malloc(sizeof(Action*) * count);

	if (scheduled != nullptr) {
		int i = 0;

		while (current != nullptr) {
			scheduled[i++] = (*current).value;
			current = (*current).next;
		}

		for (int k = 0; k < count; k++) {
			Action* action = scheduled[k];

			if (canStart(action, time)) {
#ifdef DEBUG
				Serial.print("Starting: ");
				Serial.print((*action).code);
				Serial.println();
#endif

				(*action).start(state, action);
				(*action).state = RUNNING;
				(*action).st = time;

				Action* child = (*action).child;
				while (child != nullptr) {
#ifdef DEBUG
					Serial.print("Stopping child: ");
					Serial.println((*child).code);
#endif
					(*child).start(state, child);
					(*child).state = CHILD_RUNNING;
					child = (*child).child;
				}
			}
			else {
				if ((*action).state == RUNNING) {
					if (shouldStop(action, time)) {

#ifdef DEBUG
						Serial.print("Stopping: ");
						Serial.print((*action).code);
						Serial.println();
#endif

						(*action).stop(state, action);
						(*action).lst = time;
						(*action).state = PENDING;

						if (!(*action).frozen) {
							descheduleAction(&actionsList, action);
						}

						Action* child = (*action).child;
						while (child != nullptr) {
#ifdef DEBUG
							Serial.print("Stopping child: ");
							Serial.println((*child).code);
#endif
							(*child).stop(state, child);
							(*child).state = NON_ACTIVE;
							child = (*child).child;
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
#ifdef DEBUG
		Serial.print("COULD NOT ALLOCATE MEMORY FOR SCHEDULED ACTIONS!!!");
#endif
	}

	free(scheduled);
}

void setup() {
	pinMode(PIN_FAR, INPUT);
	pinMode(PIN_MID, INPUT);
	pinMode(PIN_NEAR, INPUT);

	pinMode(PIN_VALVE_FAR, OUTPUT);
	pinMode(PIN_VALVE_MID, OUTPUT);
	pinMode(PIN_VALVE_NEAR, OUTPUT);
	pinMode(PIN_VALVE_EON, OUTPUT);

	digitalWrite(PIN_VALVE_FAR, HIGH);
	digitalWrite(PIN_VALVE_MID, HIGH);
	digitalWrite(PIN_VALVE_NEAR, HIGH);
	digitalWrite(PIN_VALVE_EON, HIGH);


	pinMode(BUTTONS_PIN, INPUT);
	pinMode(MS_BUZZER_PIN, OUTPUT); // buzzer

	pinMode(PUMP_PIN, OUTPUT); // pump relay
	digitalWrite(PUMP_PIN, PUMP_PIN_LOW);

	pinMode(SENSOR_PIN, OUTPUT); // sensor relay
	digitalWrite(SENSOR_PIN, SENSOR_PIN_LOW);
	Serial.begin(9600);
	init(&state);
	initActions(&state);
}

void loop() {
	doQueueActions(&state, &actionsList);
}