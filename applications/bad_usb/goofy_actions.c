#include "goofy_actions.h"

#include <stdlib.h>
#include <stddef.h>

void goofy_action_string(string_t string) {
    string = string;
    //printf("string: %s\n", string_get_cstr(string));
}

void goofy_action_stringln(string_t string) {
    string = string;
    //printf("stringln: %s\n", string_get_cstr(string));
}

void goofy_action_delay(int32_t delay) {
    delay = delay;
    //printf("delay: %lms\n", delay);
    //Sleep(delay);
}
