#ifndef CALC_ENGINE_H
#define CALC_ENGINE_H

#include <stdbool.h>

#define CALC_EXPR_MAX_LEN 64
#define CALC_RESULT_MAX_LEN 24

typedef enum {
    CALC_STATE_IDLE,
    CALC_STATE_INPUT,
    CALC_STATE_RESULT,
    CALC_STATE_ERROR,
} calc_state_t;

void calc_engine_init(void);
void calc_engine_reset(void);

void calc_engine_input_digit(int digit);
void calc_engine_input_dot(void);
void calc_engine_input_operator(char op);
void calc_engine_evaluate(void);
void calc_engine_backspace(void);

const char *calc_engine_get_expression(void);
const char *calc_engine_get_result(void);
calc_state_t calc_engine_get_state(void);

bool calc_engine_is_active(void);
void calc_engine_set_active(bool active);

float calc_op_add(float a, float b);
float calc_op_sub(float a, float b);
float calc_op_mul(float a, float b);
float calc_op_div(float a, float b);
float calc_op_mod(float a, float b);

#endif
