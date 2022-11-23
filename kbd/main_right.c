#include "kbd_process.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int main(void) {
        my_data.side = kbd_side_RIGHT;

        kbd_process();
    }

#ifdef __cplusplus
}
#endif
