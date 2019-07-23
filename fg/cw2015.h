/*------------------------------
program name:  cw_bat_info.h
perfect date£º2018.1.8
mcu:bs86d20a-3

function:cw2015 bat. information
-------------------------------*/ 


#ifndef _CW_BAT_INFO_H_
#define _CW_BAT_INFO_H_

#define SIZE_BATINFO 64

#define WAIT_CYCLE_COUNT 1000

#define RECEIVE 	0
#define TRANSMIT  1
//cw2015 device address
#define	READ_CW2015		0xc5
#define	WRITE_CW2015	0xc4
//date address in cw2015
#define RSOC 0x04//relative state of charge	
#define BV_L 0x03//bat. voltage
#define BV_H 0x02

//register address
#define REG_VERSION             0x0
#define REG_VCELL               0x2
#define REG_SOC                 0x4
#define REG_RRT_ALERT           0x6//rrt 
#define REG_CONFIG              0x8//config, value = low soc alert threshold
#define REG_MODE                0xA
#define REG_BATINFO             0x10//bat. information 1st register 

//bit of mode register
#define MODE_SLEEP_MASK         (0x3<<6)
#define MODE_SLEEP              (0x3<<6)//sleep mode code
#define MODE_NORMAL             (0x0<<6)//normal mode code
#define MODE_QUICK_START        (0x3<<4)
#define MODE_RESTART            (0xf<<0)

//bit of config register
#define CONFIG_UPDATE_FLG       (0x1<<1)
#define ATHD                    (0xA<<3)//ATHD = 10% ,low soc alert threshole value 

#define SIZE_BATINFO 64
#define TRY_COUNT 20

#endif




