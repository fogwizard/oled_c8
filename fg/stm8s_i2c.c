/* --------------------------------
program:stm8s_i2c
revise date:2018.3.22
function: i2c subroutine
 ----------------------------------*/
#include "x.h"
#include "stm8s003f3.h"
#include "stm8s_i2c.h"

/*--------------delay_t
t=1//10us
t=10//40us
t=20//80us
t=30//120us
t=250//1ms
t=500//1.8ms
t>=5000//18ms
-------------------*/
void delay_t(unsigned int t)
{
		if(t > 5000)
				t = 5000;
		while(t--)
				clrwdt();//clear wdt.			
}

void i2c_init(void)
{
		//stm8s i2c port
		pb_ddr &= 0xcf;//0=in
		pb_cr1 &= 0xcf;//1=float
		pb_cr2 &= 0xcf;//0=interrup disable
		//stml i2c clock
		//CLK_PCKENR1 |= (1 << 3);//open peripheral clock,open i2c1	
		
		//reset i2c
		i2c_cr2_swrst	= 1;
		delay_t(1);
		i2c_cr2_swrst	= 0;		
		//i2c register
		i2c_cr1_pe = 0;//i2c module disable
		I2C_FREQR = 4;//i2c input frequency =fmaster=4M
		I2C_CCRL = 0x14;//CCR=20
		I2C_CCRH =0;
		I2C_TRISER = 5;//TRISE=fmaster+1
		i2c_cr1_pe = 1;//i2c module enable
		i2c_cr2_ack = 1;//ack enable

}


unsigned char read_data(unsigned char reg_address, unsigned char *pdata, unsigned char data_length)
{
		unsigned int i = 0;
		//i2c start
		i = WAIT_CYCLE_COUNT;
		while(i2c_sr3_busy && (i))//check BUSY bit,wait for i2c free
				{
						i--;
						clrwdt();
				}
		if(0 == i)
				{
						i2c_cr2_stop = 1;//stop
						return 1;	
				}
		i2c_cr2_start = 1;//start
		i = WAIT_CYCLE_COUNT;
		while((0 == i2c_sr1_sb) && (i))//check SB,wait for start sent
				{
						i--;
						clrwdt();
				}
		if(0 == i)
				{
						i2c_cr2_stop = 1;//stop
						return 1;	
				}		
		delay_t(1);
		
		//sent device address
		I2C_DR = WRITE_CW2015;
		i = WAIT_CYCLE_COUNT;
		while((0 == i2c_sr1_addr) && (i))//check ADDR,wait for address sent
				{
						i--;
						clrwdt();
				}
		if(0 == i)
				{
						i2c_cr2_stop = 1;//stop
						return 1;	
				}	
		delay_t(1);
		if(i2c_sr3_tra != TRANSMIT)//read sr3 to clear addr bit
				{
						i2c_cr2_stop = 1;//stop
						return 1;	
				}	
				
		//sent rigister address of device
		I2C_DR = reg_address;
		//while(0== i2c_sr1_btf);//check BTF,wait for data sent
		i = WAIT_CYCLE_COUNT;
		while(!(i2c_sr1 &= 0x84) && (i))//check BTF and TxE
				{
						i--;
						clrwdt();
				}
		if(0 == i)
				{
						i2c_cr2_stop = 1;//stop
						return 1;	
				}		
		delay_t(1);
		
		//i2c restart 
		i2c_cr2_start = 1;//start
		i = WAIT_CYCLE_COUNT;
		while((0 == i2c_sr1_sb) && (i))//wait for start sent
				{
						i--;
						clrwdt();
				}
		if(0 == i)
				{
						i2c_cr2_stop = 1;//stop
						return 1;	
				}	
		delay_t(1);
		//sent device address
		I2C_DR = READ_CW2015;
		i = WAIT_CYCLE_COUNT;
		while((0 == i2c_sr1_addr) && (i))//wait for address sent
				{
						i--;
						clrwdt();
				}
		if(0 == i)
				{
						i2c_cr2_stop = 1;//stop
						return 1;	
				}	
		delay_t(1);
		if(i2c_sr3_tra != RECEIVE)//read sr3 to clear addr bit
				{
						i2c_cr2_stop = 1;//stop
						return 1;	
				}	
				
		//read data
		i2c_cr2_ack = 1;//ack enable		
		while(data_length)
		{
				clrwdt();
				if(1 == data_length)
						{
								i2c_cr2_ack = 0;//not ack
								i2c_cr2_stop = 1;//stop after sent the byte
						}
				i = WAIT_CYCLE_COUNT;						
				while(!(i2c_sr1_rxne) && (i))//check rxne
						{
								i--;
								clrwdt();
						}
				if(0 == i)
						{
								i2c_cr2_stop = 1;//stop
								return 1;	
						}				
				*pdata = I2C_DR;//read data and clear RxNE by hardware
				pdata++;
				data_length--;
		}	
		return 0;
}

unsigned char write_data(unsigned char reg_address, unsigned char *pdata, unsigned char data_lentgh)
{
		unsigned int i = 0;
		//i2c start
		i = WAIT_CYCLE_COUNT;
		while(i2c_sr3_busy && (i))//check BUSY bit,wait for i2c free
				{
						i--;
						clrwdt();
				}
		if(0 == i)
				{
						i2c_cr2_stop = 1;//stop
						return 1;	
				}	
		i2c_cr2_start = 1;//start
		i = WAIT_CYCLE_COUNT;
		while((0 == i2c_sr1_sb) && (i))//check SB,wait for start sent
				{
						i--;
						clrwdt();
				}
		if(0 == i)
				{
						i2c_cr2_stop = 1;//stop
						return 1;	
				}	
		delay_t(1);
		
		//sent device address
		I2C_DR = WRITE_CW2015;
		i = WAIT_CYCLE_COUNT;
		while((0 == i2c_sr1_addr) && (i))//check ADDR,wait for address sent
				{
						i--;
						clrwdt();
				}
		if(0 == i)
				{
						i2c_cr2_stop = 1;//stop
						return 1;	
				}	
		delay_t(1);
		if(i2c_sr3_tra != TRANSMIT)//read sr3 to clear addr bit
				{
						i2c_cr2_stop = 1;//stop
						return 1;	
				}	
				
		//sent rigister address of device
		I2C_DR = reg_address;
		//while(0== i2c_sr1_btf);//check BTF,wait for data sent
		i = WAIT_CYCLE_COUNT;
		while(!(i2c_sr1 &= 0x84) && (i))//check BTF and TxE
				{
						i--;
						clrwdt();
				}
		if(0 == i)
				{
						i2c_cr2_stop = 1;//stop
						return 1;	
				}	
		delay_t(1);
		{
		//sent data
		unsigned char n = 0;
		for(n = 0; n < data_lentgh; n++)
				{
						clrwdt();
						I2C_DR = *pdata;
						i = WAIT_CYCLE_COUNT;
						while(!(i2c_sr1 &= 0x84) && (i))
								{
										i--;
										clrwdt();
								}
						if(0 == i)
								{
										i2c_cr2_stop = 1;//stop
										return 1;	
								}	
						pdata++;
				}
		}
		i2c_cr2_stop = 1;//stop
		return 0;
		
}