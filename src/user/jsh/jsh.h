#ifndef __JSH_H__
#define __JSH_H__


#include "usermacros.h"
#include "userlib.h"

#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>



#define MAX_CMD_LEN                 512


// parsing/logic elements
void parse_command(char* cmd);


// UI elements
void acquire_fb();
void write_char();
void read_char();

#endif