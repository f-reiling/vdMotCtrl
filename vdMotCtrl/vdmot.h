/*
* vdmot.h
*
* Created: 15.01.2020 20:26:11
* Author: Florian
*/


#ifndef __VDMOT_H__
#define __VDMOT_H__

#include <inttypes.h>

enum errT{NO_ERROR=0, 
		  ERROR_GENERIC,
		  ERROR_BUSY,
		  ERROR_NOT_CALIBRATED};
		  
		  
#define STATE_IDLE			0
#define STATE_START_RUNNING	1
#define STATE_RUNNING		2

class vdmot
{
	//functions
	public:
	//virtual ~vdmot(){}
	//virtual void Method1() = 0;
	//virtual void Method2() = 0;
	
	//class error
	//{
		//public:
		//static const uint8_t noError = 0;
	//};
	
	vdmot();
	vdmot(uint8_t pinA, uint8_t pinB, uint8_t adcID);
	
	void bgTask(void);
	
	errT closeValve(void);
	
	uint8_t getPos(void);
	
	uint8_t getState(void);
	
	errT gotoPos(uint8_t pos);
	
	errT adapt(void);
	
	errT setCalibration(uint32_t calValue, uint8_t calType){ if (calType == 0U ) {_calValue = calValue;} else {_calValueClose = calValue;} return NO_ERROR;};
	
	
	private:
	uint8_t _state;
	uint8_t _pinA; //encoded upperNibble-> portNum, lowerNibble -> pinNum
	uint8_t	_pinB;
	uint8_t _adcID;
	
	uint8_t _curPos; // 0-closed - 255 fully open
	uint8_t _dir;
	uint8_t _target;
	uint8_t _runsSinceClose; // IDEA: after N movements goto close first!
	
	uint32_t _ticksRunStart;
	uint32_t _ticksRunTarget;
	uint32_t _calValue; // ticks needed for full move from closed to open!
	uint32_t _calValueClose; // ticks needed from open to close
	uint16_t _adcThdVal; // adc value reached if valve closed
	
	errT _startRun(uint8_t direction, uint32_t ticks);
	errT _stopRun(void);

}; //vdmot

#endif //__VDMOT_H__
