/*
 * esp32_keypad.c
 *
 *  Created on: 25 kwi 2026
 *      Author: Rinkazidoshi
 */

#include "esp32_keypad.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "hal/uart_types.h"
#include "string.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "rom/ets_sys.h"


//your const definitione

//END of your const definition


//static functions of library - do not change under any circumstance!!
static int buffer_append(char value){
	static uint64_t last_detection=0;
	if(value=='#'){
		if(buffer_length>0){
			buffer[buffer_length]='\0';
			keypad_code_action(buffer);	
			buffer_length=0;
			}
	}else if(value=='*'||buffer_length>=MAX_NUM_OF_CHARACTERS){
		buffer_length=0;
	}else if((esp_timer_get_time()/1000)-last_detection>180){
		buffer[buffer_length++]=value;
		last_detection=esp_timer_get_time()/1000;
	}
	return 1;
}
static char key_detection(int row){
		gpio_intr_disable(row);
		for(int i=0;i<NUM_OF_COLS;i++){
			gpio_set_level(cols[i],1);
			if(gpio_get_level(row)==1){
				gpio_set_level(cols[i],0);
				gpio_intr_enable(row);
				return keys[row-rows[0]][i];
			}else{ 
				gpio_set_level(cols[i],0);
			}
		}
		gpio_intr_enable(row);
		return 0;
}
static void row1_IT(void* args){
			buffer_append(key_detection(ROW_1));
}
static void row2_IT(void* args){
			buffer_append(key_detection(ROW_2));
}
static void row3_IT(void* args){
			buffer_append(key_detection(ROW_3));
}
static void row4_IT(void* args){
			buffer_append(key_detection(ROW_4));
}

//library functions
int keypad_init(){
	gpio_install_isr_service(0);
	gpio_config_t io_config={
		.intr_type=GPIO_INTR_DISABLE,
		.mode=GPIO_MODE_INPUT_OUTPUT,
		.pin_bit_mask=((1<<COL_1)|(1<<COL_2)|(1<<COL_3)|(1<<COL_4)),
		.pull_down_en=0,
		.pull_up_en=0,
	};
	gpio_config(&io_config);
	gpio_set_level(COL_1,0);
	gpio_set_level(COL_2,0);
	gpio_set_level(COL_3,0);
	gpio_set_level(COL_4,0);
	
	io_config.intr_type=GPIO_INTR_NEGEDGE,
	io_config.mode=GPIO_MODE_INPUT,
	io_config.pin_bit_mask=((1<<ROW_1)|(1<<ROW_2)|(1<<ROW_3)|(1<<ROW_4)),
	io_config.pull_down_en=0,
	io_config.pull_up_en=1,
	gpio_config(&io_config);
	gpio_isr_handler_add(ROW_1, row1_IT, NULL);
	gpio_isr_handler_add(ROW_2, row2_IT, NULL);
	gpio_isr_handler_add(ROW_3, row3_IT, NULL);
	gpio_isr_handler_add(ROW_4, row4_IT, NULL);
	return 1;
}

int keypad_error(){
	printf("Keypad error\n");
	return 0;
}

void keypad_code_action(char *code){
	if(strcmp(code,passwd)==0){
		if(gpio_get_level(20)==0){
		gpio_set_level(1,0);
		gpio_set_level(2,1);
		gpio_set_level(21,0);
		}
	}
}

int keypad_set_key(const uint8_t new_key_definition, uint8_t key_col, uint8_t key_row){
	if((key_col<3&&key_col>0)&&(key_row<3&&key_row>0)){
		keys[key_row][key_col]=new_key_definition;
	}else{
		return -1;
	}
	return 1;
}

int keypad_redefine(const uint8_t new_keys_definition[4][4]){
	for(int i=0;i<NUM_OF_ROWS;i++){
		for(int j=0;j<NUM_OF_COLS;j++){
			keys[i][j]=new_keys_definition[i][j];
		}
	}
	return 1;
}



