#ifndef DUMP_H
#define DUMP_H

#include "types.h"

int is_self(const char *fn);
void decrypt_and_dump_self(char *selfFile, char *saveFile);

#endif
