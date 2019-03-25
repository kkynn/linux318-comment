#ifndef _FATAL_SIGNAL_H
#define _FATAL_SIGNAL_H

#define MAX_EXIT_PROCESS			16
#define MAX_PROCESS_NAME_LEN		32

typedef struct fatalSignal_s {
	char			pName[MAX_PROCESS_NAME_LEN];
	unsigned char	signal_no;
} fatalSignal_t;


#endif
