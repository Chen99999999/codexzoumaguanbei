#ifndef LEGACY_COMPAT_HPP
#define LEGACY_COMPAT_HPP

#include "zf_common_typedef.hpp"
#include "zf_driver_pwm.hpp"
#include "zf_driver_gpio.hpp"
#include "zf_driver_encoder.hpp"
#include "zf_driver_pit.hpp"
#include "zf_driver_tcp_client.hpp"
#include "zf_device_uvc.hpp"
#include "zf_device_ips200_fb.hpp"
#include "seekfree_assistant.hpp"
#include "seekfree_assistant_interface.hpp"
#include <opencv2/opencv.hpp>

#define DIR1        ZF_GPIO_MOTOR_1
#define PWM2        ZF_PWM_MOTOR_1
#define DIR2        ZF_GPIO_MOTOR_2
#define PWM1        ZF_PWM_MOTOR_2
#define PWM3        ZF_PWM_SERVO_1

#define ENCODER_1   ZF_ENCODER_QUAD_1
#define ENCODER_2   ZF_ENCODER_QUAD_2

#define BEEP        ZF_GPIO_BEEP
#define KEY_1       ZF_GPIO_KEY_1
#define KEY_2       ZF_GPIO_KEY_2
#define KEY_3       ZF_GPIO_KEY_3
#define KEY_4       ZF_GPIO_KEY_4
#define SWITCH_1    ZF_GPIO_KEY_1
#define SWITCH_2    ZF_GPIO_KEY_2

extern cv::Mat First_image;
extern cv::Mat Cut_image;
extern cv::Mat Gray_image;
extern cv::Mat Resized_image;
extern uint8_t *rgay_image;

void pwm_get_dev_info(const char *path, struct pwm_info *pwm_info);
void pwm_set_duty(const char *path, uint16 duty);

void gpio_set_level(const char *path, uint8 dat);
uint8 gpio_get_level(const char *path);

int16 encoder_get_count(const char *path);

void pit_ms_init(uint32_t ms, void (*callback)(void));

int8 tcp_client_init(const char *ip_addr, uint32 port);
uint32 tcp_client_send_data(const uint8 *buff, uint32 length);
uint32 tcp_client_read_data(uint8 *buff, uint32 length);

int8 uvc_camera_init(const char *path);
int8 wait_image_refresh(void);

void ips200_init(const char *path);
void ips200_clear(void);
void ips200_full(const uint16 color);
void ips200_draw_point(uint16 x, uint16 y, const uint16 color);
void ips200_draw_line(uint16 x_start, uint16 y_start, uint16 x_end, uint16 y_end, const uint16 color);
void ips200_show_char(uint16 x, uint16 y, const char dat);
void ips200_show_string(uint16 x, uint16 y, const char dat[]);
void ips200_show_int(uint16 x, uint16 y, const int32 dat, uint8 num);
void ips200_show_uint(uint16 x, uint16 y, const uint32 dat, uint8 num);
void ips200_show_float(uint16 x, uint16 y, const double dat, uint8 num, uint8 pointnum);
void ips200_show_gray_image(uint16 x, uint16 y, const uint8 *image, uint16 width, uint16 height);

#endif
