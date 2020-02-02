/*
* hwe.cpp
*
* Created: 17.01.2020 09:47:44
*  Author: Florian
*/
#include "hwe.h"
#include <avr/io.h>
#include <avr/interrupt.h>

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

ISR(TIMER2_COMPA_vect){
    systemTicks++;
}