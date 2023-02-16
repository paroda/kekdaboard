#ifndef __F_UTIL_H
#define __F_UTIL_H

#include "ff15/ff.h"
#include "sd_card.h"

const char *FRESULT_str(FRESULT i);
FRESULT delete_node (
    TCHAR* path,    /* Path name buffer with the sub-directory to delete */
    UINT sz_buff,   /* Size of path name buffer (items) */
    FILINFO* fno    /* Name read buffer */
    );

void disk_register_sd_card(sd_card_t* sd);

#endif
