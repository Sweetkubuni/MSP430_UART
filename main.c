#include <msp430.h>
#include <stdlib.h>
/*
 * main.c
 */

#define ENABLE_WRITE  UC0IE |= UCA0TXIE
#define DISABLE_WRITE UC0IE &= ~UCA0TXIE
#define ENABLE_READ   UC0IE |= UCA0RXIE;
#define DISABLE_READ  UC0IE &= ~UCA0RXIE;

#define CTS BIT5
#define RTS BIT4

#define ACCEPT_DATA   P2OUT |= CTS
#define DENY_DATA     P2OUT &= ~CTS
#define isMCUReady    if( (P2IN & RTS) == 0 )

struct UART_MANAGER
{
   char *          data;
   unsigned int    count;
   unsigned int    len;
};

static struct UART_MANAGER uart_manager;

void begin_UART()
{
       BCSCTL1 = CALBC1_1MHZ; // Set DCO
       DCOCTL = CALDCO_1MHZ;

    /* Configure Pin Muxing P1.1 RXD and P1.2 TXD */
       P1SEL = BIT1 | BIT2 ;
       P1SEL2 = BIT1 | BIT2;

    /* Place UCA0 in Reset to be configured */
       UCA0CTL1 = UCSWRST;

    /* Configure */
       UCA0CTL1 |= UCSSEL_2; // SMCLK
       UCA0BR0 = 104; // 1MHz 9600
       UCA0BR1 = 0; // 1MHz 9600
       UCA0MCTL = UCBRS0; // Modulation UCBRSx = 1

    /*Initialize USCI state machine*/
       UCA0CTL1 &= ~UCSWRST;
}

/*
 * send_data - transmit information via UART
 */
void send_data( const char * str, size_t n)
{
		uart_manager.count = 0;
		uart_manager.len   = n;
		uart_manager.data  = (char * )str;
		ENABLE_WRITE;

		// block until everything is sent over
		while(uart_manager.count != uart_manager.len) { }
}

/*
 * read_data - read information via UART
 */
void read_data( char * str, size_t n)
{
	uart_manager.count = 0;
	uart_manager.len   = n;
	uart_manager.data  = str;
	ENABLE_READ;
	while( uart_manager.count != 0xDEAD ) { } // block until we read as much as we could
}

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    P2DIR |= CTS;
    P2OUT |= CTS;
    begin_UART();
	__bis_SR_register(GIE); // interrupts enabled

	/* Setup Bluetooth Module */

	char resp[8];
	const char * commands[] = {
			"AT+FACTORYRESET",
			"AT+GAPDEVNAME=Demo",
			"AT+BleHIDEn=On",
			"ATZ",
			"AT+BLEKEYBOARD=z"
	};
	isMCUReady
	{
		send_data(commands[0], 15);
		ACCEPT_DATA;
		read_data(&resp[0], 8);
		DENY_DATA;
	}

	isMCUReady
	{
		send_data(commands[1], 18);
		ACCEPT_DATA;
		read_data(&resp[0], 8);
	    DENY_DATA;
	}

	isMCUReady
	{
		send_data(commands[2], 14);
		ACCEPT_DATA;
		read_data(&resp[0], 8);
		DENY_DATA;
	}

	isMCUReady
	{
		send_data(commands[3], 14);
		ACCEPT_DATA;
		read_data(&resp[0], 8);
		DENY_DATA;
	}

	while(1)
	{
		isMCUReady
		{
			send_data(commands[4], 14);
			ACCEPT_DATA;
			read_data(&resp[0], 8);
			DENY_DATA;
		}
	}

}


#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
	uart_manager.data[uart_manager.count++] = UCA0RXBUF;
	if(uart_manager.len > 2)
	{
	  if( ( (uart_manager.data[uart_manager.count - 1] == '\n') && (uart_manager.data[uart_manager.count - 2] == '\r') ) || (uart_manager.len == uart_manager.count) )
		  {
		  	  DISABLE_READ;
		  	  uart_manager.count = 0xDEAD;
		  }
	}
}

#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
   UCA0TXBUF = uart_manager.data[uart_manager.count++]; // TX next character
   if (uart_manager.count == uart_manager.len) // TX over?
	   DISABLE_WRITE; // Disable USCI_A0 TX interrupt
}
