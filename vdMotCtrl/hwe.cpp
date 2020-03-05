/*
* hwe.cpp
*
* Created: 17.01.2020 09:47:44
*  Author: Florian
*/
#include "hwe.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#include "nvmConfig.h"

volatile uint16_t adcBuffer[9];
volatile uint32_t systemTicks;

void HWE::initAdc(){
    // set reference to internal(1.1V), Note: add cap to Aref!
    // set prescaler to 64 (125k adc freq) -> sampling time ~104µs
    ADMUX = (3<<REFS0) | (0<<MUX0);
    ADCSRA = (1<<ADEN) | (1<<ADIE) | (0x06<<ADPS0);
    // enable adc
    // start first sampling (takes 25 clock cycles)
    ADCSRA |= (1<<ADSC);
    
    DIDR0 |= 0x0F; // digital input disable on ADC0-3
}

void HWE::initSystemTimer(){
    // use for 1ms sampling time generation
    OCR2A = 249;
    TIMSK2 |= (1<<OCIE2A);
    // use in CTC mode, prescaler = 32, TOP = 250 (1ms@8MHz), start immediately
    TCCR2A = (1<<WGM21);
    TCCR2B = (0x03<<CS20);
}

void HWE::setOutput(uint8_t pinNum){
    uint8_t portNum = (pinNum>>4) & 0x0F;
    uint8_t pin = pinNum & 0x07;
    switch ( portNum )
    {
        //case 0:
        //DDRA |= (1<<pin);
        //break;
        
        case 1:
        PORTB |= (1<<pin);
        DDRB |= (1<<pin);
        break;
        
        case 2:
        PORTC |= (1<<pin);
        DDRC |= (1<<pin);
        break;
        
        case 3:
        PORTD |= (1<<pin);
        DDRD |= (1<<pin);
        break;
        
        default:
        break;
    }
    
}

void HWE::setPin(uint8_t pinNum, uint8_t state){
    uint8_t portNum = (pinNum>>4) & 0x0F;
    uint8_t pin = pinNum & 0x0F;
    switch ( portNum )
    {
        //PORTA not present on controller
        
        case 1:
        if (1U == state){
            PORTB |= (1<<pin);
            } else {
            PORTB &= ~(1<<pin);
        }
        break;
        
        case 2:
        if (1U == state){
            PORTC |= (1<<pin);
            } else {
            PORTC &= ~(1<<pin);
        }
        break;
        
        case 3:
        if (1U == state){
            PORTD |= (1<<pin);
            } else {
            PORTD &= ~(1<<pin);
        }
        break;
        
        default:
        break;
    }
    
}

uint16_t HWE::getAdcVal(uint8_t adcNum){
    return adcBuffer[adcNum];
}

// TODO: has to be corrected!
int16_t HWE::getTemp(){
    return ( (adcBuffer[8]-352)*100) / 128 + 25;
}

ISR(ADC_vect){
    // get result
    uint16_t result;// = ADC_RESULT;
    uint16_t oldVal;
    result = ADCL;
    result |= (ADCH<<8);
    
    uint8_t channelNum = ADMUX & 0x0F;

#if 1 // filter enabled
    oldVal = adcBuffer[channelNum];
    result = oldVal * 3 + result;
    result = result/4;
#endif

    adcBuffer[channelNum] = result;
    
    // adc channel 4+5 not used
    if (channelNum < 8){
        if (channelNum == 3){
            channelNum = 6;
        } else {
            channelNum++;
        }
    } else {
        channelNum = 0;
    }
    
    //channelNum = (channelNum >= 8) ? 0 : channelNum+1;
    
    ADMUX = (ADMUX & 0xF0) | (channelNum & 0x0F);
    
    ADCSRA |= (1<<ADSC);
}

// Timer for 1ms base
ISR(TIMER2_COMPA_vect){
    systemTicks++;
}

uint8_t HWE::eepRead(uint16_t startAdr, uint16_t len, void* data){
    
    uint16_t dataPos = 0;
    
    while (dataPos < len)
    {
        //EEARH = (uint8_t)((startAdr+dataPos)>>8)&0xFF;
        //EEARL = (uint8_t)(startAdr+dataPos)&0xFF;
        EEAR = (startAdr+dataPos);
        EECR |= (1<<EERE);
        ((uint8_t*)data)[dataPos] = EEDR;
        dataPos++;
    }
    
    return 0;
}    

uint8_t HWE::eepWrite(uint16_t startAdr, uint16_t len, void* data){
    
    uint16_t dataPos = 0;
    
    while (dataPos < len)
    {
        /* Wait for completion of previous write */
        while(EECR & (1<<EEPE))
        ;
        /* Set up address and Data Registers */
        EEAR = (startAdr+dataPos);
        EEDR = ((uint8_t*)data)[dataPos];
        /* Write logical one to EEMPE */
        EECR |= (1<<EEMPE);
        /* Start eeprom write by setting EEPE */
        EECR |= (1<<EEPE);
        dataPos++;
    }
    return 0;
    
}    

void HWE::bgTask(void){
    if (nvmChanged > 0U)
    {
        eepWrite(0U, sizeof(nvmParam), (void*)&nvmParam);
        
        nvmChanged = 0U;
    }
}