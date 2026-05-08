/*
 * 1602a_LCD.h
 *
 *  Created on: 27 kwi 2026
 *      Author: mmazu
 */
#include "freertos/idf_additions.h"
#include "stdio.h"
#include "hal/gpio_types.h"
#include "rom/ets_sys.h"
#include "stdint.h"
#include "driver/gpio.h"
#include <string.h>
#define LCD_RS 9
#define LCD_RW 10
#define LCD_EN	11
#define LCD_D0	35
#define LCD_D1	36
#define LCD_D2	37
#define LCD_D3	38
#define LCD_D4	39
#define LCD_D5	40
#define LCD_D6	41
#define LCD_D7	42
//numbers definition
#define LCD_ClearDisplay 	0b00000001
#define LCD_ReturnHome  	0b00000010
#define LCD_EntryModeSet  	0b00000111
//LCDDefaultCMD
#define LCD_DisplayON		  0b00001000
#define LCD_ShowCursorON	  0b00000110
#define LCD_BlinkingCursorON  0b00000111
#define LCD_DisplayOFF		  0b00000000
#define LCD_ShowCursorOFF	  0b00000100
#define LCD_BlinkingCursorOFF 0b00000110

//LCD Positions Registers
#define LCD_POS0_0	0x00
#define LCD_POS0_1	0x01
#define LCD_POS0_2	0x02
#define LCD_POS0_3	0x03
#define LCD_POS0_4	0x04
#define LCD_POS0_5	0x05
#define LCD_POS0_6	0x06
#define LCD_POS0_7	0x07
#define LCD_POS0_8	0x08
#define LCD_POS0_9	0x09
#define LCD_POS0_10	0x0A
#define LCD_POS0_11	0x0B
#define LCD_POS0_12	0x0C
#define LCD_POS0_13	0x0D
#define LCD_POS0_14	0x0E
#define LCD_POS0_15	0x0F
#define LCD_POS1_0	0x40
#define LCD_POS1_1	0x41
#define LCD_POS1_2	0x42
#define LCD_POS1_3	0x43
#define LCD_POS1_4	0x44
#define LCD_POS1_5	0x45
#define LCD_POS1_6	0x46
#define LCD_POS1_7	0x47
#define LCD_POS1_8	0x48
#define LCD_POS1_9	0x49
#define LCD_POS1_10	0x4A
#define LCD_POS1_11	0x4B
#define LCD_POS1_12	0x4C
#define LCD_POS1_13	0x4D
#define LCD_POS1_14	0x4E
#define LCD_POS1_15	0x4F

#define LCD_UPPER_LINE 0
#define LCD_LOWER_LINE 1

static const uint8_t upper_line[16]={LCD_POS0_0,LCD_POS0_1,LCD_POS0_2,LCD_POS0_3,
LCD_POS0_4,LCD_POS0_5,LCD_POS0_6,LCD_POS0_7,LCD_POS0_8,LCD_POS0_9,
LCD_POS0_10,LCD_POS0_11,LCD_POS0_12,LCD_POS0_13,LCD_POS0_14,LCD_POS0_15};

static const uint8_t lower_line[16]={LCD_POS1_0,LCD_POS1_1,LCD_POS1_2,LCD_POS1_3,
LCD_POS1_4,LCD_POS1_5,LCD_POS1_6,LCD_POS1_7,LCD_POS1_8,LCD_POS1_9,
LCD_POS1_10,LCD_POS1_11,LCD_POS1_12,LCD_POS1_13,LCD_POS1_14,LCD_POS1_15};

static const uint8_t data_pins[8]={LCD_D0,LCD_D1,LCD_D2,
LCD_D3,LCD_D4,LCD_D5,LCD_D6,LCD_D7};

static void enPin_pulse(){
	ets_delay_us(3);
	gpio_set_level(LCD_EN,1);
	ets_delay_us(3);
	gpio_set_level(LCD_EN,0);
	ets_delay_us(100);
}
static void byte_to_dataPins(uint8_t data){
	for(int i=0;i<8;i++){
		gpio_set_level(data_pins[i],(data>>i)&1);
	}
}
static void lcd_send_cmd(uint8_t cmd){
	byte_to_dataPins(cmd);
	gpio_set_level(LCD_RS,0);
	gpio_set_level(LCD_RW,0);
	enPin_pulse();
}
static void lcd_send_data(uint8_t data){
	byte_to_dataPins(data);
	gpio_set_level(LCD_RS,1);
	gpio_set_level(LCD_RW,0);
	enPin_pulse();
}
static int lcd_setCursour(uint8_t pos,uint8_t line){
	if(pos<16&&line<2){
		if(line==0) lcd_send_cmd(0x80|upper_line[pos]);
		else lcd_send_cmd(0x80|lower_line[pos]);
		return 1;
	}
	else return -1;
}
int lcd_init(){
	gpio_config_t pin_config={
		.intr_type=GPIO_INTR_DISABLE,
		.mode=GPIO_MODE_OUTPUT,
		.pin_bit_mask=((1ULL<<LCD_RS)|(1ULL<<LCD_RW)|(1ULL<<LCD_EN)|(1ULL<<LCD_D0)|(1ULL<<LCD_D1)
		|(1ULL<<LCD_D2)|(1ULL<<LCD_D3)|(1ULL<<LCD_D4)|(1ULL<<LCD_D5)|(1ULL<<LCD_D6)|(1ULL<<LCD_D7)),
		.pull_down_en=0,
		.pull_up_en=0,
	};
	gpio_config(&pin_config);
	ets_delay_us(15000);  // Wait >15 ms after power ON
	lcd_send_cmd(0x38);   // Function set: 8-bit, 2 lines, 5x8 font
	ets_delay_us(4500);   // Wait >4.1 ms
	lcd_send_cmd(0x38);   // Repeat
	ets_delay_us(200);    // Wait >100 µs
	lcd_send_cmd(0x38);   // Repeat again
	lcd_send_cmd(0x08);   // Display OFF
	lcd_send_cmd(0x01);   // Clear display
	ets_delay_us(2500);   // Clear needs >1.5 ms
	lcd_send_cmd(0x06);   // Entry mode: increment, no shift
	lcd_send_cmd(0x0C);   // Display ON, cursor OFF, blink OFF
	return 1;
}
int lcd_print(const char* text,uint8_t line,uint8_t position){
	if(strlen(text)>16) return -1;
	else if(line==0){
		if(lcd_setCursour(0, 0)!=1) return -1;
	}
	else if(line==1){
			if(lcd_setCursour(0, 1)!=1) return -1;
	}
		for(int i=0;i<strlen(text);i++){
			lcd_send_data(text[i]);
		}
	return 1;
	
}
int lcd_send_line(const char* text,uint8_t line){
	if(lcd_print((const char*)text,line,0)==1)	return 1;
	else return -1;
}