#ifndef MOTOR_HPP
#define MOTOR_HPP
//#include "main.hpp"
#include "zf_common_headfile.h"
#include "Servo.hpp"

#define MOTOR1_DIR   "/dev/zf_gpio_motor_1"
#define MOTOR1_PWM   "/dev/zf_pwm_motor_1"

#define MOTOR2_DIR   "/dev/zf_gpio_motor_2"
#define MOTOR2_PWM   "/dev/zf_pwm_motor_2"

void Encoder_Test1();

void Motor_Argument();
void Motor_Init1();
void Motor_PID_Left(void);
void Motor_PID_Right(void);
void Motor_Control();
void Motor_Diff_Pid1();
#endif
