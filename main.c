#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "soc/soc.h"
#include "string.h"
#include "tcp_client.c"
#include "1602a_LCD.c"
#include "driver/gptimer_types.h"
#include "hal/gpio_types.h"
#include "hal/rtc_io_types.h"
#include "hal/timer_types.h"
#include "hal/uart_types.h"
#include "rom/ets_sys.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/uart.h"
#include "soc/clk_tree_defs.h"
#include "esp32_keypad.c"
#include "tcp_client.h"

#define CORE_0		0
#define CORE_1		1
#define led_blue 	19
#define led_red 	1
#define led_green 	2
#define buzzer 		21
#define contactron 	20

typedef struct{
	uint8_t processorState;
	uint8_t alarmState;
}logData;

logData logData_Proc={
		.alarmState=0,	
		.processorState=0,
};
//own functions
char* timer_count_to_string(uint64_t time){
	char* str="00.000";
	int i=5;
	while(time>0){
		if(i!=2){
			char c=time%10+48;
			str[i]=c;
			time=time/10;
		}
			i--;
	}
	return str;
}
static void freeRTOS_UART_Transmit(void* arg){
	uint64_t last_transmission=0;
	const char* text1="\r\nBuffer: ";
	const char* text2="\r\nLeft size of stack: ";
	while(1){
		if(esp_timer_get_time()/1000-last_transmission>10000){
			uart_write_bytes(UART_NUM_0, text1, strlen(text1));
			uart_write_bytes(UART_NUM_0, buffer, sizeof(buffer));
			uart_write_bytes(UART_NUM_0, text2, strlen(text2));
			uint16_t stack=uxTaskGetStackHighWaterMark(NULL);
			ets_printf("%d",stack);
			last_transmission=esp_timer_get_time()/1000;
		}
	}
	vTaskDelete(NULL);
}
void freeRTOS_LCD_Transmission(void* arg);
void freeRTOS_TCP_Client_Task(void *arg);
//global variables
	gptimer_handle_t gptimer1=NULL;
	//timer configuration
	gptimer_config_t gptimer1_config={
		.clk_src=GPTIMER_CLK_SRC_DEFAULT,
		.direction=GPTIMER_COUNT_UP,
		.resolution_hz=10000000, //kHz
	};
//Interrupts Service Functions	
void contactron_IT(void* args){
	gpio_set_level(buzzer,1);
	gpio_set_level(led_red,1);
	gpio_set_level(led_green,0);
	logData_Proc.alarmState=1;
}


bool IRAM_ATTR timer_IT(gptimer_handle_t gptimer,const gptimer_alarm_event_data_t* gptimer_data,void* user_ctx){
	logData* log=(logData*)user_ctx;
	ets_printf("Stan pracy procesu: %d\nStan alarmu: %d\n",log->processorState,log->alarmState);
	return 1;
}
void app_main(void)
{
	/**/
	//ISR
	gpio_install_isr_service(0);
	//io configuration
	gpio_config_t io_config;
	io_config.intr_type=GPIO_INTR_DISABLE;
	io_config.mode=GPIO_MODE_INPUT_OUTPUT;
	io_config.pin_bit_mask=((1<<buzzer)|(1<<led_blue)|(1<<led_red)|(1<<led_green));
	io_config.pull_down_en=0;
	io_config.pull_up_en=0;
	gpio_config(&io_config);
	//io_It configuration
	io_config.intr_type=GPIO_INTR_POSEDGE;
	io_config.mode=GPIO_MODE_INPUT;
	io_config.pin_bit_mask=(1<<contactron);
	io_config.pull_down_en=0;
	io_config.pull_up_en=1;
	gpio_config(&io_config);
	//signal booting processed positively
    gpio_set_level(led_blue, 1);
	logData_Proc.processorState=1;
	esp_err_t gptimer1_init_err=gptimer_new_timer(&gptimer1_config,&gptimer1);
	if(gptimer1_init_err!=ESP_OK){
		gpio_set_level(led_blue, 0);
		logData_Proc.processorState=0;
	}
	//timer interrupt configuration
	gptimer_alarm_config_t gptimer1_it_config={
		.alarm_count=10000000,
		.flags.auto_reload_on_alarm=true,
		.reload_count=0,
	};
	gptimer_set_alarm_action(gptimer1, &gptimer1_it_config);
	//it function assignment
	gptimer_event_callbacks_t gptimer1_cb={
		.on_alarm=timer_IT,
	};
	gptimer_register_event_callbacks(gptimer1,&gptimer1_cb, &logData_Proc);
	//timer start
	gptimer_enable(gptimer1);
	gptimer_start(gptimer1);
	//it function configuration
	uint8_t buzzer_pin=buzzer;
	gpio_isr_handler_add(contactron,contactron_IT,&buzzer_pin);
	keypad_init();
	//uart configuration
	uart_config_t uart_config={
		.baud_rate=115200,
		.data_bits=UART_DATA_8_BITS,
		.parity=UART_PARITY_DISABLE,
		.stop_bits=UART_STOP_BITS_1,
		.flow_ctrl=UART_HW_FLOWCTRL_DISABLE
	};
	uart_driver_install(UART_NUM_0,2048,0,0,NULL,0);
	uart_param_config(UART_NUM_0,&uart_config);
	const uint32_t LCDT_task_stack_mem=2048,TCP_Client_task_stack_mem=8192;
	UBaseType_t lcd_transmission_priority=5,tcp_priority=1;
	TaskHandle_t lcd_transmission=NULL,tcp_client=NULL;
	lcd_init();
	const char* line_0="Left time: ";
	const char* line_1="Buffer: ";
	lcd_send_line(line_0,0);
	lcd_send_line(line_1,1);
	//xTaskCreatePinnedToCore(freeRTOS_LCD_Transmission, "LCD_Transmission", LCDT_task_stack_mem,NULL , lcd_transmission_priority, &lcd_transmission,CORE_0);
	xTaskCreatePinnedToCore(freeRTOS_TCP_Client_Task, "TCP_Connection", TCP_Client_task_stack_mem,NULL , tcp_priority, &tcp_client,CORE_0);
	//xTaskCreate(freeRTOS_UART_Transmit, "UART_Transmit", UT_task_stack_mem,NULL , uart_transmit_priority, &uart_transmit);

}
void freeRTOS_LCD_Transmission(void* arg){
	
	while(1){
		//uint64_t t=0;
		//gptimer_get_raw_count(gptimer1, &t);
		//lcd_print(timer_count_to_string(t/1000),0,11);
	}
	vTaskDelete(NULL);
}
void freeRTOS_TCP_Client_Task(void *arg){
		char host_ip[]="10.42.0.1";
		uint16_t port=8081;
		tcp_client_init(host_ip,&port);
		vTaskDelete(NULL);
}
