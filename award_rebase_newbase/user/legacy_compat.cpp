#include "legacy_compat.hpp"

cv::Mat First_image;
cv::Mat Cut_image;
cv::Mat Gray_image;
cv::Mat Resized_image;
uint8_t *rgay_image = nullptr;

static constexpr int kAwardImageWidth = 80;
static constexpr int kAwardImageHeight = 60;
static constexpr int kAwardRoiTop = 28;
static constexpr int kAwardRoiHeight = 92;

namespace {

zf_driver_pwm g_pwm_motor_1(ZF_PWM_MOTOR_1);
zf_driver_pwm g_pwm_motor_2(ZF_PWM_MOTOR_2);
zf_driver_pwm g_pwm_servo_1(ZF_PWM_SERVO_1);

zf_driver_gpio g_gpio_motor_1(ZF_GPIO_MOTOR_1, O_RDWR);
zf_driver_gpio g_gpio_motor_2(ZF_GPIO_MOTOR_2, O_RDWR);
zf_driver_gpio g_gpio_beep(ZF_GPIO_BEEP, O_RDWR);
zf_driver_gpio g_gpio_key_1(ZF_GPIO_KEY_1, O_RDWR);
zf_driver_gpio g_gpio_key_2(ZF_GPIO_KEY_2, O_RDWR);
zf_driver_gpio g_gpio_key_3(ZF_GPIO_KEY_3, O_RDWR);
zf_driver_gpio g_gpio_key_4(ZF_GPIO_KEY_4, O_RDWR);

zf_driver_encoder g_encoder_1(ZF_ENCODER_QUAD_1, O_RDWR);
zf_driver_encoder g_encoder_2(ZF_ENCODER_QUAD_2, O_RDWR);

zf_driver_pit g_pit_timer;
zf_driver_tcp_client g_tcp_client;
zf_device_uvc g_uvc_dev;
zf_device_ips200 g_ips200_dev;

zf_driver_pwm *resolve_pwm(const char *path)
{
    if (!path) return nullptr;
    if (strcmp(path, ZF_PWM_MOTOR_1) == 0 || strcmp(path, "/dev/zf_device_pwm_motor_1") == 0) return &g_pwm_motor_1;
    if (strcmp(path, ZF_PWM_MOTOR_2) == 0 || strcmp(path, "/dev/zf_device_pwm_motor_2") == 0) return &g_pwm_motor_2;
    if (strcmp(path, ZF_PWM_SERVO_1) == 0 || strcmp(path, "/dev/zf_device_pwm_servo") == 0) return &g_pwm_servo_1;
    return nullptr;
}

zf_driver_gpio *resolve_gpio(const char *path)
{
    if (!path) return nullptr;
    if (strcmp(path, ZF_GPIO_MOTOR_1) == 0 || strcmp(path, "/dev/zf_driver_gpio_motor_1") == 0) return &g_gpio_motor_1;
    if (strcmp(path, ZF_GPIO_MOTOR_2) == 0 || strcmp(path, "/dev/zf_driver_gpio_motor_2") == 0) return &g_gpio_motor_2;
    if (strcmp(path, ZF_GPIO_BEEP) == 0 || strcmp(path, "/dev/zf_driver_gpio_beep") == 0) return &g_gpio_beep;
    if (strcmp(path, ZF_GPIO_KEY_1) == 0 || strcmp(path, "/dev/zf_driver_gpio_key_3") == 0 || strcmp(path, SWITCH_1) == 0) return &g_gpio_key_1;
    if (strcmp(path, ZF_GPIO_KEY_2) == 0 || strcmp(path, "/dev/zf_driver_gpio_key_2") == 0 || strcmp(path, SWITCH_2) == 0) return &g_gpio_key_2;
    if (strcmp(path, ZF_GPIO_KEY_3) == 0 || strcmp(path, "/dev/zf_driver_gpio_key_1") == 0) return &g_gpio_key_3;
    if (strcmp(path, ZF_GPIO_KEY_4) == 0 || strcmp(path, "/dev/zf_driver_gpio_key_0") == 0) return &g_gpio_key_4;
    return nullptr;
}

zf_driver_encoder *resolve_encoder(const char *path)
{
    if (!path) return nullptr;
    if (strcmp(path, ZF_ENCODER_QUAD_1) == 0 || strcmp(path, "/dev/zf_encoder_1") == 0) return &g_encoder_1;
    if (strcmp(path, ZF_ENCODER_QUAD_2) == 0 || strcmp(path, "/dev/zf_encoder_2") == 0) return &g_encoder_2;
    return nullptr;
}

}  // namespace

