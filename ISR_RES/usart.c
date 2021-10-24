#include	<avr/io.h>
#include	<avr/interrupt.h>
#include	<stdio.h>
#include	"usart.h"

volatile unsigned char rx0_buffer[LENGTH_RX_BUFFER], tx0_buffer[LENGTH_TX_BUFFER];
volatile unsigned char rx0_head=0, rx0_tail=0, tx0_head=0, tx0_tail=0;

// 인터럽트 USART 초기화
void USART_init(unsigned int ubrr_baud)
{
	UCSR0B |= 1<<RXEN0 | 1<<TXEN0 | 1<<RXCIE0;
	//RXCIE0: 인터럽트 enable -> 입력값이 들어오면 링버퍼에 데이터를 쓰게함
	UBRR0H = 0;
	UBRR0L = ubrr_baud;
}

// 인터럽트에 의한 문자 전송 호출
int USART0_send(char data, FILE *stream)
{
	// txbuffer[] full, 한 개라도 빌 때까지 기다림
	while( (tx0_head+1==tx0_tail) || ((tx0_head==LENGTH_TX_BUFFER-1) && (tx0_tail==0)) );
	
	tx0_buffer[tx0_head] = data;
	tx0_head = (tx0_head==LENGTH_TX_BUFFER-1) ? 0 : tx0_head+1;
	UCSR0B = UCSR0B | 1<<UDRIE0;	// 보낼 문자가 있으므로 UDRE1 빔 인터럽트 활성화

	return data;
}

// 인터럽트에 의한 문자 수신 호출
int USART0_receive(FILE *stream)
{	unsigned char data;
	
	while( rx0_head==rx0_tail );	// 수신 문자가 없음

	data = rx0_buffer[rx0_tail];
	rx0_tail = (rx0_tail==LENGTH_RX_BUFFER-1) ? 0 : rx0_tail + 1;
	
	return data;
}

// USART1 UDR empty interrupt service
ISR(USART0_UDRE_vect)
{
	UDR0 = tx0_buffer[tx0_tail];
	tx0_tail = (tx0_tail==LENGTH_TX_BUFFER-1) ? 0 : tx0_tail+1;
	
	if( tx0_tail==tx0_head)		// 보낼 문자가 없으면 UDRE1 빔 인터럽트 비활성화
	UCSR0B = UCSR0B & ~(1<<UDRIE0);
	
	//참고: UCSR0B = UCSR0B | 1<<UDRIE0;	// 보낼 문자가 있으므로 UDRE1 빔 인터럽트 활성화
}

// USART1 RXC interrupt service //링버퍼 구현 코드
ISR(USART0_RX_vect)
{	
	volatile unsigned char data;
	
	// rx_buffer[] full, 마지막 수신 문자 버림
	if( (rx0_head+1==rx0_tail) || ((rx0_head==LENGTH_RX_BUFFER-1) && (rx0_tail==0)) )
	{
		data = UDR0;
	}
	else
	{
		rx0_buffer[rx0_head] = UDR0;
		rx0_head = (rx0_head==LENGTH_RX_BUFFER-1) ? 0 : rx0_head+1;
	}
}

// USART1 receive char check
int	USART0_rx_check(void)
{
	return (rx0_head != rx0_tail) ? 1 : 0;
}
