/*
* vdMotCtrl.cpp
*
* Created: 15.01.2020 20:06:48
* Author : Florian
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#include "vdmot.h"
#include "hwe.h"
#include "i2cCmdHndlr.h"
extern "C"{
    #include "usart0.h"
}

#define C_NUM_DRIVES 6
#define ENABLE_DEBUGOUT

extern volatile uint32_t systemTicks;
vdmot drives[C_NUM_DRIVES];
i2cCmdHndlr i2cDev;

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
    
    // TODO: read calibration values from EEP
    
    // drives connected to PINA, PINB, ADCNum
    drives[0] = vdmot(0x36, 0x37, 6); // PD6, PD7, ADC6
    drives[1] = vdmot(0x10, 0x11, 7); // PB0, PB1, ADC7
    drives[2] = vdmot(0x12, 0x13, 0); // PB2, PB3, ADC0
    drives[3] = vdmot(0x16, 0x17, 1); // PB6, PB7, ADC1
    drives[4] = vdmot(0x35, 0x34, 2); // PD5, PD4, ADC2
    drives[5] = vdmot(0x32, 0x33, 3); // PD2, PD3, ADC3
    
    i2cDev = i2cCmdHndlr(0x10);
    
    s_i2cCmd recCmd = s_i2cCmd{.command = 0, .motor = 0};
    
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
                if(cmdBufLen > 1){
                    if (cmdBuf[0] == 'c'){
                        if ( (cmdBuf[1] == '0')
                        || (cmdBuf[1] == '1') )
                        {
                            uint8_t driveNum = cmdBuf[1] - '0';
                            drives[driveNum].closeValve();
                            cmdBufLen = 0;
                        }
                    } else if (cmdBuf[0] == 'm')
                    {
                        if ( (cmdBuf[1] == '0')
                        || (cmdBuf[1] == '1'))
                        {
                            uint8_t driveNum = cmdBuf[1] - '0';
                            if (cmdBufLen == 3){
                                uint8_t pos = cmdBuf[2] - '0';
                                drives[driveNum].gotoPos(pos*25);
                                cmdBufLen = 0;
                            }
                        }
                        } else if (cmdBuf[0] == 'a'){
                        if ( (cmdBuf[1] == '0')
                        || (cmdBuf[1] == '1') )
                        {
                            uint8_t driveNum = cmdBuf[1] - '0';
                            drives[driveNum].adapt();
                            cmdBufLen = 0;
                        }
                    }
                }
            }
            
            if (cmdRec > 0){
                
                if (recCmd.command == 0){ // get general status
                    uint8_t buffer[2] = {0,1};
                    i2cDev.setTxBuf(buffer, 2);
                    } else if (recCmd.command == 1){ // get motor status
                    uint8_t buffer[3] = {1,recCmd.motor, 0};
                    uint8_t motNum = recCmd.motor;
                    if (motNum < C_NUM_DRIVES)
                    buffer[2] = drives[motNum].getState();
                    
                    i2cDev.setTxBuf(buffer, 3);
                    } else if (recCmd.command == 2){ // adapt function
                    uint8_t motNum = recCmd.motor;
                    if (motNum < C_NUM_DRIVES)
                    drives[motNum].adapt();
                    } else {
                }
            }
            
            serialComm();
            
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
            
            if ((systemTicks % 250) == 0){
                idleRatio = idleCnt;
                idleCnt = 0;
                
                taskRatio = (uint8_t)(taskCnt/250);
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

