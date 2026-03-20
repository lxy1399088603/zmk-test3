#ifndef CALC_DISPLAY_H
#define CALC_DISPLAY_H

#include <stdbool.h>

void calc_display_init(void);
void calc_display_set_mode(bool calc_mode);
void calc_display_update_calc(void);
void calc_display_update_key(const char *key_name);

#endif
