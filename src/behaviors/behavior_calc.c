#define DT_DRV_COMPAT zmk_behavior_calc

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <dt-bindings/zmk/calc_keys.h>

#include "calculator/calc_engine.h"
#include "calculator/calc_display.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define CALC_LAYER 1

static int behavior_calc_init(const struct device *dev) {
    calc_engine_init();
    calc_display_init();
    return 0;
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    uint32_t param = binding->param1;

    switch (param) {
    case CALC_TOGGLE:
        if (calc_engine_is_active()) {
            calc_engine_set_active(false);
            calc_display_set_mode(false);
            zmk_keymap_layer_deactivate(CALC_LAYER);
        } else {
            calc_engine_set_active(true);
            calc_display_set_mode(true);
            zmk_keymap_layer_activate(CALC_LAYER);
        }
        return ZMK_BEHAVIOR_OPAQUE;

    case CALC_NUM_0:
    case CALC_NUM_1:
    case CALC_NUM_2:
    case CALC_NUM_3:
    case CALC_NUM_4:
    case CALC_NUM_5:
    case CALC_NUM_6:
    case CALC_NUM_7:
    case CALC_NUM_8:
    case CALC_NUM_9:
        calc_engine_input_digit(param - CALC_NUM_0);
        break;

    case CALC_DOT:
        calc_engine_input_dot();
        break;

    case CALC_OP_ADD:
        calc_engine_input_operator('+');
        break;
    case CALC_OP_SUB:
        calc_engine_input_operator('-');
        break;
    case CALC_OP_MUL:
        calc_engine_input_operator('*');
        break;
    case CALC_OP_DIV:
        calc_engine_input_operator('/');
        break;
    case CALC_OP_MOD:
        calc_engine_input_operator('%');
        break;

    case CALC_EVAL:
        calc_engine_evaluate();
        break;

    case CALC_BKSP:
        calc_engine_backspace();
        break;

    case CALC_CLEAR:
        calc_engine_reset();
        break;

    default:
        LOG_WRN("Unknown calc param: 0x%02x", param);
        return ZMK_BEHAVIOR_OPAQUE;
    }

    calc_display_update_calc();
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_calc_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

#define CALC_INST(n)                                                          \
    static int behavior_calc_init_##n(const struct device *dev) {             \
        return behavior_calc_init(dev);                                       \
    }                                                                         \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_calc_init_##n, NULL, NULL, NULL,      \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                            &behavior_calc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CALC_INST)
