#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/signal.h>
#include <unistd.h>
#include "esp_rtc_time.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "freertos/portable.h"
#include "portmacro.h"
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
#include <signal.h>

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
	static char str[7]="00.000";
	int i=5;
	while(i>0){
		if(i!=2){
			char c=time%10+'0';
			str[i]=c;
			time=time/10;
		}
			i--;
	}
	return str;
}

int socket_read_with_timeout(int socket,char* buffer,int buf_len,int timeout){
	fd_set read_fds;
	struct timeval tv;
	
	FD_ZERO(&read_fds);
	FD_SET(socket,&read_fds);
	
	tv.tv_sec=timeout;
	tv.tv_usec=0;
	
	int ret=select(socket+1,&read_fds,NULL,NULL,&tv);

	if(ret<=0){
		return(-1);
	}else{
		return read(socket,buffer,buf_len);
	}

}
void freeRTOS_LCD_Transmission(void* arg);
void freeRTOS_TCP_Client_Task(void *arg);
//global variables
	gptimer_handle_t gptimer1=NULL;
	int cnt=0;
	//timer configuration
	gptimer_config_t gptimer1_config={
		.clk_src=GPTIMER_CLK_SRC_DEFAULT,
		.direction=GPTIMER_COUNT_UP,
		.resolution_hz=1000000, //1 MHz
	};
//Interrupts Service Functions
void alrm_handler(){
	cnt++;
}	
void contactron_IT(void* args){
	if(logData_Proc.alarmState==0){
		int info=0;
		gpio_set_level(led_red,1);
		gpio_set_level(led_green,0);
		xQueueSend(AlarmInfo,&info,0);
		gptimer_start(gptimer1);
		logData_Proc.alarmState=1;
		ch_passwd=0;
	}
}


bool IRAM_ATTR timer_IT(gptimer_handle_t gptimer,const gptimer_alarm_event_data_t* gptimer_data,void* user_ctx){
	logData* log=(logData*)user_ctx;
	int info=-1;
	gpio_set_level(buzzer,1);
	xQueueSend(AlarmInfo,&info,0);
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
		.flags.auto_reload_on_alarm=false,
		.reload_count=0,
	};
	gptimer_set_alarm_action(gptimer1, &gptimer1_it_config);
	//it function assignment
	gptimer_event_callbacks_t gptimer1_cb={
		.on_alarm=timer_IT,
	};
	gptimer_register_event_callbacks(gptimer1,&gptimer1_cb, &logData_Proc);
	//timer enable
	gptimer_enable(gptimer1);
	//it function configuration
	gpio_isr_handler_add(contactron,contactron_IT,NULL);
	esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ets_printf(TAG, "ESP32-S3 initializing Wi-Fi...");
	wifi_init_sta();
	keypad_init();
	AlarmInfo=xQueueCreate(1, sizeof(int));
	const uint32_t LCDT_task_stack_mem=8192,TCP_Client_task_stack_mem=8192;
	UBaseType_t lcd_transmission_priority=1,tcp_priority=1;
	xTaskCreatePinnedToCore(freeRTOS_LCD_Transmission, "LCD_Transmission", LCDT_task_stack_mem,NULL , lcd_transmission_priority, NULL,0);
	xTaskCreatePinnedToCore(freeRTOS_TCP_Client_Task, "TCP_Connection", TCP_Client_task_stack_mem,NULL , tcp_priority, NULL,CORE_0);

}
void freeRTOS_LCD_Transmission(void* arg){
	char s_pom[7];
	char lcd_buffer[11]="**********";
	uint64_t last_measure=esp_timer_get_time();
	uint64_t t=0;	
	lcd_init();
	vTaskDelay(1000/portTICK_PERIOD_MS);
	const char* line_0="Left time: ";
	const char* line_1="Buffer: ";
	lcd_print((char*)line_0,0,0);
	lcd_print((char*)line_1,1,0);
	lcd_print(lcd_buffer,1,7);
	while(1){
		if(esp_timer_get_time()-last_measure>50000){
			gptimer_get_raw_count(gptimer1, &t);
			snprintf(s_pom,sizeof(s_pom),"%s",timer_count_to_string(t/1000));
			lcd_print(s_pom,0,11);
			for(int i=0;i<10;i++){
				if(i<buffer_length)
					lcd_buffer[i]=buffer[i];
				else lcd_buffer[i]='*';
				}
			lcd_print(lcd_buffer,1,7);
			last_measure=esp_timer_get_time();
		}
	}
	vTaskDelete(NULL);
}
void freeRTOS_TCP_Client_Task(void *arg){
		char host_ip[]="192.168.43.141";
		char buffer[40];
		uint16_t port=8082;
		int state;
		int sockfd=0;
		uint64_t time=esp_timer_get_time();
		int n;
		int tcp_con=0;
		tcp_con=tcp_client_init(host_ip,&port,&sockfd);
			while(1){
				if((tcp_con==-1)&&(esp_timer_get_time()-time>10000000)){
					tcp_con=tcp_client_init(host_ip,&port,&sockfd);
					time=esp_timer_get_time();
				}
				if(xQueueReceive(AlarmInfo,&state ,0)){
					if(tcp_con==1){
						if(state==0){
							strcpy(buffer,"Alarm!!");
							if((n=write(sockfd,buffer,strlen(buffer)))<0);
							bzero(buffer,strlen(buffer));
							//if((n=read(sockfd,buffer,strlen(buffer)))<0);
							//printf("Confirmation from RPi: %s\n",buffer);
						}else if(state==1){
							strcpy(buffer,"Propper password!!");
							if((n=write(sockfd,buffer,strlen(buffer)))<0);
							bzero(buffer,strlen(buffer));
							gptimer_stop(gptimer1);
							gptimer_set_raw_count(gptimer1,0);
							logData_Proc.alarmState=0;
						}else if(state==-1){
							strcpy(buffer,"Critical alarm!!");
							if((n=write(sockfd,buffer,strlen(buffer)))<0);
							bzero(buffer,strlen(buffer));
						}else if(state==2){
							strcpy(buffer,"admin password has been typed");
							if((n=write(sockfd,buffer,strlen(buffer)))<0);
							bzero(buffer,strlen(buffer));
						}else if(state==3){
							sprintf(buffer,"Password has been changed to: %s",passwd);
							if((n=write(sockfd,buffer,strlen(buffer)))<0);
							bzero(buffer,strlen(buffer));
						}
					}else if(state==1){
							gptimer_stop(gptimer1);
							gptimer_set_raw_count(gptimer1,0);
							logData_Proc.alarmState=0;
					}
				}
				if(tcp_con==1&&(esp_timer_get_time()-time>8000000)){
					n=write(sockfd,"Here",5);
					n=socket_read_with_timeout(sockfd, buffer, sizeof(buffer), 10);
					if(n<=0){
						if(cnt>=3){
							ets_printf("[ERROR] Server disconected\n");
							tcp_client_close(&sockfd);
							tcp_con=-1;
							cnt=0;
						}else{
							cnt++;
						}
					}
					time=esp_timer_get_time();
				}
			}
		tcp_client_close(&sockfd);
		vTaskDelete(NULL);
}