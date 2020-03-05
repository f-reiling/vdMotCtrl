/*
* vdMotCtrl.cpp
*
* Created: 15.01.2020 20:06:48
* Author : Florian
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#include "nvmConfig.h"
#include "vdmot.h"
#include "hwe.h"
#include "i2cCmdHndlr.h"
extern "C"{
    #include "usart0.h"
}

#define C_NUM_DRIVES 6
#define ENABLE_DEBUGOUT

#define I2C_CMD_GET_STATUS          0x00
#define I2C_CMD_GET_MOTOR_POS       0x08

#define I2C_CMD_SET_MOTOR_ADAPT     0x20
#define I2C_CMD_SET_MOTOR_MOVE      0x21
#define I2C_CMD_SET_MOTOR_MOVE_REL  0x22 // relative move

extern volatile uint32_t systemTicks;
vdmot drives[C_NUM_DRIVES];
i2cCmdHndlr i2cDev;
s_nvmParam nvmParam;
uint8_t nvmChanged;

int main(void)
{
    uint32_t oldSystemTicks = 0;
    uint32_t idleCnt = 0;
    uint16_t taskCnt = 0;
    uint32_t idleRatio = 0;
    uint8_t taskRatio = 0;
    
    uint8_t cmdBuf[8];
    uint8_t cmdBufLen = 0;
    /* Replace with your application code */
    
    
    // drives connected to PINA, PINB, ADCNum
    drives[0] = vdmot(0x36, 0x37, 6); // PD6, PD7, ADC6
    drives[1] = vdmot(0x10, 0x11, 7); // PB0, PB1, ADC7
    drives[2] = vdmot(0x12, 0x13, 0); // PB2, PB3, ADC0
    drives[3] = vdmot(0x16, 0x17, 1); // PB6, PB7, ADC1
    drives[4] = vdmot(0x35, 0x34, 2); // PD5, PD4, ADC2
    drives[5] = vdmot(0x32, 0x33, 3); // PD2, PD3, ADC3
    
    // TODO: read calibration values from EEP
    HWE::eepRead(0U, sizeof(nvmParam), (void*)&nvmParam );
    for (uint8_t idx = 0; idx < C_NUM_DRIVES; idx++)
    {
        if (nvmParam.vdMotParam[idx].calValue != 0xFFFFFFFF){
            drives[idx].setCalibration(nvmParam.vdMotParam[idx].calValue, 0U);
        }
        if (nvmParam.vdMotParam[idx].calValueClose != 0xFFFFFFFF){
            drives[idx].setCalibration(nvmParam.vdMotParam[idx].calValueClose, 1U);
        }
    }
    
    i2cDev = i2cCmdHndlr(0x10);
    
    s_i2cCmd recCmd = s_i2cCmd{.subdevice = 0, .command = 0};
    
    HWE::initAdc();
    HWE::initSystemTimer();
    Usart0_Init();
    
    sei();
    
    
    while (1)
    {
        //test only!!!
        
        // task every ms
        if (oldSystemTicks != systemTicks){
            taskCnt++;
            
            uint8_t cmdRec = 0;
            if (i2cDev.cmdReceived()){
                cmdRec = 1;
                i2cDev.getCmd(&recCmd);
                } else {
                // TODO!
                //if (uart.cmdReceived()){
                //	cmdRec = 2;
                //	getCmd(&recCmd);
                //}
                uint8_t tempVal = 0;
                
                while (uartGetChar(&tempVal)){
                    cmdBuf[cmdBufLen++] = tempVal;
                    if (cmdBufLen > 6){
                        cmdBufLen = 0;
                    }
                }
                if(cmdBufLen > 1)
                {
                    if (cmdBuf[0] == 'c')
                    {
                        if ( (cmdBuf[1] == '0') || (cmdBuf[1] == '1') )
                        {
                            uint8_t driveNum = cmdBuf[1] - '0';
                            drives[driveNum].closeValve();
                            cmdBufLen = 0;
                        }
                    } else if (cmdBuf[0] == 'm')
                    {
                        if ( (cmdBuf[1] == '0') || (cmdBuf[1] == '1'))
                        {
                            uint8_t driveNum = cmdBuf[1] - '0';
                            if (cmdBufLen == 3){
                                uint8_t pos = cmdBuf[2] - '0';
                                drives[driveNum].gotoPos(pos*25);
                                cmdBufLen = 0;
                            }
                        }
                    } else if (cmdBuf[0] == 'a'){
                        if ( (cmdBuf[1] == '0') || (cmdBuf[1] == '1') )
                        {
                            uint8_t driveNum = cmdBuf[1] - '0';
                            drives[driveNum].adapt();
                            cmdBufLen = 0;
                        }
                    } else if (cmdBuf[0] == 'i'){
                        if ( (cmdBuf[1] == '0') || (cmdBuf[1] == '1') )
                        {
                            uint8_t driveNum = cmdBuf[1] - '0';
                            uint8_t newPos = drives[driveNum].getPos();
                            if (newPos < 255)
                                drives[driveNum].gotoPos(newPos+1);
                            cmdBufLen = 0;
                        }
                    } else if (cmdBuf[0] == 'd'){
                        if ( (cmdBuf[1] == '0') || (cmdBuf[1] == '1') )
                        {
                            uint8_t driveNum = cmdBuf[1] - '0';
                            uint8_t newPos = drives[driveNum].getPos();
                            if (newPos > 0)
                                drives[driveNum].gotoPos(newPos-1);
                            cmdBufLen = 0;
                        }
                    }
                }
            }
            
            if (cmdRec > 0){
                uint8_t rspBuffer[4]; // 0 - subdevice, 1 - command, 2 - response code, 3... - parameter value
                uint8_t rspBufLen = 0;
                uint8_t motNum;
                rspBuffer[0] = recCmd.subdevice;
                rspBuffer[1] = recCmd.command;
                
                if (0U == recCmd.subdevice)
                {
                    if (I2C_CMD_GET_STATUS == recCmd.command)
                    {
                        rspBuffer[2] = 1; // TODO: currently not defined.
                        rspBufLen = 3;
                    }
                } else if (recCmd.subdevice <= C_NUM_DRIVES)
                {
                    motNum = recCmd.subdevice-1U;   
                    if (I2C_CMD_GET_STATUS == recCmd.command)
                    {
                        rspBuffer[2] = drives[motNum].getState();
                        rspBufLen = 3;
                    } else if (recCmd.command == I2C_CMD_GET_MOTOR_POS){ // get position
                        rspBuffer[2] = 0; // no error
                        rspBuffer[3] = drives[motNum].getPos();
                        rspBufLen = 4;
                    
                    } else if (recCmd.command == I2C_CMD_SET_MOTOR_ADAPT){ // adapt function
                        drives[motNum].adapt();
                        rspBufLen = 3;
                    
                    } else if (recCmd.command == I2C_CMD_SET_MOTOR_MOVE){ // move function
                        if (drives[motNum].getState() == STATE_IDLE)
                        {
                            drives[motNum].gotoPos(recCmd.parameter[0]);
                            rspBuffer[2] = 0;
                        } else
                        {
                            rspBuffer[2] = 0xFF; // error
                        }
                        rspBufLen = 3;
                    } else if (recCmd.command == I2C_CMD_SET_MOTOR_MOVE_REL){ // move relative function
                        if (drives[motNum].getState() == STATE_IDLE)
                        {
                            uint8_t newPos = drives[motNum].getPos();
                            uint8_t dir = recCmd.parameter[0];
                            uint8_t steps = recCmd.parameter[1];
                            if (dir == 0U) // move to close
                            {
                                newPos = (steps < newPos) ? newPos - steps : 0;
                            } else
                            {
                                newPos = (steps < (255-newPos) ) ? newPos + steps : 255;
                            }
                            
                            drives[motNum].gotoPos(newPos);
                            rspBuffer[2] = 0;
                        } else
                        {
                            rspBuffer[2] = 0xFF; // error
                        }
                        rspBufLen = 3;
                    }else // invalid command
                    {
                        rspBuffer[2] = 0xFF; //error
                        rspBufLen = 3;
                    }
                    
                } else // invalid subdevice number
                {
                    rspBuffer[2] = 0xFF; // error
                    rspBufLen = 3;
                }
                
                i2cDev.setTxBuf(rspBuffer, rspBufLen);
            }
            
            serialComm();
            HWE::bgTask();
            
            // background task for all drives
            for (uint8_t idxDrv = 0; idxDrv < C_NUM_DRIVES; idxDrv++)
            {
                drives[idxDrv].bgTask();
            }
            
#ifdef ENABLE_DEBUGOUT
            // debug stream output every 20ms
            if ((systemTicks%20) == 0)
            {
                uint16_t temp;
                
                Uart_putc_0(0x03);
                
                for (uint8_t cntAdcNum = 0; cntAdcNum < 8; cntAdcNum++){
                    temp = HWE::getAdcVal(cntAdcNum);
                    Uart_putc_0( (uint8_t)((temp)&0xFF) );
                    Uart_putc_0( (uint8_t)((temp>>8)&0xFF) );
                }
                
                for (uint8_t cntDriveNum = 0; cntDriveNum < C_NUM_DRIVES; cntDriveNum++){
                    Uart_putc_0( drives[cntDriveNum].getState() );
                }
                
                Uart_putc_0( (uint8_t)((systemTicks)&0xFF) );
                Uart_putc_0( (uint8_t)((systemTicks>>8)&0xFF) );
                Uart_putc_0( (uint8_t)((systemTicks>>16)&0xFF) );
                Uart_putc_0( (uint8_t)((systemTicks>>24)&0xFF) );
                
                Uart_putc_0( (uint8_t)((idleRatio)&0xFF) );
                Uart_putc_0( (uint8_t)((idleRatio>>8)&0xFF) );
                Uart_putc_0( (uint8_t)((idleRatio>>16)&0xFF) );
                Uart_putc_0( (uint8_t)((idleRatio>>24)&0xFF) );
                
                Uart_putc_0( (uint8_t)((taskRatio)&0xFF) );
                
                
                Uart_putc_0(0xFC);
            }
#endif
            
            if ((systemTicks % 256) == 0){
                idleRatio = idleCnt;
                idleCnt = 0;
                
                taskRatio = (uint8_t)(taskCnt/256);
                taskCnt = 0;
            }
            
            // At end: update system ticks
            oldSystemTicks = systemTicks;
            
            //TODO: calc load, read timer register value TCNT2.
            // max load is reached if TCNT2 == 249 -> 1ms over!
            taskCnt += TCNT2;
        } else { // else path: systemTick over
            idleCnt++;
        }
        
        
    } // while 1
}

