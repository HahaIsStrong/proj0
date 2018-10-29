#ifndef __HEAD_H__
#define __HEAD_H__

#define SERVER_IP  		"192.168.1.123"
#define MYFILE 			"/tmpfile"
#define SERIAL_DEVICE 		"/dev/ttyUSB0"
#define BaudRate 			9600 
/*********************************************************/
#define NAMESIZE 64
typedef struct st1
{
	char type;   
	char name[NAMESIZE];
	char data[NAMESIZE];
}tcp_t; 
// type 
#define RGS_REQUEST 	'R'
#define LOG_REQUEST 	'G'
#define OUT_REQUEST 	'O'
#define RGS_SUCCESS 	5
#define RGS_FAILURE 	6
#define LOG_SUCCESS 	7
#define LOG_FAILURE 	8
#define OUT_SUCCESS 	9
#define OUT_FAILURE 	10
// data is passwd
/*********************************************************/

typedef struct st2
{
	long type;
	char cmd;
}msg_t; 
// type
#define client_to_server 888
// cmd : the same as "cmd for STM32"
// #define LED1_ON 				1
// #define LED1_OFF 				2
// #define LED2_ON 				3
// #define LED2_OFF 				4
// #define LED3_ON 				5
// #define LED3_OFF  				6
// #define BEEP_ON 				7
// #define BEEP_OFF 				8
// #define BEEP_ALARM_ON 			9 	 	// 客户端可以不发送
// #define BEEP_ALARM_OFF 			10 	 	// 客户端可以不发送
// #define GET_TEM_HUM  			11   		// 客户端可以不发送



/*********************************************************/
#define TEMSIZE 16
#define HUMSIZE 16
#define LIGHT_V_SIZE 6
typedef struct st3
{
	int status;   					// Client <------ Server
	char lower_tem[TEMSIZE]; 		// Client ------> Server
	char higher_tem[TEMSIZE]; 		// Client ------> Server
	char tem[TEMSIZE]; 			// Client <------ Server  eg: 30.1C
	char lower_hum[HUMSIZE]; 		// Client ------> Server  
	char higher_hum[HUMSIZE]; 		// Client ------> Server  
	char hum[HUMSIZE]; 			// Client <------ Server  eg: 70.1%
	char light_v[LIGHT_V_SIZE]; 		// Client <------ Server  eg: 0.77v
}shm_t; 
// status
#define SET0(status,bit) status=((~(1<<(bit)))&(status)) 
#define SET1(status,bit) status=((1<<(bit))|(status))
#define V(status,bit)   (((1<<(bit))&(status))?1:0)  //获取status的位bit的值
// bit
#define LED1 			0     		// 1 : ON     0 : OFF 	
#define LED2 			1
#define LED3 			2
#define BEEP 			3 	  	// 蜂鸣器(普通)       客户端可以不显示
#define BEEP_ALARM 	4 	  	// 蜂鸣器(警报)(didi) 客户端可以不显示
#define TEM_HIGH 		5 	
#define TEM_OK 		6 	
#define TEM_LOW 		7 	
#define HUM_HIGH 	8
#define HUM_OK 		9
#define HUM_LOW 		10
#define NO_FIRE 		11
/*********************************************************/

typedef struct st4
{
	long type;
	char cmd;
}stm32_cmd_t;
#define server_waiting_cmd_to_stm32 999
// cmd for STM32 
#define LED1_ON 			1
#define LED1_OFF 			2
#define LED2_ON 			3
#define LED2_OFF 			4
#define LED3_ON 			5
#define LED3_OFF  			6
#define BEEP_ON 			7
#define BEEP_OFF 			8
#define BEEP_ALARM_ON 	9
#define BEEP_ALARM_OFF 	10
#define GET_TEM_HUM  		11   // 自动获取温湿度 , 客户端可以不发送
/***  data from STM32 ***/
// uint8_t hum_int_H;
// uint8_t hum_int_L;
// uint8_t hum_dec;

// uint8_t tem_int_H;
// uint8_t tem_int_L;
// uint8_t tem_dec;

// uint8_t no_fire;

// uint8_t light_v_int;
// uint8_t light_v_point;
// uint8_t light_v_dec_H
// uint8_t light_v_dec_L

// \r
// \n 

#endif

