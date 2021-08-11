// ActionsList.h

#ifndef _ACTIONSLIST_h
#define _ACTIONSLIST_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif


#endif

typedef enum ActionState {
	MS_NON_ACTIVE = 0,
	MS_SCHEDULED = 1,
	MS_PENDING = 2,
	MS_RUNNING = 3,
	MS_CHILD_RUNNING = 4,
	MS_CHILD_PENDING = 5,
	MS_CHILD_SCHEDULED = 6
};

typedef struct Action {
	char* name;
	Action* next = nullptr;
	Action* prev = nullptr;
	bool clear;
	bool frozen = false; // once scheduled the action is never descheduled
	bool stopRequested = false;
	unsigned long ti = 0; // interval between executions
	unsigned long td = 1000; // maximum duration interval; '0' makes the action execute forever.
	unsigned long lst = 0; // last stop time
	unsigned long to = 0; // first tick offset from start
	unsigned long st = 0; // last start time
	ActionState state = MS_NON_ACTIVE;
	void (*tick)(Action*);
	void (*start)(Action*);
	void (*stop)(Action*);
	Action* child = nullptr; // a child action; Child actions are activated after the master action has been activated;
	   // child actions are deactivated before the master action is deactivated;
	   // child actions are not part of the execution queue
};

typedef struct ActionsList {
	size_t count = 0;
	size_t availableActionsCount = 0;
	Action* first = nullptr;
	Action* last = nullptr;
	Action* availableActions = nullptr;
};

bool requestStop(ActionsList* list, Action* a);
bool descheduleAction(ActionsList* list, Action* a);
bool scheduleAction(ActionsList* list, Action* a);
void doQueueActions(ActionsList* executionList);
int initActionsList(int actionsCount);


