/* --------------------------------
program:uem
revise date:2018.3.23
function: 
.port initialize
.tim4 interrupt
.cw2015 initialize
.cw2015 read
.uart
.under volt.
 ----------------------------------*/
 
#include "x.h"
#include "stm8s003f3.h"
#include "stm8s_i2c.h"
#include "uart1.h"

volatile Bytebits Bbituse1;
#define bituse1 Bbituse1.Byte
#define cap_read_command 	Bbituse1.Bits.bit0

//flowing 4 bits can't be changed.
#define cw_read_tag 			Bbituse1.Bits.bit3
#define cw_init_tag			 	Bbituse1.Bits.bit4
#define data_valid_tag 		Bbituse1.Bits.bit5
#define cw_read_suc				Bbituse1.Bits.bit6

#define rcf_tag						Bbituse1.Bits.bit7

volatile Bytebits Bbituse2;
#define bituse2 Bbituse2.Byte
#define uart_tx_command 	Bbituse2.Bits.bit0
#define rx_head_tag 			Bbituse2.Bits.bit1
#define ack_tag						Bbituse2.Bits.bit2//1=acknowleged

#define vb_low_tag 		Bbituse2.Bits.bit4
#define bat_full_tag 	Bbituse2.Bits.bit5

//following 2 bits are related to trasmit data can't be changed.  
#define cc_tag				Bbituse2.Bits.bit6//solar panel connection tag
#define charge_tag		Bbituse2.Bits.bit7

volatile Bytebits Bbituse3;
#define bituse3 Bbituse3.Byte
#define c_0 Bbituse3.Bits.bit0

#define sleep_tag	 Bbituse3.Bits.bit7

unsigned int intCount = 0;

unsigned char rsoc = 0;
unsigned char vb_read[2] = {0};
unsigned int vb = 0;


unsigned char rcf_count =0;//cap. read fail count
unsigned char ird_count = 0;//initial read data count
unsigned int t_cr = 0;//cw2015 read time
unsigned char t_sd = 0;//sleep delay time

void gpio_init(void)
{
	//pa
	pa_odr=0;
	pa_idr=0;
	pa_ddr=0xfb;//1=out,0=in	
	pa_cr1=0x04;
	pa_cr2=0x04;
	EXTI_CR1 |= 0x02;//trigger by trailing edge
	
	//pb
	pb_odr =0;
	pb_idr =0;
	pb_ddr =0xcf;
	pb_cr1=0;
	pb_cr2=0;
	
	//pc
	pc_odr =0;
	pc_idr =0;
	pc_ddr =0xff;
	pc_cr1 =0x68;
	pc_cr2 =0;		
	
	//pd
	pd_odr = 0;
	pd_idr = 0;
	pd_ddr = 0xb1;
	pd_cr1 = 0x3c;
	pd_cr2 = 0x24;
	EXTI_CR1 |= 0xc0;//trigger by trailing and rising edge
}
void clk_init(void)
{
		//set fmaster=4M, fcpu=fmaster
		CLK_CKDIVR = 0x10;
		//close peripherals
		CLK_PCKENR1 &= 0x1d;//0b00010101 
		CLK_PCKENR2 =0;
}
void clrwdt(void)
{
		unsigned char counter_value = 0;
		counter_value = WWDG_CR & 0x7f;
		if(counter_value < WINDOW_VALUE)
				{
						WWDG_CR = COUNTER_INIT_VALUE;
				}
}
void wwdg_init(void)
{
		WWDG_WR = WINDOW_VALUE;
		WWDG_CR = COUNTER_INIT_VALUE;
		WWDG_CR |= 0x80;//ÆôÓÃWWDG
		//clrwdt();
}
void tim4_init(void)
{
	TIM4_PSCR=0x07;//perscale=2^7
	TIM4_ARR =0xe5;//counter overflow threshold
	TIM4_CNTR=0x00;//counter initialize
	TIM4_IER =0x01;//tim4 interupt enable
	tim4_cen =0;//time4 count disable
}


void var_init(void)
{
		bituse1 = 0;
		bituse2 = 0;
		bituse3 = 0;
		led_op = LED_OFF;
		led_chrg = LED_OFF;
		dce = DISABLE;
		shdn = DISABLE;
}

void init(void)
{
		clk_init();			
		wwdg_init();				
		gpio_init();
		tim4_init();
		i2c_init();
		uart1_init();
		var_init();
}

