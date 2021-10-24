/*
 * PID.h
 *
 * Created: 2018-10-29 오후 10:01:46
 *  Author: usuzin
 */ 


#ifndef PID_H_
#define PID_H_


#include <stdio.h>
#include <stdlib.h>

// function declaration
unsigned int PID_Control(unsigned int setpoint, unsigned int curPoint);


#endif /* PID_H_ */