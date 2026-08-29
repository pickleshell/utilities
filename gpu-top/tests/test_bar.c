#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
int main(void) {
    char b[64];
    make_bar(0, 10, b, sizeof b);    assert(strcmp(b, "----------") == 0);
    make_bar(100, 10, b, sizeof b);  assert(strcmp(b, "##########") == 0);
    make_bar(50, 10, b, sizeof b);   assert(strcmp(b, "#####-----") == 0);
    make_bar(120, 10, b, sizeof b);  assert(strcmp(b, "##########") == 0); /* clamp */
    make_bar(-10, 10, b, sizeof b);  assert(strcmp(b, "----------") == 0); /* clamp */
    make_bar(30, 10, b, sizeof b);   assert(strcmp(b, "###-------") == 0);
    make_bar(42, 0, b, sizeof b);    assert(strcmp(b, "") == 0);          /* zero width */
    printf("test_bar PASS\n");
    return 0;
}
