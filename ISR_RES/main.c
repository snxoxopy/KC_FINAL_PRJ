/*
 * ISR_RES.c
 *
 * Created: 2018-11-05 오후 7:29:00
 * Author : usuzin
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <math.h>
#include "usart.h"
#include "PID.h"
#include "motor.h"
#include "UART1.h"

static volatile uint8_t tim;

FILE usart0_stdio = FDEV_SETUP_STREAM(USART0_send, NULL, _FDEV_SETUP_WRITE);
//FILE INPUT = FDEV_SETUP_STREAM(NULL, USART0_receive, _FDEV_SETUP_READ);

ISR(TIMER0_OVF_vect)
{
	tim ++;
	tim %= 64; //0~63까지 숫자를 셈 tim
}

void ADC_init(unsigned char channel)
{
	ADMUX |= (1<<REFS0);	// AVCC(USB 전원5V)기준 전압으로 선택
	
	ADCSRA |= 0x07;			// 분주비 설정
	
	ADCSRA |= (1<<ADEN);	// ADC 활성화
	ADCSRA |= (1<<ADFR);	// 프리러닝 모드
	
	ADMUX = ((ADMUX & 0xE0) | channel );	// 1110 0000 | 채널 채널 선택
	ADCSRA |= (1<<ADSC);	// 변환시작
}

void AVR_init(void)
{
	TCCR0 |= ((1<<CS02) | (1<<CS01) | (1<<CS00)); //1024 64 1초
	TIMSK |= (1 << TOIE0);
}

unsigned int read_ADC(void)
{
	while(!(ADCSRA & (1<<ADIF)));	// 변환 종료 대기 ADIF(Interrupt Flag)
	return ADC;						
}

int main(void)
{
	unsigned int read;
	unsigned int res;
	int bt_rx;
	int mode=1;
	int main_vv=190;
	
	DDRB=0x01;
	PORTB=0x01;

	
	USART_init(BR9600);		//BRR=103: 9600,  USART0 보오레이트 : 115200(UBRR=8), 16MHz
	ADC_init(1);			//AD 변환기 초기화
	AVR_init();
	motor_init();
	USART1_init(BR9600);
	
	sei();	//전역적 인터럽트 허용 : 인터럽트를 통해 UART 링버퍼 구현 하도록 선언

	stdin = stdout = stderr = &usart0_stdio;
	
	printf("The Range of R. [0,1023] [V]\n");
	
	while (1)
	{
		
		
		//CW_cycle();
		//약 0.5초에 한 번씩
		switch(mode)
		{
			case 0:  //수동모드
			bt_rx=USART1_receive();
			if(bt_rx==49)
			{
				PORTB=0x00;
			}
			else if(bt_rx==50)
			{
				PORTB=0x01;
			}
			break;
			
			case 1:
			if(tim == 31)
			{
			//가변저항 PF1 연결
			read = read_ADC();
			printf("Current Vr = ");
			printf("%d\r\n", read);
						
			//이전 값과 현재 입력 비교
			res = PID_Control(512,read);
			printf("The result of PID = ");
			printf("%d\r\n",res);
			
			main_vv = mtr_v(res);
			}
			break;
			
			default:
			break;
		}
		
	
	}
	return 0;
}
