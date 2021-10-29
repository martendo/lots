#ifndef LOTS_CMD_H
#define LOTS_CMD_H

enum cmd {
	CMD_UNKNOWN,
	CMD_UP,
	CMD_DOWN,
	CMD_QUIT
};

enum cmd getcmd(void);

#endif
