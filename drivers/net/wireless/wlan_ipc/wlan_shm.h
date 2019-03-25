#ifndef __WLAN_SHM_H__
#define __WLAN_SHM_H__

#define WLAN_IPC_MEM_SIZE				2048
#define WLAN_BUFFER_SIZE 				2000

#define AIPC_WLAN_SHM_BASE				(0xB0000000 + CONFIG_RTL8686_IPC_MEM_SIZE - 4096)
#define WLAN_SHM_BASE(x)				(AIPC_WLAN_SHM_BASE + (x * WLAN_IPC_MEM_SIZE))

#define AIPC_WLAN_OWN_BY_MASTER         0
#define AIPC_WLAN_OWN_BY_SLAVE          1

//Control
typedef struct {
	char buf[ WLAN_BUFFER_SIZE ];
} aipc_wlan_data_t;

//Control
typedef struct {
	char reserved[ WLAN_IPC_MEM_SIZE - sizeof(aipc_wlan_data_t) - 4];
} aipc_reserved_t;

typedef struct {
	volatile unsigned char   	lock_m;
	volatile unsigned char		lock_s;
	volatile unsigned short		own;
	aipc_wlan_data_t      		data;
	aipc_reserved_t				reserved;
} aipc_wlan_shm_t;

#define WLAN_SHM(x)						(*(aipc_wlan_shm_t*)(WLAN_SHM_BASE(x)))

#endif