//bat. status check
void bat_status(void)
{
		//undervoltage check
		if(vb < VB_LOW)
				vb_low_tag = 1;
		if(vb >= VB_LOW_R)
				vb_low_tag = 0;
		//solar panel connect check
		if(0 == cc)
				cc_tag = 1;
				else cc_tag = 0;
		//charge check
		if(chrg != down)
				charge_tag = 1;				
				else charge_tag = 0;
		//bat. full judge		
		if(charge_tag)
				{
						if(chrg && (0 == down))
								bat_full_tag = 1;
				}
		if(0== (chrg && (0 == down)))
				bat_full_tag = 0;
}
//bat. operate
void bat_op(void)
{
		//dc. output
		if(!vb_low_tag)
				dce = 1;
				else dce = 0;
		//charge indicator
		if(charge_tag)
				{
						if(bat_full_tag)
								led_chrg = LED_ON;
								else led_chrg = c_0;
				}
				else led_chrg = LED_OFF;

}

unsigned char read_cw2015(void)
{
		unsigned char ret = 0;										
		ret = read_data(RSOC, &rsoc, 1);
		if(ret)
				return 1;			
		ret = read_data(BV_H, vb_read, 2);
		if(ret)
				return 1;	
		vb = (((unsigned int)vb_read[0] <<8) | vb_read[1]);		
		return 0;
}
//checksum calculate
//checksum of tx_data
unsigned char tx_checksum(void)
{
		unsigned char i;
		unsigned int bat_sum = 0;
		for(i = 1; i < (tx_data[1] + 2); i++)
				{
						bat_sum += tx_data[i];
						if(bat_sum > 0xff)
								{
										bat_sum = ~bat_sum;
										bat_sum += 1;
								}
						bat_sum &= 0xff;
				}
		return (unsigned char)bat_sum;
}
//checksum of rx_data
unsigned char rx_buf_checksum(void)
{
		unsigned char i;
		unsigned int bat_sum = 0;
		for(i = 1; i < (rx_buf[1] + 2); i++)
				{
						bat_sum += rx_buf[i];
						if(bat_sum > 0xff)
								{
										bat_sum = ~bat_sum;
										bat_sum += 1;
								}
						bat_sum &= 0xff;
				}
		return (unsigned char)bat_sum;
}

//set tx_data
void setSendData(void)
{
		unsigned long int vb_dec = 0;		
		vb_dec = ((unsigned long)vb * 305)/1000;//cell volt. value in mv
		//set vb value
		tx_data[4] = (unsigned char)vb_dec;
		tx_data[3] = (unsigned char)((vb_dec >> 8) & 0x1f);	
		//set bat. status
		tx_data[3] |= (bituse2 & 0xc0); 		
		//set rsoc value
		tx_data[5] = rsoc;
		//set checksum	
		tx_data[6] = (unsigned char)tx_checksum();

}

@far @interrupt void tim4_isr(void)
{
		TIM4_SR = 0;//claer update flag
		intCount++;		
		//timer 1s 		
		if(0 == (intCount & (ONE_SECOND_COUNT_MASK)))
		{
				//sleep delay timer
				if((!charge_tag) && (!sleep_tag))
						{		
								if((bituse1 & 0x78) == 0x78)
										{
												t_sd++;
												if(t_sd >= TIME_SD)
														sleep_tag = 1;
										}
						}

		}//end 1s timer

		//read cw2015 command
		if((bituse1 & 0xf0) != 0x70)
				t_cr = CAPF_READ_TIME_MASK;
				else t_cr = CAP_READ_TIME_MASK;
		if(0 == (intCount & (t_cr)))
				cap_read_command = 1;				
		//led_flicker
		c_0=LED_OFF;
		if((intCount & LED_FLICKER_COUNT_MASK)<LED_FLICKER_COUNT_END)
				c_0 = LED_ON;								
}

