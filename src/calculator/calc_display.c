#include "calculator/calc_display.h"
#include "calculator/calc_engine.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

LOG_MODULE_REGISTER(calc_display, CONFIG_LOG_DEFAULT_LEVEL);

static lv_obj_t *calc_screen;
static lv_obj_t *default_screen;
static lv_obj_t *title_label;
static lv_obj_t *expr_label;
static lv_obj_t *sep_line;
static lv_obj_t *result_label;
static bool display_initialized;

static lv_style_t style_title;
static lv_style_t style_expr;
static lv_style_t style_result;

static struct k_work display_work;

static void display_work_handler(struct k_work *work) {
    if (!display_initialized || !calc_screen) {
        return;
    }

    const char *expr = calc_engine_get_expression();
    const char *result = calc_engine_get_result();
    calc_state_t st = calc_engine_get_state();

    lv_label_set_text(expr_label, (expr[0] != '\0') ? expr : "0");

    switch (st) {
    case CALC_STATE_RESULT:
        lv_label_set_text_fmt(result_label, "= %s", result);
        break;
    case CALC_STATE_ERROR:
        lv_label_set_text(result_label, "Error");
        break;
    default:
        lv_label_set_text(result_label, "");
        break;
    }
}

static void create_styles(void) {
    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, &lv_font_unscii_8);
    lv_style_set_text_color(&style_title, lv_color_white());

    lv_style_init(&style_expr);
    lv_style_set_text_font(&style_expr, &lv_font_unscii_8);
    lv_style_set_text_color(&style_expr, lv_color_white());

    lv_style_init(&style_result);
    lv_style_set_text_font(&style_result, lv_font_default());
    lv_style_set_text_color(&style_result, lv_color_white());
}

void calc_display_init(void) {
    k_work_init(&display_work, display_work_handler);
    create_styles();
    display_initialized = true;
    LOG_DBG("Calculator display initialized");
}

void calc_display_show(void) {
    if (!display_initialized) {
        calc_display_init();
    }

    default_screen = lv_scr_act();

    calc_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(calc_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(calc_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(calc_screen, LV_OBJ_FLAG_SCROLLABLE);

    title_label = lv_label_create(calc_screen);
    lv_obj_add_style(title_label, &style_title, 0);
    lv_label_set_text(title_label, "[ CALC ]");
    lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 2, 2);

    static lv_point_t line_pts[] = {{0, 0}, {124, 0}};
    sep_line = lv_line_create(calc_screen);
    lv_line_set_points(sep_line, line_pts, 2);
    lv_obj_set_style_line_color(sep_line, lv_color_white(), 0);
    lv_obj_set_style_line_width(sep_line, 1, 0);
    lv_obj_align(sep_line, LV_ALIGN_TOP_LEFT, 2, 14);

    expr_label = lv_label_create(calc_screen);
    lv_obj_add_style(expr_label, &style_expr, 0);
    lv_obj_set_width(expr_label, 124);
    lv_label_set_long_mode(expr_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(expr_label, "0");
    lv_obj_align(expr_label, LV_ALIGN_TOP_LEFT, 2, 18);

    result_label = lv_label_create(calc_screen);
    lv_obj_add_style(result_label, &style_result, 0);
    lv_obj_set_width(result_label, 124);
    lv_obj_set_style_text_align(result_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(result_label, "");
    lv_obj_align(result_label, LV_ALIGN_BOTTOM_RIGHT, -2, -2);

    lv_scr_load(calc_screen);
    LOG_INF("Calculator display shown");
}

void calc_display_hide(void) {
    if (default_screen) {
        lv_scr_load(default_screen);
    }
    if (calc_screen) {
        lv_obj_del(calc_screen);
        calc_screen = NULL;
    }
    title_label = NULL;
    expr_label = NULL;
    sep_line = NULL;
    result_label = NULL;
    LOG_INF("Calculator display hidden");
}

void calc_display_update_content(void) {
    if (!display_initialized || !calc_screen) {
        return;
    }
    k_work_submit(&display_work);
}
