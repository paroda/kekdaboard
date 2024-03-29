#include "ff15/ff.h"

const char *FRESULT_str(FRESULT i) {
    switch (i) {
    case FR_OK:
        return "Succeeded";
    case FR_DISK_ERR:
        return "A hard error occurred in the low level disk I/O layer";
    case FR_INT_ERR:
        return "Assertion failed";
    case FR_NOT_READY:
        return "The physical drive cannot work";
    case FR_NO_FILE:
        return "Could not find the file";
    case FR_NO_PATH:
        return "Could not find the path";
    case FR_INVALID_NAME:
        return "The path name format is invalid";
    case FR_DENIED:
        return "Access denied due to prohibited access or directory full";
    case FR_EXIST:
        return "Access denied due to prohibited access (exists)";
    case FR_INVALID_OBJECT:
        return "The file/directory object is invalid";
    case FR_WRITE_PROTECTED:
        return "The physical drive is write protected";
    case FR_INVALID_DRIVE:
        return "The logical drive number is invalid";
    case FR_NOT_ENABLED:
        return "The volume has no work area (mount)";
    case FR_NO_FILESYSTEM:
        return "There is no valid FAT volume";
    case FR_MKFS_ABORTED:
        return "The f_mkfs() aborted due to any problem";
    case FR_TIMEOUT:
        return "Could not get a grant to access the volume within defined "
            "period";
    case FR_LOCKED:
        return "The operation is rejected according to the file sharing "
            "policy";
    case FR_NOT_ENOUGH_CORE:
        return "LFN working buffer could not be allocated";
    case FR_TOO_MANY_OPEN_FILES:
        return "Number of open files > FF_FS_LOCK";
    case FR_INVALID_PARAMETER:
        return "Given parameter is invalid";
    default:
        return "Unknown";
    }
}

FRESULT delete_node (
    TCHAR* path,    /* Path name buffer with the sub-directory to delete */
    UINT sz_buff,   /* Size of path name buffer (items) */
    FILINFO* fno    /* Name read buffer */
    )
{
    UINT i, j;
    FRESULT fr;
    DIR dir;


    fr = f_opendir(&dir, path); /* Open the sub-directory to make it empty */
    if (fr != FR_OK) return fr;

    for (i = 0; path[i]; i++) ; /* Get current path length */
    path[i++] = '/';

    for (;;) {
        fr = f_readdir(&dir, fno);  /* Get a directory item */
        if (fr != FR_OK || !fno->fname[0]) break;   /* End of directory? */
        j = 0;
        do {    /* Make a path name */
            if (i + j >= sz_buff) { /* Buffer over flow? */
                fr = 100; break;    /* Fails with 100 when buffer overflow */
            }
            path[i + j] = fno->fname[j];
        } while (fno->fname[j++]);
        if (fno->fattrib & AM_DIR) {    /* Item is a sub-directory */
            fr = delete_node(path, sz_buff, fno);
        } else {                        /* Item is a file */
            fr = f_unlink(path);
        }
        if (fr != FR_OK) break;
    }

    path[--i] = 0;  /* Restore the path name */
    f_closedir(&dir);

    if (fr == FR_OK) fr = f_unlink(path);  /* Delete the empty sub-directory */
    return fr;
}
