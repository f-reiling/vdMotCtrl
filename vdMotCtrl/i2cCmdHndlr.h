/*
* i2cCmdHndlr.h
*
* Created: 18.01.2020 19:54:03
* Author: Florian
*/


#ifndef __I2CCMDHNDLR_H__
#define __I2CCMDHNDLR_H__

#include <inttypes.h>

typedef struct{
    uint8_t subdevice;
    uint8_t command;
    uint8_t parameter[2];
}s_i2cCmd;

class i2cCmdHndlr
{
    
    //functions
    public:
    
    i2cCmdHndlr(){};
    i2cCmdHndlr(uint8_t address);
    
    void bgTask();
    
    uint8_t cmdReceived();
    
    uint8_t getCmd(s_i2cCmd* cmd);
    uint8_t setTxBuf(uint8_t * buffer, uint8_t len);
    
    private:
    uint8_t _ownAddress;
    
    void _init();
    

}; //i2cCmdHndlr

#endif //__I2CCMDHNDLR_H__
