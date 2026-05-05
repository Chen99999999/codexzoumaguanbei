#include "award_signal.hpp"

#include "image.hpp"

#include <cstdio>

void award_signal_capture(AwardSignal *out_signal)
{
    if (out_signal == nullptr)
    {
        return;
    }

    out_signal->det_true = ImageStatus.Det_True;
    out_signal->road_type = (int)ImageStatus.Road_type;
    out_signal->rings_flag = (int)ImageFlag.image_element_rings_flag;
    out_signal->left_line = (int)ImageStatus.Left_Line;
    out_signal->right_line = (int)ImageStatus.Right_Line;
    out_signal->white_line = (int)ImageStatus.WhiteLine;
    out_signal->offline = (int)ImageStatus.OFFLine;
    out_signal->tow_point_true = (int)ImageStatus.TowPoint_True;

    out_signal->is_left_ring = (ImageStatus.Road_type == LeftCirque) ? 1 : 0;
    out_signal->is_right_ring = (ImageStatus.Road_type == RightCirque) ? 1 : 0;
    out_signal->is_barn_in = (ImageStatus.Road_type == Barn_in) ? 1 : 0;
    out_signal->is_cross = (ImageStatus.Road_type == Cross_ture) ? 1 : 0;
    out_signal->is_straight = (ImageStatus.Road_type == Straight) ? 1 : 0;
}

void award_signal_print(const AwardSignal *signal)
{
    if (signal == nullptr)
    {
        return;
    }

    printf("[award_signal] det=%d road=%d ring=%d L=%d R=%d W=%d off=%d tow=%d left_ring=%d right_ring=%d barn=%d cross=%d straight=%d\n",
           signal->det_true,
           signal->road_type,
           signal->rings_flag,
           signal->left_line,
           signal->right_line,
           signal->white_line,
           signal->offline,
           signal->tow_point_true,
           signal->is_left_ring,
           signal->is_right_ring,
           signal->is_barn_in,
           signal->is_cross,
           signal->is_straight);
}
