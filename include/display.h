#ifndef LOTS_DISPLAY_H
#define LOTS_DISPLAY_H

#include "ctl.h"

void print_status(const struct lotsctl *const);

void move_forwards(struct lotsctl *const, int);

void display_file(struct lotsctl *const, const char *const);

#endif
