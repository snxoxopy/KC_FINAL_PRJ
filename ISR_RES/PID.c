/*
 * PID.c
 * Created: 2018-10-29 오후 10:00:46
 * Modified 18.11.05. USUZIN
 */ 

#include "PID.h"

volatile float Kp=1, Ki=0, Kd=0;

unsigned int PID_Control(unsigned int setpoint, unsigned int curPoint)
{
	int pTerm, iTerm, dTerm, pidTerm;
	static int prevError=0, error=0, errorSum=0;
	
	error = setpoint - curPoint;
	errorSum += error;
	pTerm = Kp * error;
	iTerm = Ki * errorSum;
	dTerm = Kd * (error - prevError);
	prevError = error;
	
	pidTerm = pTerm + iTerm + dTerm;
	return pidTerm;
}