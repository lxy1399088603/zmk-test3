#include "calculator/calc_engine.h"
#include <string.h>
#include <math.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(calc_engine, CONFIG_LOG_DEFAULT_LEVEL);

static char expression[CALC_EXPR_MAX_LEN + 1];
static char result_str[CALC_RESULT_MAX_LEN + 1];
static int expr_len;
static calc_state_t state;
static bool active;

/* ---------- 运算函数（每个操作符独立绑定） ---------- */

float calc_op_add(float a, float b) { return a + b; }
float calc_op_sub(float a, float b) { return a - b; }
float calc_op_mul(float a, float b) { return a * b; }

float calc_op_div(float a, float b) {
    if (b == 0.0f) {
        return NAN;
    }
    return a / b;
}

float calc_op_mod(float a, float b) {
    if (b == 0.0f) {
        return NAN;
    }
    return fmodf(a, b);
}

/* ---------- 表达式解析器（递归下降，支持优先级） ---------- */

static bool is_digit_char(char c) { return c >= '0' && c <= '9'; }

static float parse_number(const char **s) {
    float result = 0.0f;
    while (is_digit_char(**s)) {
        result = result * 10.0f + (float)(**s - '0');
        (*s)++;
    }
    if (**s == '.') {
        (*s)++;
        float frac = 0.1f;
        while (is_digit_char(**s)) {
            result += (float)(**s - '0') * frac;
            frac *= 0.1f;
            (*s)++;
        }
    }
    return result;
}

static float parse_term(const char **s) {
    float result = parse_number(s);
    while (**s == '*' || **s == '/' || **s == '%') {
        char op = **s;
        (*s)++;
        float right = parse_number(s);
        switch (op) {
        case '*':
            result = calc_op_mul(result, right);
            break;
        case '/':
            result = calc_op_div(result, right);
            break;
        case '%':
            result = calc_op_mod(result, right);
            break;
        }
    }
    return result;
}

static float parse_expr(const char **s) {
    float result = parse_term(s);
    while (**s == '+' || **s == '-') {
        char op = **s;
        (*s)++;
        float right = parse_term(s);
        switch (op) {
        case '+':
            result = calc_op_add(result, right);
            break;
        case '-':
            result = calc_op_sub(result, right);
            break;
        }
    }
    return result;
}

static bool is_operator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%';
}

static void format_result(float val) {
    if (isnan(val) || isinf(val)) {
        strncpy(result_str, "Error", sizeof(result_str));
        state = CALC_STATE_ERROR;
        return;
    }

    if (val == (float)(int)val && fabsf(val) < 1e9f) {
        snprintf(result_str, sizeof(result_str), "%d", (int)val);
    } else {
        snprintf(result_str, sizeof(result_str), "%.7g", (double)val);
    }
}

/* ---------- 公共接口 ---------- */

void calc_engine_init(void) {
    calc_engine_reset();
    active = false;
}

void calc_engine_reset(void) {
    memset(expression, 0, sizeof(expression));
    memset(result_str, 0, sizeof(result_str));
    expr_len = 0;
    state = CALC_STATE_IDLE;
    LOG_DBG("Calculator reset");
}

void calc_engine_input_digit(int digit) {
    if (state == CALC_STATE_ERROR) {
        calc_engine_reset();
    }
    if (state == CALC_STATE_RESULT) {
        calc_engine_reset();
    }
    if (expr_len >= CALC_EXPR_MAX_LEN) {
        return;
    }

    expression[expr_len++] = '0' + digit;
    expression[expr_len] = '\0';
    state = CALC_STATE_INPUT;
}

void calc_engine_input_dot(void) {
    if (state == CALC_STATE_ERROR) {
        calc_engine_reset();
    }
    if (state == CALC_STATE_RESULT) {
        calc_engine_reset();
    }
    if (expr_len >= CALC_EXPR_MAX_LEN) {
        return;
    }

    int i = expr_len - 1;
    while (i >= 0 && !is_operator(expression[i])) {
        if (expression[i] == '.') {
            return;
        }
        i--;
    }

    if (expr_len == 0 || is_operator(expression[expr_len - 1])) {
        expression[expr_len++] = '0';
    }

    expression[expr_len++] = '.';
    expression[expr_len] = '\0';
    state = CALC_STATE_INPUT;
}

void calc_engine_input_operator(char op) {
    if (state == CALC_STATE_ERROR) {
        calc_engine_reset();
        return;
    }
    if (expr_len == 0) {
        return;
    }

    if (state == CALC_STATE_RESULT) {
        strncpy(expression, result_str, CALC_EXPR_MAX_LEN);
        expr_len = strlen(expression);
        memset(result_str, 0, sizeof(result_str));
    }

    if (is_operator(expression[expr_len - 1])) {
        expression[expr_len - 1] = op;
    } else {
        if (expr_len >= CALC_EXPR_MAX_LEN) {
            return;
        }
        expression[expr_len++] = op;
    }
    expression[expr_len] = '\0';
    state = CALC_STATE_INPUT;
}

void calc_engine_evaluate(void) {
    if (expr_len == 0 || state == CALC_STATE_ERROR) {
        return;
    }

    if (is_operator(expression[expr_len - 1])) {
        expression[--expr_len] = '\0';
    }

    if (expr_len == 0) {
        return;
    }

    const char *ptr = expression;
    float result = parse_expr(&ptr);

    format_result(result);
    state = (state == CALC_STATE_ERROR) ? CALC_STATE_ERROR : CALC_STATE_RESULT;
    LOG_DBG("Eval: %s = %s", expression, result_str);
}

void calc_engine_backspace(void) {
    if (state == CALC_STATE_ERROR || state == CALC_STATE_RESULT) {
        calc_engine_reset();
        return;
    }
    if (expr_len > 0) {
        expression[--expr_len] = '\0';
    }
    if (expr_len == 0) {
        state = CALC_STATE_IDLE;
    }
}

const char *calc_engine_get_expression(void) {
    return expression;
}

const char *calc_engine_get_result(void) {
    return result_str;
}

calc_state_t calc_engine_get_state(void) {
    return state;
}

bool calc_engine_is_active(void) {
    return active;
}

void calc_engine_set_active(bool val) {
    active = val;
    if (active) {
        calc_engine_reset();
        LOG_INF("Calculator activated");
    } else {
        LOG_INF("Calculator deactivated");
    }
}
