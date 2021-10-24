/*
 * motor.h
 *
 * Created: 2018-11-06 오후 3:36:11
 *  Author: usuzin
 */ 


#ifndef MOTOR_H_
#define MOTOR_H_

//모터 상태 설정
#define FAN1_DDR			 DDRE
#define FAN1_forward_PIN	 PORTE3
#define FAN1_back_PIN		 PORTE4

//모터 속도 설정
#define   SpeedMotor_F1A(sl)		OCR1A = sl	//PB5
#define	  SpeedMotor_F1B(bl)		OCR1B = bl	//PB6

#define   SpeedMotor_F2A(sr)		OCR3A = sr	//PE3
#define   SpeedMotor_F2B(br)		OCR3B = br	//PE4

void motor_init(void);
void CW_rot(int v);
void CW_non_rot(void);
int mtr_v(int res);

#endif /* MOTOR_H_ */