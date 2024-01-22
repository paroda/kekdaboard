#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    bool has_motion;
    bool on_surface;
    int16_t dx;
    int16_t dy;
} kbd_tb_motion_t;

typedef enum {
    kbd_info_screen_welcome = 0x00,
    kbd_info_screen_home = 0x01,
    kbd_config_screen_date = 0x80
} kbd_screen_t;

typedef struct {
    kbd_screen_t screen;
    bool caps_lock;
    bool num_lock;
    bool scroll_lock;
} kbd_state_t;

void test() {
    printf("\nsizeof(kbd_tb_motion_t): %lu", sizeof(kbd_tb_motion_t));

    printf("\nsizeof(kbd_screent_t): %lu", sizeof(kbd_screen_t));
    printf("\nsizeof(kbd_state_t): %lu", sizeof(kbd_state_t));
}

void test2() {
    bool a = 1;
    bool b = 2;
    printf("\na==b: %d", a==b);

    kbd_state_t sa, sb;
    sb.caps_lock = 23;
    printf("\nsb.caps_lock: %d", sb.caps_lock);
    memcpy(&sa, &sb, sizeof(kbd_state_t));
    sb.caps_lock = true;
    printf("\nsa.caps_lock==sb.caps_lock: %d", sa.caps_lock==sb.caps_lock);

    printf("\nsa==sb: %d", memcmp(&sa, &sb, sizeof(kbd_state_t))==0);
}

void test3() {
    kbd_tb_motion_t ta = {.has_motion=false, .on_surface=true, .dx=100, .dy=3000};
    kbd_tb_motion_t tb = ta;
    printf("\ntb==ta: %d", ta.has_motion==tb.has_motion && ta.on_surface==tb.on_surface
           && ta.dx==tb.dx && ta.dy==tb.dy);
}

void test4() {
    kbd_state_t sa = {kbd_info_screen_home, false, true, false};
    ((uint8_t*)&sa)[7]=1;
    kbd_state_t sb = sa;
    printf("\nsa:");
    for(int i=0; i<sizeof(kbd_state_t); i++)
        printf(" %u", ((uint8_t*)&sa)[i]);
    printf("\nsb:");
    for(int i=0; i<sizeof(kbd_state_t); i++)
        printf(" %u", ((uint8_t*)&sb)[i]);
    printf("\ns=: %d, %d, %d, %d", sa.screen, sa.caps_lock, sa.num_lock, sa.scroll_lock);
    printf("\nsa==sb: %d", memcmp(&sa, &sb, sizeof(kbd_state_t))==0);
}

int main(void) {
    printf("\nTest struct packing\n");

    test();

    test2();

    test3();

    test4();

    printf("\nEnd of Test struct packing\n");
}
