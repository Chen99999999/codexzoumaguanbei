#include "Servo.hpp"

PID_Datatypedef SteerPIDdata;  
struct pwm_info steer_pwm_info;
int16 Su400_out;

float servo_motor_duty = 90.0;                                                  // 舵机动作角度
float servo_motor_dir = 1;                                                      // 舵机动作状态


void Steer_init1()
{
    pwm_get_dev_info(PWM3, &steer_pwm_info);
    pwm_set_duty(PWM3, steer_middle);
}

void Steer_PID_Realize(void) 
{
    float iError, SteerErr;  //
    float kp_two = 0.3;
    float offset = ImageStatus.Det_True - ImageStatus.MiddleLine;
    iError = offset;  
    SteerErr =
              SteerPIDdata.P * iError +
              iError * abs(iError) * kp_two + (iError - SteerPIDdata.LastError) * SteerPIDdata.D; 
    SteerPIDdata.LastError = iError;
    Su400_out = int16(steer_middle - SteerErr);
    if(Su400_out >= steer_left)	    Su400_out = steer_left;
    if(Su400_out <= steer_right)	Su400_out = steer_right;

    pwm_set_duty(PWM3, Su400_out);
}
