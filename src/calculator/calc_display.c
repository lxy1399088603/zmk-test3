#include "calculator/calc_display.h"
#include "calculator/calc_engine.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <string.h>

LOG_MODULE_REGISTER(calc_display, CONFIG_LOG_DEFAULT_LEVEL);

static lv_obj_t *screen;
static lv_obj_t *title_label;
static lv_obj_t *sep_label;
static lv_obj_t *line1_label;
static lv_obj_t *line2_label;

static bool screen_ready;
static bool is_calc_mode;

static lv_style_t style_small;
static lv_style_t style_large;

static struct k_work calc_work;
static struct k_work key_work;
static struct k_work_delayable boot_work;

static char pending_key[16];

static void create_styles(void) {
    lv_style_init(&style_small);
    lv_style_set_text_font(&style_small, &lv_font_unscii_8);
    lv_style_set_text_color(&style_small, lv_color_white());

    lv_style_init(&style_large);
    lv_style_set_text_font(&style_large, lv_font_default());
    lv_style_set_text_color(&style_large, lv_color_white());
}

static void create_screen(void) {
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    title_label = lv_label_create(screen);
    lv_obj_add_style(title_label, &style_small, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 2, 2);

    sep_label = lv_label_create(screen);
    lv_obj_add_style(sep_label, &style_small, 0);
    lv_label_set_text(sep_label, "--------------------");
    lv_obj_align(sep_label, LV_ALIGN_TOP_LEFT, 2, 13);

    line1_label = lv_label_create(screen);
    lv_obj_add_style(line1_label, &style_small, 0);
    lv_obj_set_width(line1_label, 124);
    lv_label_set_long_mode(line1_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(line1_label, LV_ALIGN_TOP_LEFT, 2, 26);

    line2_label = lv_label_create(screen);
    lv_obj_add_style(line2_label, &style_large, 0);
    lv_obj_set_width(line2_label, 124);
    lv_obj_set_style_text_align(line2_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(line2_label, LV_ALIGN_BOTTOM_RIGHT, -2, -4);

    lv_label_set_text(title_label, "[ CalcPad ]");
    lv_label_set_text(line1_label, "Ready");
    lv_label_set_text(line2_label, "");

    lv_scr_load(screen);
    screen_ready = true;
    LOG_INF("Display screen created");
}

static void calc_work_handler(struct k_work *work) {
    if (!screen_ready) {
        return;
    }
    const char *expr = calc_engine_get_expression();
    const char *result = calc_engine_get_result();
    calc_state_t st = calc_engine_get_state();

    lv_label_set_text(line1_label, (expr[0] != '\0') ? expr : "0");

    switch (st) {
    case CALC_STATE_RESULT:
        lv_label_set_text_fmt(line2_label, "= %s", result);
        break;
    case CALC_STATE_ERROR:
        lv_label_set_text(line2_label, "Error");
        break;
    default:
        lv_label_set_text(line2_label, "");
        break;
    }
}

static void key_work_handler(struct k_work *work) {
    if (!screen_ready || is_calc_mode) {
        return;
    }
    lv_label_set_text(line1_label, pending_key);
}

static void boot_work_handler(struct k_work *work) {
    create_styles();
    create_screen();
    LOG_INF("Boot display ready");
}

void calc_display_init(void) {
    k_work_init(&calc_work, calc_work_handler);
    k_work_init(&key_work, key_work_handler);
    k_work_init_delayable(&boot_work, boot_work_handler);
    k_work_schedule(&boot_work, K_SECONDS(2));
}

void calc_display_set_mode(bool calc_mode) {
    is_calc_mode = calc_mode;
    if (!screen_ready) {
        return;
    }

    if (calc_mode) {
        lv_label_set_text(title_label, "[ CALC ]");
        lv_label_set_text(line1_label, "0");
        lv_label_set_text(line2_label, "");
    } else {
        lv_label_set_text(title_label, "[ CalcPad ]");
        lv_label_set_text(line1_label, "Ready");
        lv_label_set_text(line2_label, "");
    }
}

void calc_display_update_calc(void) {
    if (!screen_ready) {
        return;
    }
    k_work_submit(&calc_work);
}

void calc_display_update_key(const char *key_name) {
    if (!screen_ready || is_calc_mode) {
        return;
    }
    strncpy(pending_key, key_name, sizeof(pending_key) - 1);
    pending_key[sizeof(pending_key) - 1] = '\0';
    k_work_submit(&key_work);
}
