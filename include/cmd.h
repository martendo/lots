#ifndef LOTS_CMD_H
#define LOTS_CMD_H

#include "ctl.h"

enum cmd {
	CMD_UNKNOWN,
	CMD_UP,
	CMD_DOWN,
	CMD_QUIT
};

enum cmd getcmd(const struct lotsctl *const);

#endif
