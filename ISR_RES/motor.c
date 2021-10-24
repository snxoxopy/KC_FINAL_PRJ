/*
* motor.c
*
* Created: 2018-11-06 오후 3:35:56
*  Author: usuzin
*/
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "motor.h"


volatile int mtr_state = 1;

void motor_init(void)
{
	
	//타이머
	/*
	TCCR1A |= ((1 << WGM11) | (1 << WGM10));	//Fast PWM mode
	TCCR1A |= ((1 << COM1A1) | (1 << COM1B1) );
	TCCR1B |= ( (1 << CS12) | (1 << CS10) );
	TCCR1B |= (1 << WGM12);
	*/
	
	TCCR3A |= (1 << WGM31) | (1 << WGM30);		//Fast PWM mode
	TCCR3A |= ((1 << COM3A1) | (1 << COM3B1));	//비반전모드
	TCCR3B |= ((1 << CS32) | (1 << CS30));		//타이머 분주비 1024으로 설정
	TCCR3B |= (1 << WGM32);						//Fast PWM mode
	
	//모터 출력 방향핀 설정
	FAN1_DDR |= ( (1<<FAN1_forward_PIN) | (1<<FAN1_back_PIN) );
	
	//LED test
	DDRA |= (1 << PORTA0) | (1 << PORTA1) | (1 << PORTA2) | (1 << PORTA3);
	PORTA = 0x00;
}

void CW_rot(int v)
{

	SpeedMotor_F2A(v);
	SpeedMotor_F2B(0);
}

void CW_non_rot(void)
{
	SpeedMotor_F2A(0);
	SpeedMotor_F2B(0);
}

int mtr_v(int res)
{
	int vv;
	if( ((res <= 512) && (res >=350)) ) // 온도 높음
	{
		mtr_state = 1;
		vv=1000;
		PORTB=0x01;   //led off
	}
	else if( ((res <= 349) && (res >=200)) )
	{
		mtr_state = 1;
		vv=600;
		PORTB=0x01;   //led off
		
	}
	else if(((res <= 199) && (res >=1))  )
	{
		mtr_state = 1;
		vv=200;
		PORTB=0x00;   // led on
		// PORTB=0x01;   //led off
	}
	else if( ((res <= 0) && (res >= -200))  )
	{
		mtr_state = 1;
		vv=200;
		PORTB=0x00;   // led on
		
	}
	//else mtr_state = 0;
	if(mtr_state)
	{
		CW_rot(vv);
		//printf("Motor ON\r\n");
	}
	else CW_non_rot();
	
	return vv;
}

