/*
* IncFile1.h
*
* Created: 17.01.2020 09:40:35
*  Author: Florian
*/


#ifndef HWE_H_
#define HWE_H_

#include <inttypes.h>

class HWE
{
    public:
    static void setOutput(uint8_t pinNum);
    static void setPin(uint8_t pinNum, uint8_t state);
    
    static void initAdc(void);
    static uint16_t getAdcVal(uint8_t adcNum);
    static int16_t getTemp();
    
    static void initSystemTimer(void);
    uint32_t getSystemTicks(void);
    
    static uint8_t eepRead(uint16_t startAdr, uint16_t len, void* data);
    static uint8_t eepWrite(uint16_t startAdr, uint16_t len, void* data);
    
    static void bgTask(void);
    
    //private:
    //static uint32_t _systemTicks;
};


#endif /* INCFILE1_H_ */