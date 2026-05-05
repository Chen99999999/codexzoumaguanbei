#include "zf_common_headfile.h"

uint8 beep = 0;
volatile uint32 g_pit_ticks = 0;

void Interrupt()
{
    ++g_pit_ticks;
    Motor_Control();
}

void Init_all()
{
    system_delay_ms(500);

    gpio_set_level(BEEP, beep);

    ips200_init("/dev/fb0");
    if (uvc_camera_init("/dev/video0") < 0)
    {
        ips200_show_string(0, 20, "Camera Error");
    }

    Motor_Init1();
    Motor_Argument();

    ips200_full(RGB565_BLUE);
    system_delay_ms(1000);
    ips200_clear();
    pit_ms_init(5, Interrupt);
}
