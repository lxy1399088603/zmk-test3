#include "calculator/key_display.h"
#include "calculator/calc_display.h"
#include "calculator/calc_engine.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

LOG_MODULE_REGISTER(key_display, CONFIG_LOG_DEFAULT_LEVEL);

#define MATRIX_COLS 5

static const char *key_names[] = {
    "CALC", "/",  "*",  "BKSP", "%",
    "7",    "8",  "9",  "-",    "",
    "4",    "5",  "6",  "+",    "",
    "1",    "2",  "3",  "=",    "",
    "0",    ".",  "",   "ENT",  "",
};

#define KEY_COUNT (sizeof(key_names) / sizeof(key_names[0]))

static int key_event_listener(const zmk_event_t *eh) {
    if (calc_engine_is_active()) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (!ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    uint32_t pos = ev->position;
    if (pos >= KEY_COUNT) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    const char *name = key_names[pos];
    if (name[0] == '\0') {
        return ZMK_EV_EVENT_BUBBLE;
    }

    LOG_DBG("Key pressed: pos=%d name=%s", pos, name);
    calc_display_update_key(name);

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(key_display_listener, key_event_listener);
ZMK_SUBSCRIPTION(key_display_listener, zmk_position_state_changed);
