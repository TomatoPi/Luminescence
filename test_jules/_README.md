## What is an Effect ?

`Effect`s are meant to be composed with one another. An effect is a function that takes some inputs (see **What is an EffectInputs ?**) and outputs a float between 0 and 1. In the process, the `Effect` is expected to make a call to the next `Effect` in the chain, using `APPLY_NEXT_EFFECT()`.

An `Effect` is applied by modifying either the input or the output (or both) of the next `Effect`.

Example : 

```cpp
EFFECT(InvertTime)
{
    in.time = 1.f - in.time; // Modify the input that will be passed to the next effect
    return APPLY_NEXT_EFFECT();
}
```

## What is a FinalEffect ?