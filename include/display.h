#ifndef LOTS_DISPLAY_H
#define LOTS_DISPLAY_H

#include "ctl.h"

void clear_status(void);
void print_status(const struct lotsctl *const);

void move_forwards(struct lotsctl *const, unsigned long);
void move_backwards(struct lotsctl *const, unsigned long);

int display_file(struct lotsctl *const, const int);
void switch_file(struct lotsctl *const, const int);

#endif
