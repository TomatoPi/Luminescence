#include "Generators.h"

Generator     Generators::base;
MetaGenerator Generators::layers[4];

void Generators::Init()
{
    base      = &BScrollingGradient;
    layers[0] = &MIdentity;
    layers[1] = &MIdentity;
    layers[2] = &MIdentity;
    layers[3] = &MIdentity;
}

float BScrollingGradient(GeneratorInput in)
{
    return in.pos + in.time;
}

float MIdentity(GeneratorInput in, int idx)
{
    return Generators::eval(in, idx);
}

float MBlink(GeneratorInput in, int idx)
{
    in.time = in.time < 0.5f ? 0.f : 0.5f;
    return Generators::eval(in, idx);
}