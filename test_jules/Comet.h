#pragma once

#include "util.h"

struct Comet {
    Comet(int head_idx = 0, int dir = 1, int speed = 1)
        : head_idx{head_idx}, dir{dir}, speed{speed}
    {
    }
    int head_idx;
    int dir;
    int speed;
    int length = 10;

    float eval(int idx)
    {
        int offset = head_idx - dir * idx;
        return 1.f - offset / (float)(max(length - 1, 1));
    }

    void update()
    {
        head_idx += dir * speed;
        int tail_idx = head_idx - dir * (length - 1);
        if ((dir > 0 && tail_idx >= NUM_LEDS) || (dir < 0 && tail_idx < 0)) {
            dir *= -1;
            head_idx += length * dir;
        }
    }
};