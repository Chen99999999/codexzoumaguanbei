#ifndef SERVO_HPP
#define SERVO_HPP

#include "zf_common_headfile.h"
// #include "main.hpp"

// #define PWM3 "/dev/zf_device_pwm_servo"  


typedef struct {
    float P;
    float I;
    float D;
    int LastError;  // Error[-1]
    int PrevError;  // Error[-2]
    int EC;
    float Kdin;   //入弯D
    float Kdout;  //出弯D
  } PID_Datatypedef;

#define steer_middle  4605   //4550     
#define steer_right  4150           
#define steer_left   5000            

#define LimitLeft(Left)    (Left = ((Left > steer_left) ? steer_left : Left))
#define LimitRight(Right)  (Right = ((Right < steer_right) ? steer_right : Right))


extern PID_Datatypedef SteerPIDdata;
extern pwm_info steer_pwm_info;
extern int16 Su400_out;
extern float servo_motor_duty;                                                  // 舵机动作角度
extern float servo_motor_dir;                                                      // 舵机动作状态
void Steer_init1(void);
void Steer_PID_Realize(void);
#endif
