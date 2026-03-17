#include "keypad_mgr.h"
#include <Keypad.h>
#include "../core/config.h"
#include "../core/event_bus.h"

static const byte ROWS = 4;
static const byte COLS = 4;

static char keys[ROWS][COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

static byte rowPins[ROWS] = {
    (byte)KEYPAD_ROW_PINS[0],
    (byte)KEYPAD_ROW_PINS[1],
    (byte)KEYPAD_ROW_PINS[2],
    (byte)KEYPAD_ROW_PINS[3],
};

static byte colPins[COLS] = {
    (byte)KEYPAD_COL_PINS[0],
    (byte)KEYPAD_COL_PINS[1],
    (byte)KEYPAD_COL_PINS[2],
    (byte)KEYPAD_COL_PINS[3],
};

static Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

static char lastKey = 0;

void keypadInit() {
    keypad.setDebounceTime(30);
}

void keypadUpdate() {
    char key = keypad.getKey();
    if (key) {
        lastKey = key;
        Event evt;
        evt.type = EVT_KEYPAD_INPUT;
        evt.value = key;
        eventBusPublish(evt);
    }
}

char keypadReadKey() {
    char k = lastKey;
    lastKey = 0;
    return k;
}
