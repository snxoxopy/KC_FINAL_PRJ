#include "main.h"
#include "stm32f4xx_hal.h"
#include "app.h"
#include "i2c_hd44780.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "key.h"

extern I2C_HandleTypeDef hi2c1;
extern DMA_HandleTypeDef hdma_i2c1_rx;
extern DMA_HandleTypeDef hdma_i2c1_tx;

extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;

extern UART_HandleTypeDef huart1;

extern TIM_HandleTypeDef htim5;
extern TIM_HandleTypeDef htim11;

uint32_t t_rising, t_falling;
uint32_t dist;


uint8_t f_fnd[3], f_dht=0;
volatile	uint8_t fnd_buff = 0x00;
void displayFND(uint8_t number);
void ShiftClock(void);
void LatchClock(void);

//1ms에 한번씩 호출하는 함수
void HAL_SYSTICK_Callback(void)
{
	//1초마다 f_btn 확인
	static  uint16_t t_dht = 0;
	static  uint8_t t_fnd = 0;
	static	uint8_t t_reg = 0;

	t_dht++; t_dht %= 10000;			//t_btn = t_btn%1000 0부터 9999까지 count
	t_fnd++; t_fnd %= 256;			//t_led = t_led%1000
	t_reg++; t_reg %= 1000;
	
	if(t_reg == 0)
	{
		static int ui = 0;
		ui%=10;
		displayFND(ui);
		ui++;
	}
	
	if(t_dht == 0)
	{
		f_dht = 1; //t_btn = 0일때 마다 f_btn =1 즉, 100번카운트 할때 1
	}
	//else f_dht = 0;
	
	
	if(t_fnd == 0)
	{
		f_fnd[0] = 1;
		f_fnd[1] = 0;
		f_fnd[2] = 0;
			
	}
	else if(t_fnd == 84)
	{
		f_fnd[0] = 0;
		f_fnd[1] = 1;
		f_fnd[2] = 0;
	}
	else if(t_fnd == 168)
	{
		f_fnd[0] = 0;
		f_fnd[1] = 0;
		f_fnd[2] = 1;
	}
}

/*
void DispThread_1(void *arg)
{
	uint8_t i;

		for (i = 0;;)
		{
			lcd_locate(1,0);
			lcd_printf("1. Count=%d", i++);
			printf("1. Count=%d\r\n", i);
		}
}

void DispThread_2(void *arg)
{
	uint8_t i;

	for (i = 0;;)
	{
			f_dht=0;
			lcd_locate(2,0);
			lcd_printf("2. Count=%d", i++);
			printf("2. Count=%d\r\n", i);
	}
}
*/

/*
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
		GPIO_PinState sts = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);
		if(sts == GPIO_PIN_SET)
		{
			t_rising = __HAL_TIM_GET_COUNTER(&htim2);
		}
		else t_falling = __HAL_TIM_GET_COUNTER(&htim2);
		dist = t_falling - t_rising;
}*/


uint8_t mtr_state = 0;
uint8_t cmp_val = 150;

uint16_t get_time(void)
{
	return (uint16_t)__HAL_TIM_GET_COUNTER(&htim11);
}

void set_time(uint16_t time)
{
	__HAL_TIM_SET_COUNTER(&htim11, time);
}

// PA0 핀 출력을 1로
// 하위 16비트가 핀을 1로 설정하는 비트임
// 예로 PA3 핀을 1로 설정하려면 0x00000004; 를 BSRR 레지스터에 입력하면 됨.
inline void pin_high(void)
{
	  GPIOA->BSRR = 0x00000001;
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
}


// PA0 핀 출력을 0으로
// 상위 16비트가 핀을 0으로 설정하는 비트임
// 예는 pin_high와 같은 개념임
inline void pin_low(void)
{
	GPIOA->BSRR = 0x00010000;
	//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
}

// 원본:PA0 핀 상태를 읽음
// 수정:PB1 핀 상태를 읽음
// 하위 16비트 해당 핀은 비트와 동일
uint8_t pin_get(void)
{
	return (uint8_t)((GPIOA->IDR & 0x00001) >> 0);
	//return (uint8_t)((GPIOA->IDR & 0x00001) >> 0);
}

// us단위 대기..
// 16비트이므로 최대 (65535 - 1)us 만큼 가능
void pin_out_wait(uint16_t time)
{
	volatile uint16_t start, curr;

	start = get_time();
	while (1) {
		curr = get_time();
		if ((uint16_t)(curr - start) > time) break;
	}
}

