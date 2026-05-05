#pragma once

#include "zf_common_headfile.hpp"

// ============================================================
// smartcar_encoder.hpp
// 编码器封装
//
// 统一规则：
// 左轮向前：encoder_get_left()  > 0
// 右轮向前：encoder_get_right() > 0
// ============================================================

void encoder_init(void);
void encoder_clear(void);
void encoder_update(void);

int encoder_get_left(void);
int encoder_get_right(void);

void encoder_print(void);
