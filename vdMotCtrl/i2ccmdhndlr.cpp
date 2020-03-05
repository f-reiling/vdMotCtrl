/*
* i2ccmdhndlr.cpp
*
* Created: 18.01.2020 19:57:42
*  Author: Florian
*/

#include "avr/interrupt.h"
#include "avr/io.h"
#include <util/twi.h>
#include <string.h>

#include "i2cCmdHndlr.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define F_CPU		8000000UL
#define I2C_SPEED	 100000UL
#define PRESCALER		  1

#define I2C_RXBUFFER_SIZE 6

#define I2C_TWBR_VAL (F_CPU)/(I2C_SPEED*2*PRESCALER)-8/PRESCALER
#if I2C_TWBR_VAL > 255
#error ("TWBR value too large")
#pragma message STR(I2C_TWBR_VAL)
#endif

volatile uint8_t rxDataPtr;
volatile uint8_t rxbuffer[I2C_RXBUFFER_SIZE];
volatile uint8_t flgRxCmd;
volatile uint8_t txDataPtr;
volatile uint8_t txDataLen;
volatile uint8_t txbuffer[8];

i2cCmdHndlr::i2cCmdHndlr(uint8_t address):
_ownAddress(address)
{
    // init i2c interface
    _init();
}


uint8_t i2cCmdHndlr::cmdReceived(){
    return flgRxCmd;
}

uint8_t i2cCmdHndlr::getCmd(s_i2cCmd* cmd){
    
    memset(cmd, 0, 4);
    memcpy(cmd, (const void*)rxbuffer, (rxDataPtr-1));
    
    
    flgRxCmd = 0;
    return 0;
}

uint8_t i2cCmdHndlr::setTxBuf(uint8_t * buffer, uint8_t len){
    memcpy((void*)&txbuffer[1], buffer, len);
    txbuffer[0] = len;
    txDataLen = len+1;
    
    return 0;
}


void i2cCmdHndlr::_init(){
    
    // stop interface
    TWCR &= ~( (1<<TWEA) | (1<<TWEN) );

    // init i2c interface
    TWAR = (_ownAddress << 1);

    // init baudrate
    TWBR = I2C_TWBR_VAL;

    TWCR = (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWEN);
}

ISR(TWI_vect){
    
    //NOTE: code from https://github.com/knightshrub/I2C-slave-lib/blob/master/I2C_slave.c
    
    // temporary stores the received data
    uint8_t dataRec = TWDR;
    uint8_t twsrState = (TWSR & 0xF8);
//    uint8_t twcrVal = TWCR;
    
    switch (twsrState)
    {
        case TW_SR_SLA_ACK:
            rxDataPtr = 0;
            TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
        break;
        
        case TW_SR_DATA_ACK:
            rxbuffer[rxDataPtr++] = dataRec;
            TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
            // next byte is last of buffer size -> send NAK
            if(rxDataPtr >= I2C_RXBUFFER_SIZE-1){
                TWCR &= ~(1<<TWEA);
            }
        break;
        
        case TW_SR_DATA_NACK:
            rxbuffer[rxDataPtr++] = dataRec;
            TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
        break;
        
        case TW_SR_STOP:
            TWCR |= (1<<TWINT) | (1<<TWEA);
            flgRxCmd = 1;
        break;
        
        case TW_ST_SLA_ACK:
            txDataPtr = 0;
        case TW_ST_DATA_ACK:
            if(txDataPtr >= txDataLen)
            {
//              TWCR &= ~(1<<TWEA);
                TWDR = 0;
            } else
            {
                TWDR = txbuffer[txDataPtr++];
            }
            TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
        break;
        
        // last byte transmitted and NACK received from master as expected
        case TW_ST_DATA_NACK:
            TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
        break;
        
        //case TW_ST_DATA_ACK:
        //TWDR = txbuffer[txDataPtr++];
        //twcrVal |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
        //break;
        
        case TW_BUS_ERROR:
            TWCR &= ~( (1<<TWEA) | (1<<TWEN) );
            TWCR = (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWEN);
        break;
        
        
        
        default:
        break;
    }
    
    // finally set TWCR register
    
    // own address has been acknowledged
    //if( (TWSR & 0xF8) == TW_SR_SLA_ACK ){
    //dataPtr = 0;
    //// clear TWI interrupt flag, prepare to receive next byte and acknowledge
    //TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
    //}
    //else if( (TWSR & 0xF8) == TW_SR_DATA_ACK ){ // data has been received in slave receiver mode
    //
    //// save the received byte inside data
    //data = TWDR;
    //
    //// check wether an address has already been transmitted or not
    //if(buffer_address == 0xFF){
    //
    //buffer_address = data;
    //
    //// clear TWI interrupt flag, prepare to receive next byte and acknowledge
    //TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
    //}
    //else{ // if a databyte has already been received
    //
    //// store the data at the current address
    //rxbuffer[buffer_address] = data;
    //
    //// increment the buffer address
    //buffer_address++;
    //
    //// if there is still enough space inside the buffer
    //if(buffer_address < 0xFF){
    //// clear TWI interrupt flag, prepare to receive next byte and acknowledge
    //TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
    //}
    //else{
    //// Don't acknowledge
    //TWCR &= ~(1<<TWEA);
    //// clear TWI interrupt flag, prepare to receive last byte.
    //TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEN);
    //}
    //}
    //}
    //else if( (TWSR & 0xF8) == TW_ST_DATA_ACK ){ // device has been addressed to be a transmitter
    //
    //// copy data from TWDR to the temporary memory
    //data = TWDR;
    //
    //// if no buffer read address has been sent yet
    //if( buffer_address == 0xFF ){
    //buffer_address = data;
    //}
    //
    //// copy the specified buffer address into the TWDR register for transmission
    //TWDR = txbuffer[buffer_address];
    //// increment buffer read address
    //buffer_address++;
    //
    //// if there is another buffer address that can be sent
    //if(buffer_address < 0xFF){
    //// clear TWI interrupt flag, prepare to send next byte and receive acknowledge
    //TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
    //}
    //else{
    //// Don't acknowledge
    //TWCR &= ~(1<<TWEA);
    //// clear TWI interrupt flag, prepare to receive last byte.
    //TWCR |= (1<<TWIE) | (1<<TWINT) | (1<<TWEN);
    //}
    //
    //}
    //else{
    //// if none of the above apply prepare TWI to be addressed again
    //TWCR |= (1<<TWIE) | (1<<TWEA) | (1<<TWEN);
    //}
}