#include "smartcar_encoder.hpp"

#include <cstdio>

// 官方正交编码器设备
static zf_driver_encoder encoder_quad_1(ZF_ENCODER_QUAD_1);
static zf_driver_encoder encoder_quad_2(ZF_ENCODER_QUAD_2);

static bool g_encoder_inited = false;

static int g_left_encoder = 0;
static int g_right_encoder = 0;

void encoder_init(void)
{
    if (g_encoder_inited)
    {
        return;
    }

    g_encoder_inited = true;
    encoder_clear();

    printf("[encoder_init]\n");
    printf("  raw encoder 1 = %s\n", ZF_ENCODER_QUAD_1);
    printf("  raw encoder 2 = %s\n", ZF_ENCODER_QUAD_2);
    printf("  mapping: left = raw2, right = -raw1\n");
}

void encoder_clear(void)
{
    encoder_quad_1.clear_count();
    encoder_quad_2.clear_count();

    g_left_encoder = 0;
    g_right_encoder = 0;
}

void encoder_update(void)
{
    if (!g_encoder_inited)
    {
        encoder_init();
    }

    int raw1 = encoder_quad_1.get_count();
    int raw2 = encoder_quad_2.get_count();

    // 实测映射：
    // 物理左轮向前 -> raw2 为正
    // 物理右轮向前 -> raw1 为负
    g_left_encoder = raw2;
    g_right_encoder = -raw1;

    encoder_quad_1.clear_count();
    encoder_quad_2.clear_count();
}

int encoder_get_left(void)
{
    return g_left_encoder;
}

int encoder_get_right(void)
{
    return g_right_encoder;
}

void encoder_print(void)
{
    printf("[encoder] L=%d R=%d\n", g_left_encoder, g_right_encoder);
}