// rising edge면 	0
// falling edge면 	1
// rising edge면 low level 유지 시간을 time으로 return
// falling edge면 high level 유지 시간을 time으로 return
// DHT11의 각 시그널의 길이는 80us 넘지 않음
// 150us 이상 넘으면 센서 응답의 끝으로 볼 수 있음
int8_t pin_get_change(uint16_t *time)
{
	volatile uint8_t pin_prev; 
	volatile uint16_t start;

	pin_prev= pin_get();     	// 현재 핀 상태 저장
	start = get_time();				// 시작하는 시간 저장 

	while (1) {
		if (pin_prev != pin_get()) { 		// 핀 상태가 변하는가?
			*time = get_time() - start;		// 변했을 때 핀의 상태가 얼마나 	유지된 시간
			break;
		} else {
			if (get_time() - start > 150) return -1;		// 그렇지 않고 150us이상 변화가 없으면 time-out
		}
	}
	
	return !pin_get();      // 핀 상태 return
}  	

// sts는 DHT11에서 출력되는 signal이 low인지 high인지 
// time은 signal이 유지되는 시간
typedef struct {
	int8_t sts;
	uint16_t time;
} PIN_T;

void mtr_main()
{
	
	lcd_init();
	lcd_disp_on();
	lcd_clear_display();
	lcd_home();
	
	lcd_print_string("-JAMONGDA-");	
	
	
	PIN_T pin_sts[100];   // DHT11에서 출력되는 되는 시그널을 저장하기 위한 변수
	uint8_t data[5];			// pin_sts에 저장된 시그널을 분석해서 바이트 단위로 저장하기 위한 변수
	uint8_t i, j, k, l;
	int8_t err;
	uint8_t checksum;

	uint8_t light_state = 0;
	uint8_t data_buff;
	uint8_t dht_state = 0;
	
	char temp_buff[100];
	
	printf("Hello uart!\r\n");
	
	HAL_TIM_Base_Start(&htim11);			// 타이머 11번 16비트, 1us 단위로 설정
	
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
	
	HAL_TIM_PWM_Start(&(htim5), TIM_CHANNEL_2);
	
	while (1)
	{
		if (f_dht == 1)
		{
			f_dht = 0;			
			memset(data, 0, 5);
			err = 0;

			// 디바이스(DHT11)에 시작 요청
			pin_low();                         	
			pin_out_wait(18000);
			pin_high();
			pin_out_wait(40);
			
			for (i=0; i<100; i++)
			{ //83개만 읽으면 됨... : 테스트 결과
				pin_sts[i].sts = pin_get_change(&pin_sts[i].time);
				if (pin_sts[i].sts == -1)
				{
					err = -1;      // 센서 응답 끝이나 응답이 없을 때 
					break;
				}
			}

				printf("err code = %d\n", err);
				printf("i = %d\n", i);

				l = 0; k = 0;
				for (j=3; j<i; j+=2)
				{
					if (pin_sts[j].time > 50)
					{ 
						// MSB first : 첫비트가 제일 먼저옴
						data[l] |= (0x80 >> k);
					}
					k++;
					k %= 8;	// 8비트 단위	
					
					if (k == 0)
					{  // k가 0이면 다음 바이트
						l++;
						if (l >= 5) break;    // 5바이트 넘으면 끝.
					}
				}
				
				printf("result------\n");
				//배열 데이터 확인
				for (i=0; i<l; i++)
				{
					printf("[%3d]%3d,%02x\n", i, data[i], data[i]);
				}
				
				checksum = 0;
				for (i=0; i<4; i++)
				{
					checksum += data[i];
				}

				if (checksum != data[4])
				{
					printf("Checksum error\n");
				}
				else
				{
					printf("Checksum ok!\n");	
					printf("Humidity:%d.%d%%\n", data[0], data[1]);
					printf("Temperature:%d.%dC\n", data[2], data[3]);
					lcd_locate(2,0);
					sprintf(temp_buff,"Temp.: %d. %dC", data[2], data[3]);
					lcd_print_string(temp_buff);	
				}
		}
		
		
		//백열등 on/off
		if(getkey() == 1)
		{
			light_state = !light_state;
			
			#if 1
			if(light_state)
			{
				printf("Motor ON\r\n");
				printf("state: %u\r\n", dht_state);
				//HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
			}
			#else
			if(light_state)
			{
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
			}
			else HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
			//GPIOB->BSRR = 0x00020000;
			#endif
		}
		
		
		//data_buff[2]: (Decimal) Temp.
		//data_buff[3]: (integral) Temp.
		
		if( (27>= data[2] && data[2]>25) ) // 27>=T(dec)>25
		{
			dht_state = 1;
		}
		else if( (28>= data[2] && data[2]>27) ) // 28>=T(dec)>27
		{
			dht_state = 2;
		}
		else if( (31>= data[2] && data[2]>28) ) // 31>=T(dec)>28
		{
			dht_state = 3;
		}
		else
		{
			dht_state = 0;
		}
		
		switch (dht_state)
		{
			case 1:
				// LED1 on Mtr on, 27>=T(dec)>25
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
				//HAL_TIM_Base_Start(&htim2);
				// *(uint32_t *) 0x40000c38 = 100;
				//150부터 모터 on
				__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 180);
				//printf("dht_state1\r\n");
				break;
				
			case 2:
				// LED1 on Mtr on, 30>=T(dec)>27
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
				__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 200);
				//printf("dht_state2\r\n");
				break;
			case 3:
				// LED1 off Mtr 250 on, 32>=T(dec)>30
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
				__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 220);
				//printf("dht_state3\r\n");
				break;
			default:
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
				__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, cmp_val);
				//printf("dht_state: default\r\n");
				break;
		}	
		
	
		if(f_fnd[0] == 1) //1의 자리
		{
			//displayFND(1);			
			data_buff = (data[2]%10);
			//printf("10: %u\r\n", data_buff);
		}
		else if(f_fnd[1] == 1) //10의 자리
		{
			//displayFND(2);
			data_buff = (data[2]/10);
			//printf("1: %u\r\n", data_buff);
		}
		else if(f_fnd[2] == 1)
		{
			//displayFND(0);
			//printf("C: 0\r\n");
		}
	}
}		
	
	//Mtr_test

