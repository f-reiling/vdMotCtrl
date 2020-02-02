// Prototypen für UART-Funktionen

#ifdef _USART0_C_
#define _EXT_
#else
#define _EXT_ extern
#endif

_EXT_ void Usart0_Init(void);
_EXT_ int Uart_putc_0(unsigned char c);
_EXT_ void Uart_puts_0(char *s);
_EXT_ void serialComm(void);

_EXT_ uint8_t uartGetChar(uint8_t* data);
_EXT_ uint8_t uartNumRecData();

#undef _EXT_

