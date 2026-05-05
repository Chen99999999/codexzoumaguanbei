#include <cstdio>
#include <cstdint>
#include <fcntl.h>

// 直接使用逐飞官方 E3_motor 例程里的 C++ 驱动实现
#include "../libraries/seekfree_official/LS2K300_Library/Example/Motherboard_Demo/E3_motor/libraries/zf_driver/zf_driver_gpio.cpp"
#include "../libraries/seekfree_official/LS2K300_Library/Example/Motherboard_Demo/E3_motor/libraries/zf_driver/zf_driver_pwm.cpp"

static zf_driver_gpio g_dir1(ZF_GPIO_MOTOR_1, O_RDWR);
static zf_driver_gpio g_dir2(ZF_GPIO_MOTOR_2, O_RDWR);
static zf_driver_pwm  g_pwm1(ZF_PWM_MOTOR_1);
static zf_driver_pwm  g_pwm2(ZF_PWM_MOTOR_2);

static struct pwm_info g_pwm1_info;
static struct pwm_info g_pwm2_info;

extern "C" void official_motor_init_bridge(uint32_t *max1, uint32_t *max2)
{
    g_pwm1.get_dev_info(&g_pwm1_info);
    g_pwm2.get_dev_info(&g_pwm2_info);

    g_pwm1.set_duty(0);
    g_pwm2.set_duty(0);

    if (max1) *max1 = g_pwm1_info.duty_max;
    if (max2) *max2 = g_pwm2_info.duty_max;

    printf("[official_bridge_init]\n");
    printf("  ZF_GPIO_MOTOR_1=%s\n", ZF_GPIO_MOTOR_1);
    printf("  ZF_PWM_MOTOR_1 =%s duty_max=%u\n", ZF_PWM_MOTOR_1, g_pwm1_info.duty_max);
    printf("  ZF_GPIO_MOTOR_2=%s\n", ZF_GPIO_MOTOR_2);
    printf("  ZF_PWM_MOTOR_2 =%s duty_max=%u\n", ZF_PWM_MOTOR_2, g_pwm2_info.duty_max);
}

extern "C" void official_motor_stop_bridge(void)
{
    g_pwm1.set_duty(0);
    g_pwm2.set_duty(0);
}

extern "C" void official_motor_set_percent_bridge(int left_percent, int right_percent)
{
    if (left_percent >= 0)
    {
        g_dir1.set_level(1);
        g_pwm1.set_duty((uint32_t)(left_percent * (g_pwm1_info.duty_max / 100)));
    }
    else
    {
        g_dir1.set_level(0);
        g_pwm1.set_duty((uint32_t)((-left_percent) * (g_pwm1_info.duty_max / 100)));
    }

    if (right_percent >= 0)
    {
        g_dir2.set_level(1);
        g_pwm2.set_duty((uint32_t)(right_percent * (g_pwm2_info.duty_max / 100)));
    }
    else
    {
        g_dir2.set_level(0);
        g_pwm2.set_duty((uint32_t)((-right_percent) * (g_pwm2_info.duty_max / 100)));
    }

    printf("[official_bridge_set] L=%d%% R=%d%%\n", left_percent, right_percent);
}
