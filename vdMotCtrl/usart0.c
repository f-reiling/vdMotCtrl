
#define _USART0_C_

#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <string.h>

//#include "config.h"
#include "usart0.h"
//#include "typedefs.h"

#define C_SIZE_UART_BUFFER 64
typedef struct s_uartFifo
{
	uint8_t data[C_SIZE_UART_BUFFER];
	uint8_t uiInPos;
	volatile uint8_t uiOutPos;
} t_uartFifo;


#ifndef F_CPU
/* In neueren Version der WinAVR/Mfile Makefile-Vorlage kann
   F_CPU im Makefile definiert werden, eine nochmalige Definition
   hier wuerde zu einer Compilerwarnung fuehren. Daher "Schutz" durch
   #ifndef/#endif

   Dieser "Schutz" kann zu Debugsessions führen, wenn AVRStudio
   verwendet wird und dort eine andere, nicht zur Hardware passende
   Taktrate eingestellt ist: Dann wird die folgende Definition
   nicht verwendet, sondern stattdessen der Defaultwert (8 MHz?)
   von AVRStudio - daher Ausgabe einer Warnung falls F_CPU
   noch nicht definiert: */
#warning "F_CPU war noch nicht definiert, wird nun nachgeholt mit 8000000"
#define F_CPU 8000000UL    // Systemtakt in Hz - Definition als unsigned long beachten >> Ohne ergeben Fehler in der Berechnung
#endif

#ifndef BAUD
#define BAUD 38400UL
#endif
// Berechnungen
// UBRR = (F_CPU/16*BAUD)-1

#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)   // clever runden
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))     // Reale Baudrate
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD) // Fehler in Promille, 1000 = kein Fehler.

#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
#error Systematischer Fehler der Baudrate grösser 1% und damit zu hoch!
#endif


t_uartFifo uartTxBuf;
t_uartFifo uartRxBuf;

void Usart0_Init(void)
{
    UCSR0C = (0<<USBS0) | (0<<UMSEL00) | (0<<UMSEL01) | (1<<UCSZ00) | (1<<UCSZ01);	// Asynchron 8N1
//    
#ifdef C_USE_UART_X2
    UCSR0A = (1<<U2X0);	//double speed uart -> Baudrate oben stimmt nicht!
#endif

    UBRR0H = UBRR_VAL >> 8;
    UBRR0L = UBRR_VAL & 0xFF;

    memset((void*)&uartTxBuf.data,0,C_SIZE_UART_BUFFER);
    uartTxBuf.uiInPos = 0;
    uartTxBuf.uiOutPos = 0;
	
    memset((void*)&uartRxBuf.data,0,C_SIZE_UART_BUFFER);
    uartRxBuf.uiInPos = 0;
    uartRxBuf.uiOutPos = 0;

	
	UCSR0B = (1<<TXEN0) | (1<<RXEN0) | (1<<UDRIE0) | (1<<RXCIE0);	// UART TX+RX einschalten, Interruptflags werden in main.c aktiviert!
}

/* Wird jedesmal aufgerufen, wenn Datenregister leer -> zeichen gesendet */

ISR(USART_UDRE_vect)
{
    uartTxBuf.uiOutPos++;
    uartTxBuf.uiOutPos = uartTxBuf.uiOutPos%C_SIZE_UART_BUFFER;
    if (uartTxBuf.uiOutPos != uartTxBuf.uiInPos)
    {
        UDR0 = uartTxBuf.data[uartTxBuf.uiOutPos];
    }
    else
    {
        UCSR0B &= ~(1<<TXEN0); // serielle Übertragung stoppen
        UCSR0B &= ~(1<<UDRIE0);
    }
}

// TODO: handle input data to buffer

ISR(USART_RX_vect)
{
	uartRxBuf.data[uartRxBuf.uiInPos] = UDR0;
	uartRxBuf.uiInPos++;
	uartRxBuf.uiInPos = uartRxBuf.uiInPos%C_SIZE_UART_BUFFER;
}

uint8_t uartGetChar(uint8_t* data){
	if (uartRxBuf.uiOutPos != uartRxBuf.uiInPos)
	{
		*data = uartRxBuf.data[uartRxBuf.uiOutPos];
		uartRxBuf.uiOutPos++;
		uartRxBuf.uiOutPos = uartRxBuf.uiOutPos%C_SIZE_UART_BUFFER;
		return 1;
	} else {
		return 0;
	}
}

uint8_t uartNumRecData(void){
	if (uartRxBuf.uiInPos >= uartRxBuf.uiOutPos){
		uartRxBuf.uiInPos - uartRxBuf.uiOutPos;
	} else {
		(uartRxBuf.uiOutPos+C_SIZE_UART_BUFFER) - uartRxBuf.uiInPos;
	}
}

// Da serielle Übertragung nun interruptgesteuert erfolgt, sind folgende Funktionen nicht mehr notwendig!

/* Problem: Routine wartet bis nächstes Zeichen möglich zu senden */

int Uart_putc_0(unsigned char c)
{
    uartTxBuf.data[uartTxBuf.uiInPos] = c;
    uartTxBuf.uiInPos++;
    uartTxBuf.uiInPos = uartTxBuf.uiInPos%C_SIZE_UART_BUFFER;

    return 0;
}

/* puts ist unabhaengig vom Controllertyp */
void Uart_puts_0 (char *s)
{
    while (*s)
    {
        /* so lange *s != '\0' also ungleich dem "String-Endezeichen" */
        uartTxBuf.data[uartTxBuf.uiInPos] = *s++;
        uartTxBuf.uiInPos++;
        uartTxBuf.uiInPos = uartTxBuf.uiInPos%C_SIZE_UART_BUFFER;

    }
}

// Serial communication task, has to be called cyclically
void serialComm(void)
{
// Checks tx buffer for transmit
    if (uartTxBuf.uiInPos != uartTxBuf.uiOutPos)
    {
        if ((UCSR0B & (1<<TXEN0)) == 0)
        {
            UDR0 = uartTxBuf.data[uartTxBuf.uiOutPos];                      /* sende Zeichen */
            UCSR0B |= (1<<TXEN0); // serielle Übertragung starten
            UCSR0B |= (1<<UDRIE0);
        }
    }
// TODO: parse rx message buffer
}