/*--------------receive data from host
input:/
output: ack_tag//reset ack_tag if receive success 
        rx_data[4]//received data;
------------------------------------------*/
@far @interrupt void uart1_receive(void)
{
		static unsigned char rx_count=0;
		unsigned char rx_temp;
		led_op = LED_ON;
		if(UART1_SR & 0x08)//error due to overload
				{
						rx_temp = UART1_SR;
						rx_temp = UART1_DR;
				}
		if(UART1_SR & 0x20)
				rx_temp = UART1_DR;
		//check head data		
		if(0xaa == UART1_DR)
				{
						rx_count = 0;
						rx_head_tag = 1;
				}		 
		if(rx_head_tag)
				{
						rx_buf[rx_count] = UART1_DR;
						rx_count++;
						//check host require
						if(2 == rx_count)
								{		
										if(!(0x03 == rx_buf[1]))
												rx_head_tag = 0;		
								}
								else if(3 == rx_count)
										{		
												if(!(0x01 == rx_buf[2]))
														rx_head_tag = 0;		
										}						
								else if(6 == rx_count)
										{
												if(rx_buf[5] == rx_buf_checksum())
														{
																unsigned char i = 0;
																ack_tag = 0;//ack_tag reset
																//receive host command
																for(i; i < 6; i++)
																		rx_data[i] = rx_buf[i];
																//led_op = LED_OFF;	
														}	
														rx_head_tag = 0;	
										}
				}
		led_op = LED_OFF;	
		{
				//sleep tag reset
				t_sd = 0;
				sleep_tag = 0;					
				cap_read_command = 1;
		}	
}
//
@far @interrupt void porta_isr(void)
{
		//sleep tag reset
		t_sd = 0;
		sleep_tag = 0;
		cap_read_command = 1;
		//return;
}

@far @interrupt void portd_isr(void)
{
		//sleep tag reset
		t_sd = 0;
		sleep_tag = 0;
		cap_read_command = 1;
		//return;
}

main()
{
		init();
		_asm("rim");//interrupt enable
		tim4_cen =1;
		while (1)
		{
				clrwdt();
				_asm("nop");

				bat_status();
				bat_op();
				//cw2015 initialize
				if(!cw_init_tag && cap_read_command)
						{
								cap_read_command = 0;
								i2c_init();								
								led_op = LED_ON;
								{
										unsigned char ret;
										ret = cw_init();			
										if(!ret)
												{
														cw_init_tag = 1;			
														led_op =LED_OFF;
												}
								}
								
						}
						
				if(cw_init_tag)
						{
								//get data from cw2015
								if(cap_read_command)
										{
												unsigned char ret =255;
												cap_read_command = 0;
												cw_read_tag = 0;
												led_op = LED_ON;
												ret = read_cw2015();
												if(!ret)
														{
																led_op = LED_OFF;//read indication
																cw_read_tag = 1;
																if(!data_valid_tag)
																		{
																				ird_count++;
																				if(ird_count > 3)
																						data_valid_tag = 1;
																		}	
																//read cap. fail tag reset
																{
																		cw_read_suc = 1;
																		rcf_count = 0;
																		rcf_tag = 0;
																}
														}								
														else if(!rcf_tag)
																{
																		cw_read_suc = 0;
																		ird_count = 0;
																		i2c_init();
																		rcf_count++;
																		if(rcf_count >= RCF_COUNT)
																				rcf_tag = 1;
																						
																}				
												//read cap. fail
												if(rcf_tag)
														{
																cw_init_tag = 0;
																data_valid_tag = 0;
																rsoc = 0;
																vb = 0;											
														}										
												//_asm("nop");								
										}
								
								//send data to host
								if(!ack_tag)
										{
												unsigned char *pdata = tx_data;
												unsigned char i = 0;
												shdn = ENABLE;
												setSendData();
												led_op = LED_ON;
												for(i; i < (tx_data[1]+3); i++)
														{
																unsigned char ret = 0;
																ret = uart1_sendByte(*pdata);	
																if(ret)
																		break;
																pdata++;
																clrwdt();
														}
												if((tx_data[1]+3) == i)
														{
																led_op = LED_OFF;
																ack_tag = 1;
																shdn = DISABLE;
														}
										}		
										
				}//end if(cw_init_tag)
				//_asm("nop");
				//sleep
				if(sleep_tag)
						{
								tim4_cen = 0;
								i2c_cr1_pe = 0;//i2c module disable
								UART1_CR1 |= 0x20;//uart disable
								pd_cr2 |= 0x40;//enable uart_rx pin exit interrupt
								CLK_PCKENR1 = 0;//close fmaster to peripherals
								
								//led_op = LED_OFF;
								//sleep tag reset
								t_sd = 0;
								sleep_tag = 0;									
								_asm("halt");
								
								pd_cr2 &= 0xbf;//disable uart_rx pin exit interrupt
								//open fmaster to peripherals
								CLK_PCKENR1 |= 0x1d;//0b00011101 
								tim4_cen = 1;								
								i2c_cr1_pe = 1;//i2c module enable
								UART1_CR1 &= 0xdf;//uart enable
						}
					
		}//end while(1)
}