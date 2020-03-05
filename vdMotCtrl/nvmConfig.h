/*
 * nvmConfig.h
 *
 * Created: 21.02.2020 21:15:31
 *  Author: Florian
 */ 


#ifndef NVMCONFIG_H_
#define NVMCONFIG_H_

typedef struct  
{
    
    uint32_t calValue;
    uint32_t calValueClose;
    uint16_t adcThdVal;
} s_vdMotParam;


typedef struct  
{
    uint8_t version;
    uint8_t i2cAddress;
    s_vdMotParam vdMotParam[6];
    
} s_nvmParam;

extern s_nvmParam nvmParam;
extern uint8_t nvmChanged;


#endif /* NVMCONFIG_H_ */