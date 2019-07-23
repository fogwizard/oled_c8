/* --------------------------------
program:stm8_uart1
revise date:2018.3.22
function: uart1 subroutine
 ----------------------------------*/
#include "x.h"
#include "stm8s003f3.h"

//set baud rate
void uart1_baud(unsigned int sys_clk, unsigned int baud)//sys_clk = Mhz
{
		unsigned int uart_temp;
		uart_temp = sys_clk*1000000/baud;
		UART1_BRR2 = (unsigned char)(((uart_temp & 0xf000) >> 8) | (uart_temp & 0x000f));
		UART1_BRR1 = (unsigned char)((uart_temp & 0x0ff0) >> 4);
}

void uart1_init(void)
{
		UART1_CR2 = 0;//disable uart transmit and receive
		UART1_CR1 = 0;//byet=8 bit
		UART1_CR3 = 0;//1 stop bit
		uart1_baud(4, 9600);//sys_clc=4M; baud=9600;
		UART1_CR2 = 0x2c;// enable transmit/receive/reiceive interrupt
}

//send a byte
unsigned char uart1_sendByte(unsigned char data)
{
		UART1_CR2 &= 0xfe;//transmit disenable
		UART1_DR = data;
		UART1_CR2 |= 0x08;//transmit enable
		{
				unsigned int i = 1000;
				while(((UART1_SR & 0xc0)!= 0xc0) && (i))//TXE=1 & TC=1
						{
								i--;
								clrwdt();
						}
				if(0 == i)
						return 1;
		}
		return 0;
}


