#pragma once

#include <stdint.h>

typedef struct AwardSignal
{
    int det_true;
    int road_type;
    int rings_flag;
    int left_line;
    int right_line;
    int white_line;
    int offline;
    int tow_point_true;

    int is_left_ring;
    int is_right_ring;
    int is_barn_in;
    int is_cross;
    int is_straight;
} AwardSignal;

void award_signal_capture(AwardSignal *out_signal);
void award_signal_print(const AwardSignal *signal);