/*
	while(1)
	{
		//스위치 ON일때 모터 ON
		GPIO_PinState sts = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);
		if(sts == 1)
		{
			HAL_Delay(50);
			mtr_state = 1;
		}
		if(mtr_state)
		{
			#if 1
			HAL_TIM_PWM_Start(&(htim5), TIM_CHANNEL_2);
			HAL_TIM_Base_Start(&htim2);
			// *(uint32_t *) 0x40000c38 = 100;
			//150부터 모터 on
			__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, cmp_val);
			printf("Motor ON\r\n");
			printf("cmp_val: %u\r\n", cmp_val);
			
			#else // test led
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
				HAL_Delay(500);
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
				HAL_Delay(500);
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
				HAL_Delay(500);
			#endif
			
		}
		//printf("mtr_state: %d\r\n", mtr_state);
		
	}*/

int fputc(int ch, FILE *f)
{
	HAL_UART_Transmit(&huart1, (uint8_t *) &ch, 1, 10);
	return ch;
}

#define Data_pin	GPIO_PIN_15
#define Latchpin	GPIO_PIN_14
#define Clockpin	GPIO_PIN_13


void ShiftClock(void)
{
	HAL_GPIO_WritePin(GPIOB, Clockpin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, Clockpin, GPIO_PIN_RESET);
}

void LatchClock(void)
{
	HAL_GPIO_WritePin(GPIOB, Latchpin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, Latchpin, GPIO_PIN_RESET);
}

void displayFND(uint8_t number)
{
									// 0,     1,    2,    3,    4,    5,    6,    7,    8,    9
	//uint8_t num[] = {0x3f, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x27, 0x7F, 0x67};
	uint8_t num[] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xd8, 0x80, 0x98};
	//uint8_t findselect[] = {0x08, 0x04, 0x02, 0x01};
	uint8_t fnd_buff;
	fnd_buff = num[number];
	
	for(uint8_t i =0; i<8; i++)
	{
		//fnd_buff = (num[number] << i);
		if(fnd_buff & 0x80) //상위비트 먼저 읽어오기
		{
			HAL_GPIO_WritePin(GPIOB, Data_pin, GPIO_PIN_SET);
		}
		else
		{
			HAL_GPIO_WritePin(GPIOB, Data_pin, GPIO_PIN_RESET);
		}
		
		ShiftClock();
		
		fnd_buff = fnd_buff << 1; //다음 비트를 읽기위해 시프트
	}	
	LatchClock();
}

