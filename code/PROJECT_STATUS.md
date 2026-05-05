# 项目当前状态记录

## 日期
2026-04-26

## 当前主线分支
official-motor-base

## 当前路线
旧比赛工程暂时冻结，不再继续在旧工程里硬迁电机。
当前主线改为基于逐飞官方新版库建立新地基：

official_motor_base/E3_motor

逐飞官方库作为 vendor 底层库，我们自己的比赛封装统一放在：

official_motor_base/E3_motor/libraries/smartcar_app

## 已完成模块

### 1. 电机模块 smartcar_motor
已完成并验证。

接口：
motor_init()
motor_stop()
motor_set_pwm(left_percent, right_percent)
motor_set_pwm_x100(left_percent_x100, right_percent_x100)

说明：
motor_set_pwm 使用整数百分比。
motor_set_pwm_x100 使用百分比 x100，例如 550 表示 5.50%。

当前电机方向已修正：
LEFT + 左轮向前
LEFT - 左轮向后
RIGHT + 右轮向前
RIGHT - 右轮向后
BOTH + 前进
BOTH - 后退

当前车辆前进方向对应：
MOTOR1_FORWARD_LEVEL = 0
MOTOR2_FORWARD_LEVEL = 0

### 2. 屏幕模块 smartcar_display
已完成并验证。

接口：
display_init()
display_clear()
display_title()
display_status()
display_line()
display_value()
display_two_values()
display_motor_pwm()
display_show_gray_image()

说明：
已从逐飞 E05_01_ips200_display_demo 迁入 zf_device_ips200_fb。
IPS200 可正常显示文字和摄像头灰度图。

### 3. 编码器模块 smartcar_encoder
已完成并验证。

接口：
encoder_init()
encoder_clear()
encoder_update()
encoder_get_left()
encoder_get_right()
encoder_print()

当前编码器映射：
left = raw2
right = -raw1

验证结果：
左轮向前转，encoder_left 为正。
右轮向前转，encoder_right 为正。

### 4. 摄像头模块 smartcar_camera
已完成并验证。

接口：
camera_init()
camera_update()
camera_get_gray()
camera_get_rgb565()
camera_get_width()
camera_get_height()
camera_print_status()

验证结果：
UVC 摄像头可初始化。
摄像头可持续出帧。
当前图像尺寸为 160x120。
gray 和 rgb 指针正常。
IPS200 可显示摄像头灰度画面。
画面在屏幕左上角显示，属于官方原尺寸显示，不影响后续图像处理。

### 5. 速度闭环模块 smartcar_speed
已完成初版并细化 PWM 输出。

接口：
speed_control_init(sample_ms)
speed_control_stop()
speed_control_set_target(left_count_per_sample, right_count_per_sample)
speed_control_update()
speed_control_get_left_target()
speed_control_get_right_target()
speed_control_get_left_measured()
speed_control_get_right_measured()
speed_control_get_left_pwm()
speed_control_get_right_pwm()
speed_control_get_left_pwm_x100()
speed_control_get_right_pwm_x100()
speed_control_print()

当前采样周期：
100 ms

当前速度单位：
每 100 ms 编码器计数

当前闭环参数：
Kp = 0.001200
Ki = 0.000015
FF = 0.003400
PWM_LIMIT = 20.00%
SLEW_LIMIT = 0.60% / 100ms

当前验证结果：
LEFT T1500 时，ML 可稳定在 1500 附近。
RIGHT T1500 时，MR 可稳定在 1500 附近。
BOTH T1500 时，ML/MR 可稳定跟随目标附近。
PWM 已从整数百分比升级为百分比 x100，输出更细腻，例如 5.08%、5.15%。

## 当前核心结论
当前新地基已经具备：
电机控制
屏幕显示
编码器读取
摄像头采图
速度闭环

可以进入下一阶段：
smartcar_line 最小循迹模块

下一阶段目标：
摄像头灰度图 -> 计算赛道中心 center_x -> 计算 error -> 屏幕显示 error。
在没有真实赛道前，先只算 error，不控车。
到实验室后再连接 speed_control_set_target(base - turn, base + turn) 进行低速循迹。

## 给 B 同学的说明
请切到 official-motor-base 分支。
当前不要继续在旧 main/control-dev 工程里修电机。
后续开发优先基于 official_motor_base/E3_motor/libraries/smartcar_app 里的封装模块继续推进。
