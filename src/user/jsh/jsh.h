#ifndef __JSH_H__
#define __JSH_H__


#include <stdint.h>


#define MAX_CMD_LEN                 512


// parsing/logic elements
void parse_command(char* cmd);


// UI elements
void acquire_fb();
void write_char();
void read_char();

extern void syscall(u64 sys_num, ...);

#endif