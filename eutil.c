#include "emalloc.h"

void panic(const char *error)
{
    write(2, error, strlen(error));
    abort();
}