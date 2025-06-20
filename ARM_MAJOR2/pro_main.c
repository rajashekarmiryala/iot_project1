#include<lpc21xx.h>
#include "types.h"
#include "delay.h"
#include "defines.h"
#include "keypad.h"
#include "uart0.h"
#include "i2c.h"
#include "esp01.h"
#include "lcd.h"
#include "i2c_defines.h"
#include "i2c_eeprom.h"
#include "extint.h"
#include "dht11.h"
#include "rtc.h"
#define BUZZER 5 //p0.5
u8 rt,rsp;
extern u8 i_flag;
//u8 humidity_integer, humidity_decimal, temp_integer, temp_decimal, checksum; 
u8 temp,k=0,min;

void Buzzer(void)
{
	u32 j;
	for(j=0;j<10;j++)
	{
		IOSET0=1<<BUZZER;
		delay_s(1);
		IOCLR0=1<<BUZZER;
		delay_ms(500);
	}
}

main()
{
	u8 humidity_integer, humidity_decimal, temp_integer, temp_decimal, checksum;
	SETBIT(IODIR0,BUZZER);
	InitUART0();
	LCD_Init();
	init_i2c();
	initkpm();
	Enable_EINT0();
	RTC_Init();
	esp01_connectAP();
	i2c_eeprom_write(0x50,0x00,45);
	rt=i2c_eeprom_read(0x50,0x00);
	Write_CMD_LCD(0x01);
	delay_ms(2000);
	RTCSetTime(8,23,0);
	min=MIN;
	while(1)
	{
		k=0;
		do
		{
			delay_ms(2000);
			dht11_request();
			dht11_response();
			humidity_integer = dht11_data();
			humidity_decimal = dht11_data();
			temp_integer = dht11_data();
			temp_decimal = dht11_data();
			checksum = dht11_data();
			if( (humidity_integer + humidity_decimal + temp_integer + temp_decimal) != checksum )
			{
				Write_str_LCD("Checksum Error\r\n"); 
			}
			else
			{ 
				Write_CMD_LCD(0x80);
				Write_str_LCD("H : ");
				Write_int_LCD(humidity_integer);
				Write_DAT_LCD('.');
				Write_int_LCD(humidity_decimal);
				Write_str_LCD(" % RH ");
				Write_CMD_LCD(0xC0);
				Write_str_LCD("T: ");
				Write_int_LCD(temp_integer);
				Write_DAT_LCD('.');
				Write_int_LCD(temp_decimal);
				Write_DAT_LCD(223);
				Write_str_LCD("C");
				Write_CMD_LCD(0xC0+10);
				Write_str_LCD("SP: ");
				Write_int_LCD(rt);
				Write_CMD_LCD(0xD4);
				Write_str_LCD("Checksum : ");
				Write_int_LCD(checksum);
				delay_ms(2000);
				if(min==MIN)
				{
					//esp01_sendToThingspeak(0,3);
					espo1_ReadfromThingspeak();
					//extract_sp(buff);
					if(rt!=rsp)
					{
						i2c_eeprom_write(0x50,0x00,rsp);
						rt=i2c_eeprom_read(0x50,0x00);
						Write_CMD_LCD(0xC0+10);
						Write_str_LCD("SP: ");
						Write_int_LCD(rt);
						//min=MIN;					
					}
					min=min+3;
				}
				if(MIN==59)
					min=0;
				if(k==0)
				{
					if(temp_integer>rt)
					{
						Buzzer();
						esp01_sendToThingspeak(temp_integer,1);
						delay_ms(1000);
						esp01_sendToThingspeak(humidity_integer,2);
						k++;
					}
				}
				if((k>=1)&&(k<=30))
				{
					k++;
				}
				else
					k=0;

			}
		}while(i_flag==0);
		Write_CMD_LCD(0x01);
		Write_str_LCD("Enter the setpoint");
		temp=Readnum();
		i2c_eeprom_write(0x50,0x00,temp);
		rt=i2c_eeprom_read(0x50,0x00);
		Write_CMD_LCD(0x01);
		i_flag=0;
	}	
}
