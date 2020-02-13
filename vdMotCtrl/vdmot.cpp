/*
* vdmot.cpp
*
* Created: 15.01.2020 20:32:17
*  Author: Florian
*/

#include "vdmot.h"
#include "hwe.h"

extern uint32_t systemTicks;

#define DELAY_MOTOR_START_TICKS 100

vdmot::vdmot()
:_state(STATE_IDLE)
{
    _state = STATE_IDLE;
};

vdmot::vdmot(uint8_t pinA, uint8_t pinB, uint8_t adcID)
:_state(STATE_IDLE), _pinA(pinA), _pinB(pinB), _adcID(adcID)
{
    HWE::setOutput(pinA);
    HWE::setOutput(pinB);
    
    _stopRun();
    
    // TODO: currently only hardcoded for tests. shall be calibrated automatically
    // TODO: get calib values from EEP
    _adcThdVal = 70;
    _calValue = 0;
    _calValueClose = 0;
    
    _curPos = 0;
    _dir = 0;
    _target = 0;
    _runsSinceClose = 0;
    _ticksRunStart = 0;
    _ticksRunTarget = 0;
};

void vdmot::bgTask(void)
{
    switch (_state)
    {
        case STATE_IDLE:
        //_state = 5;
        break;
        
        case STATE_START_RUNNING: // START_RUNNNING
        if (systemTicks > (_ticksRunStart+DELAY_MOTOR_START_TICKS) ){
            _state = STATE_RUNNING; // RUNNING
        }
        break;
        
        case STATE_RUNNING: // RUNNING
        if (HWE::getAdcVal(_adcID) > _adcThdVal){
            _stopRun();
            if (_dir == 0){
                _curPos = 0;
                } else {
                _curPos = 255; // fully open
            }
        }
        if (systemTicks > _ticksRunTarget){
            _stopRun();
            _curPos = _target;
        }
        break;
        
        case 20: // start adaption
        if (systemTicks > (_ticksRunStart+DELAY_MOTOR_START_TICKS) ){
            _state = 21; //
        }
        break;
        
        case 21: // wait until closed
        if (HWE::getAdcVal(_adcID) > _adcThdVal){
            _stopRun();
            _startRun(1, 50000UL); //
            _state = 24; //
        }
        // target not reached
        if (systemTicks > _ticksRunTarget){
            _stopRun();
            _calValue = 0;
            _calValueClose = 0;
        }
        break;
        
        case 24: // delay
        if (systemTicks > (_ticksRunStart+DELAY_MOTOR_START_TICKS) ){
            _state = 25; //
        }
        break;
        
        case 25: // wait until open
        if (HWE::getAdcVal(_adcID) > _adcThdVal){
            _calValue = systemTicks - _ticksRunStart;
            _stopRun();
            _startRun(0, 50000UL); //
            _state = 30; // RUNNING
        }
        // target not reached
        if (systemTicks > _ticksRunTarget){
            _stopRun();
            _calValue = 0;
            _calValueClose = 0;
        }
        break;
        
        case 30: // close valve
        if (systemTicks > (_ticksRunStart+DELAY_MOTOR_START_TICKS) ){
            _state = 31; // RUNNING
        }
        break;
        
        case 31: // wait until closed
        if (HWE::getAdcVal(_adcID) > _adcThdVal){
            _calValueClose = systemTicks - _ticksRunStart;
            _stopRun();
            _curPos = 0;
        }
        break;
        
        
        default:
        _state = 0;
        break;
    }
}

errT vdmot::adapt(){
    
    _startRun(0, 50000UL); //
    _state = 20;
    
    return NO_ERROR;
}

errT vdmot::closeValve()
{
    if (_state == STATE_IDLE)
    {
        _startRun(0, _calValueClose*2U);
        _target = 0;
        return NO_ERROR;
    } else {
        return ERROR_BUSY;
    }
    
}

// direction: 0 - close, 1 - open
errT vdmot::_startRun(uint8_t direction, uint32_t ticks){
    
    if (_state == STATE_IDLE)
    {
        if (direction)
        {
            HWE::setPin(_pinA,0);
            HWE::setPin(_pinB,1);
        }
        else
        {
            HWE::setPin(_pinA,1);
            HWE::setPin(_pinB,0);
        }
        _dir = direction;
        
        _ticksRunStart = systemTicks;
        _ticksRunTarget = systemTicks + ticks;
        _state = STATE_START_RUNNING;
        return NO_ERROR;
    } else {
        return ERROR_BUSY;
    }            
}

errT vdmot::_stopRun(){
    HWE::setPin(_pinA,0);
    HWE::setPin(_pinB,0);
    
    _state = STATE_IDLE;
    
    return NO_ERROR;
}

uint8_t vdmot::getPos(){
    return _curPos;
}

uint8_t vdmot::getState(){
    return _state;
}

errT vdmot::gotoPos(uint8_t targetPos){
    if ( (_state == STATE_IDLE)
    && (_curPos != targetPos)
    //&& (targetPos >= 0)
    //&& (targetPos <= 100)
    ){
        if (targetPos < _curPos){
            uint8_t move = _curPos - targetPos;
            uint32_t moveTicks = (uint32_t)move * _calValueClose;
            moveTicks = moveTicks / 256;
            _startRun(0, moveTicks);
            _target = targetPos;
        } else {
            uint8_t move = targetPos - _curPos;
            uint32_t moveTicks = (uint32_t)move * _calValue;
            moveTicks = moveTicks / 256;
            _startRun(1, moveTicks);
            _target = targetPos;
        }
    }
}