#ifndef SD_CRC_H
#define SD_CRC_H

#include <stddef.h>

char crc7(const char* data, int length);
unsigned short crc16(const char* data, int length);
void update_crc16(unsigned short *pCrc16, const char data[], size_t length);

#endif
