#include "config.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    gpu_top_config c = {1, 0, 60, 85};

    assert(config_parse_line("refresh = 2", &c) == 1);
    assert(c.refresh_s == 2);

    assert(config_parse_line("warn=70", &c) == 1);
    assert(c.warn == 70);

    assert(config_parse_line("crit = 90", &c) == 1);
    assert(c.crit == 90);

    assert(config_parse_line("device = 1", &c) == 1);
    assert(c.device == 1);

    /* malformed / unknown lines are ignored and return 0, leaving prior
       values intact */
    assert(config_parse_line("garbage", &c) == 0);
    assert(config_parse_line("bogus = 5", &c) == 0);
    assert(c.refresh_s == 2);
    assert(c.warn == 70);
    assert(c.crit == 90);
    assert(c.device == 1);

    /* blank and comment-like lines are ignored */
    assert(config_parse_line("   ", &c) == 0);
    assert(config_parse_line("# comment", &c) == 0);

    printf("test_config PASS\n");
    return 0;
}