void pwm_get_dev_info(const char *path, struct pwm_info *pwm_info)
{
    auto *obj = resolve_pwm(path);
    if (obj && pwm_info) obj->get_dev_info(pwm_info);
}

void pwm_set_duty(const char *path, uint16 duty)
{
    auto *obj = resolve_pwm(path);
    if (obj) obj->set_duty(duty);
}

void gpio_set_level(const char *path, uint8 dat)
{
    auto *obj = resolve_gpio(path);
    if (!obj) return;

    uint8 level = dat;
    if (path && strcmp(path, ZF_GPIO_MOTOR_2) == 0)
    {
        // Keep the award motor logic unchanged and adapt the new base polarity here.
        level = dat ? 0 : 1;
    }

    obj->set_level(level);
}

uint8 gpio_get_level(const char *path)
{
    auto *obj = resolve_gpio(path);
    return obj ? obj->get_level() : 0;
}

int16 encoder_get_count(const char *path)
{
    auto *obj = resolve_encoder(path);
    if (!obj) return 0;
    const int16 count = obj->get_count();
    obj->clear_count();
    return count;
}

void pit_ms_init(uint32_t ms, void (*callback)(void))
{
    g_pit_timer.init_ms(ms, callback);
}

int8 tcp_client_init(const char *ip_addr, uint32 port)
{
    g_tcp_client.set_retry_param(2, 100);
    return g_tcp_client.init(ip_addr, port);
}

uint32 tcp_client_send_data(const uint8 *buff, uint32 length)
{
    return g_tcp_client.send_data(buff, length);
}

uint32 tcp_client_read_data(uint8 *buff, uint32 length)
{
    return g_tcp_client.read_data(buff, length);
}

int8 uvc_camera_init(const char *path)
{
    return g_uvc_dev.init(path);
}

int8 wait_image_refresh(void)
{
    if (g_uvc_dev.wait_image_refresh() < 0) return -1;

    First_image = g_uvc_dev.get_frame_mjpg();
    uint8_t *gray_ptr = g_uvc_dev.get_gray_image_ptr();
    if (!gray_ptr) return -1;

    cv::Mat full_gray(UVC_HEIGHT, UVC_WIDTH, CV_8UC1, gray_ptr);
    const int roi_top = (kAwardRoiTop >= 0 && kAwardRoiTop < UVC_HEIGHT) ? kAwardRoiTop : 0;
    const int roi_height = (roi_top + kAwardRoiHeight <= UVC_HEIGHT) ? kAwardRoiHeight : (UVC_HEIGHT - roi_top);
    cv::Rect roi(0, roi_top, UVC_WIDTH, roi_height);
    cv::Mat roi_gray = full_gray(roi);
    cv::resize(roi_gray, Gray_image, cv::Size(kAwardImageWidth, kAwardImageHeight), 0, 0, cv::INTER_AREA);
    Cut_image = Gray_image;
    Resized_image = Gray_image;
    rgay_image = Gray_image.data;
    return 0;
}

void ips200_init(const char *path)
{
    g_ips200_dev.init(path);
}

void ips200_clear(void)
{
    g_ips200_dev.clear();
}

void ips200_full(const uint16 color)
{
    g_ips200_dev.full(color);
}

void ips200_draw_point(uint16 x, uint16 y, const uint16 color)
{
    g_ips200_dev.draw_point(x, y, color);
}

void ips200_draw_line(uint16 x_start, uint16 y_start, uint16 x_end, uint16 y_end, const uint16 color)
{
    g_ips200_dev.draw_line(x_start, y_start, x_end, y_end, color);
}

void ips200_show_char(uint16 x, uint16 y, const char dat)
{
    g_ips200_dev.show_char(x, y, dat);
}

void ips200_show_string(uint16 x, uint16 y, const char dat[])
{
    g_ips200_dev.show_string(x, y, dat);
}

void ips200_show_int(uint16 x, uint16 y, const int32 dat, uint8 num)
{
    g_ips200_dev.show_int(x, y, dat, num);
}

void ips200_show_uint(uint16 x, uint16 y, const uint32 dat, uint8 num)
{
    g_ips200_dev.show_uint(x, y, dat, num);
}

void ips200_show_float(uint16 x, uint16 y, const double dat, uint8 num, uint8 pointnum)
{
    g_ips200_dev.show_float(x, y, dat, num, pointnum);
}

void ips200_show_gray_image(uint16 x, uint16 y, const uint8 *image, uint16 width, uint16 height)
{
    g_ips200_dev.show_gray_image(x, y, image, width, height, width, height, 0);
}
