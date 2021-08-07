#pragma once
#include "vec3.h"

// From https://iquilezles.org/www/articles/palettes/palettes.htm
struct ColorPalette {
    vec3 eval(float t) const { return a + b * cos((c * t + d) * 6.28f); }

    vec3 a;
    vec3 b;
    vec3 c;
    vec3 d;
};

static const ColorPalette palette_rainbow = {
    {0.5f, 0.5f, 0.5f},
    {0.5f, 0.5f, 0.5f},
    {1.f, 1.f, 1.f},
    {0.f, 0.33f, 0.67f}};

static const ColorPalette palette_deep = {
    {0.5f, 0.5f, 0.5f},
    {0.5f, 0.5f, 0.5f},
    {2.f, 1.f, 0.f},
    {0.5f, 0.2f, 0.25f}};

static const ColorPalette palette_coper = {
    {0.8f, 0.5f, 0.4f},
    {0.2f, 0.4f, 0.2f},
    {2.f, 1.f, 1.f},
    {0.f, 0.25f, 0.25f}};

static const ColorPalette palette_yellow_blue = {
    {0.5f, 0.5f, 0.5f},
    {0.5f, 0.5f, 0.5f},
    {1.f, 1.f, 0.5f},
    {0.8f, 0.9f, 0.3f}};