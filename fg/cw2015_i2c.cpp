#include "stdint.h"
#include "delay.h"
#include "sys.h"

/* PB9 sda */
#define sda_pin_set()    GPIO_SetBits(GPIOB,GPIO_Pin_9)
#define sda_pin_reset()  GPIO_ResetBits(GPIOB,GPIO_Pin_9)
#define sda_pin_read()   GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9)

/* PB8 scl */
#define scl_pin_set()    GPIO_SetBits(GPIOB,GPIO_Pin_8)
#define scl_pin_reset()  GPIO_ResetBits(GPIOB,GPIO_Pin_8)


static int delay_times = 1000;

static void delay_us_local(uint64_t us)
{
    for (int i = 0; i < us; i++) {
        //		for(int j = 0; j < 2;j++);
    }
}

void begin(uint32_t speed)
{
    speed = speed;
}

void init_i2c_gpio(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};

    begin(2000);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //使能B端口时钟
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB,GPIO_Pin_8|GPIO_Pin_9);
}

void start()
{
    sda_pin_set();
    scl_pin_set();
    delay_us_local(delay_times);
    sda_pin_reset();
    delay_us_local(delay_times);
    scl_pin_reset();
}

void stop()
{
    scl_pin_reset();
    sda_pin_reset();
    delay_us_local(delay_times);
    scl_pin_set();
    delay_us_local(delay_times);
    sda_pin_set();
}
int8_t wait_ack()
{
    uint8_t cErrTime = 100;
    sda_pin_set();  //DEBUG
    scl_pin_set();
    delay_us_local(delay_times);
    while (sda_pin_read()) {
        cErrTime--;
        delay_us_local(delay_times);
        if (cErrTime == 0) {
            stop();
            return -1;

        }

    }
    scl_pin_reset();
    delay_us_local(delay_times);
    return 0;
}
int8_t send_ack()
{
    sda_pin_reset();
    delay_us_local(delay_times);
    scl_pin_set();
    delay_us_local(delay_times);
    scl_pin_reset();
    delay_us_local(delay_times);

    return 0;
}
int8_t send_no_ack()
{
    sda_pin_set();
    delay_us_local(delay_times);
    scl_pin_set();
    delay_us_local(delay_times);
    scl_pin_reset();
    delay_us_local(delay_times);
    return 0;
}
int8_t send_byte(uint8_t byte)
{
    int8_t ret = 0;

    uint8_t ii = 8;
    sda_pin_set();
    scl_pin_reset();

    while ( ii-- ) {

        if(byte & 0x80) {
            sda_pin_set();
        } else {
            sda_pin_reset();
        }
        delay_us_local(delay_times);
        byte <<= 1;

        scl_pin_set();
        delay_us_local(delay_times);
        scl_pin_reset();
        delay_us_local(delay_times);
    }
    ret = wait_ack();
    return ret;
}
int8_t  send_7bits_address(uint8_t slave_address)
{
    int8_t ret = 0;
    send_byte(slave_address);
    return ret;
}

uint8_t receive_byte(void)
{
    uint8_t i = 8;
    uint8_t byte = 0;
    sda_pin_set();

    while (i--) {
        scl_pin_set();
        byte += byte;
        delay_us_local(delay_times);
        byte |= sda_pin_read();
        scl_pin_reset();
        delay_us_local(delay_times);
    }
    delay_us_local(delay_times);

    return byte;
}
int8_t write_byte(uint8_t slave_address, uint8_t reg_address, uint8_t data)
{
    int8_t ret = 0;
    start();

    if (send_7bits_address(slave_address) == -1)
        ret = -1;

    if (send_byte(reg_address) == -1)
        ret = -2;

    if (send_byte(data) == -1)
        ret = -3;

    stop();

    delay_us_local(10);
    return ret;
}
int8_t write_bytes(uint8_t slave_address, uint8_t reg_address, const uint8_t *data, uint16_t num_to_write)
{
    int8_t ret = 0;
    start();

    if (send_7bits_address(slave_address) == -1)
        ret = -1;

    if (send_byte(reg_address) == -1)
        ret = -2;

    while (num_to_write--) {
        if (-1 == send_byte(*data++)) {
            ret = -3;
            break;
        }
    }

    stop();

    delay_us_local(10);
    return ret;
}
int8_t  read_byte(uint8_t slave_address, uint8_t reg_address, uint8_t *data)
{

    int8_t ret = 0;
    start();

    if (send_7bits_address(slave_address) == -1)
        ret = -1;


    if (send_byte(reg_address) == -1)
        ret = -2;

    start();

    if (send_byte(slave_address + 1) == -1)
        ret = -3;

    *data = receive_byte();
    send_no_ack();
    stop();

    return ret;
}
int8_t  read_bytes(uint8_t slave_address, uint8_t reg_address, uint8_t *data, uint16_t num_to_read)
{
    int8_t ret = 0;
    int i = 0;

    start();

    if (send_7bits_address(slave_address) == -1)
        ret = -1;

    if (send_byte(reg_address) == -1)
        ret = -2;

    start();
    if (send_7bits_address(slave_address + 1) == -1)
        ret = -3;

    while (num_to_read) {
        *data++ = receive_byte();
        num_to_read--;
        i++;
        if (num_to_read == 0) {
            send_no_ack();
            stop();
            ret = 0;
        } else
            send_ack();
    }

    return ret;
}
int8_t wait_dev_busy(uint8_t slave_address)
{
    int8_t ret;
    uint8_t i = 0;
    do {
        start();
        ret = send_7bits_address(slave_address);
        send_ack();
        send_byte(slave_address);
        stop();
        if (i++ == 100) {
            return -1;
        }
    } while (ret != 0);
    return 0;
}
