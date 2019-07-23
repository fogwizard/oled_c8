#include "cw2015.h"

typedef unsigned char uint8_t;

unsigned char temp_data[SIZE_BATINFO] = {0};//read data

//battery curve value of ÈýÐÇ18650/2600mah/4s4p 
const unsigned char cw_bat_config_info[SIZE_BATINFO] = {
0x14,0xA6,0x70,0x6C,0x6C,0x67,0x64,0x62,
0x60,0x5C,0x59,0x4E,0x50,0x42,0x2E,0x26,
0x25,0x24,0x27,0x2E,0x41,0x5A,0x6D,0x4C,
0x39,0xD7,0x06,0x66,0x00,0x01,0x05,0x34,
0x61,0x76,0x79,0x7D,0x42,0x1E,0xA4,0x00,
0x27,0x5A,0x52,0x87,0x8F,0x91,0x94,0x52,
0x82,0x8C,0x92,0x96,0x2B,0x59,0x6E,0xCB,
0x2F,0x7D,0x72,0xA5,0xB5,0xC1,0x94,0x99
};

void delay_t(int n)
{
	while(n--);
}

uint8_t write_data(uint8_t addr, const uint8_t *pdata, int length)
{
	return 0;
}

uint8_t read_data(uint8_t addr, uint8_t *pdata, int length)
{
	return 0;
}

unsigned char write_cw(unsigned char reg_addr, const unsigned char *pdata, unsigned char data_length)
{
	unsigned char i;
	unsigned char ret;	
	for(i =0; i <= TRY_COUNT; i++)//try TRY_COUNT times if fail
		{	
			ret = write_data(reg_addr, pdata, data_length);
			if(!ret)
				break;
				else if(TRY_COUNT == i)
					return ret;		
		}		
	return 0;
}

unsigned char read_cw(unsigned char reg_addr, unsigned char *pdata, unsigned char data_length)
{
	unsigned char i;
	unsigned char ret;	
	for(i =0; i <= TRY_COUNT; i++)//try TRY_COUNT times if fail
		{	
			ret = read_data(reg_addr, pdata, data_length);
			if(!ret)
				break;
				else if(TRY_COUNT == i)
					return ret;		
		}		
	return 0;
}


//check bat. infomation
unsigned char check_bat_info(void)
{
	unsigned char i;
	const unsigned char *wdata,*rdata;
	rdata = temp_data;
	wdata = cw_bat_config_info;
	for(i = 0; i < SIZE_BATINFO; i++)
		{
			if(*rdata != *wdata)
				return 1;
			rdata++;
			wdata++;
		}
	return 0;
}

//cw2015 initialize							
unsigned char cw_init(void)
{	
	unsigned char ret;
	unsigned char reg_val;
					
	//check sleep status
	ret = read_cw(REG_MODE, &reg_val, 1);
	if(ret)
		return ret;		
	//cw2015 wake up	
	if((reg_val & MODE_SLEEP_MASK) == MODE_SLEEP)
		{	
			reg_val = MODE_NORMAL;	
			ret = write_cw(REG_MODE, &reg_val, 1);
			if(ret)
				return ret;	
		}
	//check config updata flag
	ret = read_cw(REG_CONFIG, &reg_val, 1);
	if(ret)
		return ret;			
	if(!(reg_val & CONFIG_UPDATE_FLG))	
		{			
			//write bat. info.
			ret = write_cw(REG_BATINFO, cw_bat_config_info, SIZE_BATINFO);
			if(ret)
				return ret;
		}
		else
			{	
				//read bat. infomation		
				ret = read_cw(REG_BATINFO, temp_data, SIZE_BATINFO);
				if(ret)
					return ret;		
					
				//compare
				ret = check_bat_info();
				if(ret)
					{
						//write bat. info.
						ret = write_cw(REG_BATINFO, cw_bat_config_info, SIZE_BATINFO);
						if(ret)
							return ret;								
					}
							
			}
	//read bat. infomation		
	ret = read_cw(REG_BATINFO, temp_data, SIZE_BATINFO);
	if(ret)
		return ret;		
					
	//compare
	ret = check_bat_info();
	if(ret)
		return ret;	
											
	//set update flag	
	reg_val = 0;
	reg_val |= CONFIG_UPDATE_FLG;				
	ret = write_cw(REG_CONFIG, &reg_val,1);
	if(ret)
		return ret;
									
	//restart cw2015
	reg_val = MODE_RESTART;
	ret = write_cw(REG_MODE, &reg_val, 1);
	if(ret)
		return ret;					
		
	delay_t(30);//delay 100us
	

	//reset(wake up) cw2015	
	reg_val = MODE_NORMAL;
	ret = write_cw(REG_MODE, &reg_val,1);
	if(ret)
		return ret;
								
	return 0;
					
}
