#include	<avr/io.h>
#include	<avr/interrupt.h>
#include	<stdio.h>
#include	"usart.h"

volatile unsigned char rx0_buffer[LENGTH_RX_BUFFER], tx0_buffer[LENGTH_TX_BUFFER];
volatile unsigned char rx0_head=0, rx0_tail=0, tx0_head=0, tx0_tail=0;

// ���ͷ�Ʈ USART �ʱ�ȭ
void USART_init(unsigned int ubrr_baud)
{
	UCSR0B |= 1<<RXEN0 | 1<<TXEN0 | 1<<RXCIE0;
	//RXCIE0: ���ͷ�Ʈ enable -> �Է°��� ������ �����ۿ� �����͸� ������
	UBRR0H = 0;
	UBRR0L = ubrr_baud;
}

// ���ͷ�Ʈ�� ���� ���� ���� ȣ��
int USART0_send(char data, FILE *stream)
{
	// txbuffer[] full, �� ���� �� ������ ��ٸ�
	while( (tx0_head+1==tx0_tail) || ((tx0_head==LENGTH_TX_BUFFER-1) && (tx0_tail==0)) );
	
	tx0_buffer[tx0_head] = data;
	tx0_head = (tx0_head==LENGTH_TX_BUFFER-1) ? 0 : tx0_head+1;
	UCSR0B = UCSR0B | 1<<UDRIE0;	// ���� ���ڰ� �����Ƿ� UDRE1 �� ���ͷ�Ʈ Ȱ��ȭ

	return data;
}

// ���ͷ�Ʈ�� ���� ���� ���� ȣ��
int USART0_receive(FILE *stream)
{	unsigned char data;
	
	while( rx0_head==rx0_tail );	// ���� ���ڰ� ����

	data = rx0_buffer[rx0_tail];
	rx0_tail = (rx0_tail==LENGTH_RX_BUFFER-1) ? 0 : rx0_tail + 1;
	
	return data;
}

// USART1 UDR empty interrupt service
ISR(USART0_UDRE_vect)
{
	UDR0 = tx0_buffer[tx0_tail];
	tx0_tail = (tx0_tail==LENGTH_TX_BUFFER-1) ? 0 : tx0_tail+1;
	
	if( tx0_tail==tx0_head)		// ���� ���ڰ� ������ UDRE1 �� ���ͷ�Ʈ ��Ȱ��ȭ
	UCSR0B = UCSR0B & ~(1<<UDRIE0);
	
	//����: UCSR0B = UCSR0B | 1<<UDRIE0;	// ���� ���ڰ� �����Ƿ� UDRE1 �� ���ͷ�Ʈ Ȱ��ȭ
}

// USART1 RXC interrupt service //������ ���� �ڵ�
ISR(USART0_RX_vect)
{	
	volatile unsigned char data;
	
	// rx_buffer[] full, ������ ���� ���� ����
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
