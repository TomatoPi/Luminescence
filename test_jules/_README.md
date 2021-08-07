## API

### What is an Effect ?

`Effect`s are meant to be composed with one another. An effect is a function that takes some inputs (see *What is an EffectInput ?*) and outputs a color represented by a `vec3` whose components range between 0 and 1. In the process, the `Effect` is expected to make a call to the next `Effect` in the chain, using `APPLY_NEXT_EFFECT()`.

An `Effect` is applied by modifying either the input or the output (or both) of the next `Effect`.

Examples : 

```cpp
EFFECT(InvertTime)
{
    // Modify the input that will be passed to the next effect
    in.time = 1.f - in.time;
    return APPLY_NEXT_EFFECT();
}
```

```cpp
EFFECT(SinusoidalBlink)
{
    // Modify the output of the next effect
    return APPLY_NEXT_EFFECT() * (sin(in.time * 6.28) * 0.5 + 0.5);
}
```

### What is a FinalEffect ?

A `FinalEffect` is an `Effect` that is at the end of the chain. It shall not call `APPLY_NEXT_EFFECT()`, but instead generate a `vec3` only from the `EffectInput`.

Example : 

```cpp
FINAL_EFFECT(ScrollingRainbow)
{
    return palette_rainbow.eval(in.pos + in.time);
}
```

### What is an EffectInput ?

It is the common input to all `Effect`s and `FinalEffect`s. It is made of :
- `pos` : the relative position of the led on the band, represented as a number between 0 and 1
- `time` : a number between 0 and 1 that increases over time

**Effects that modify the inputs shall respect the invariant that the numbers are in the [0, 1] range !**

## Implementation

An `Effect` is a function with the signature `(EffectInput, int) -> vec3` (the int is an index indicating which effect should be applied next ; this is detailed in the `EffectsMixer` section). More precisely they are function pointers, which allows us to have polymorphic behaviour without requiring dynamic allocations [^1]

[^1]: NB : we don't use `std::function` because it doesn't seem to be supported by Arduino, and we can only afford function pointers anyways (some stateful lambdas could require dynamic allocations).

The `EffectsMixer` contains an array of `Effect`s  that will be applied in order (called `layers`), and a `FinalEffect` that will be applied at the end.

To remove the need for the effects to have function pointers to each other, we instead use an index into `layers`. Each effect receives an `EffectInput` and an index `idx` and knows that it has to call the effect `layers[idx]` (or the `FinalEffect` if `idx == -1`). This machinery is handled by the `EffectsMixer::eval(EffectInput in, int next_layer_idx)` method.

We also use macros (😱) to simplify the declarations of the effects and the call to `EffectsMixer::eval`. (This effectively hides the fact that we need an `idx`, which is both easier on the eye and easier to refactor if need be).

```cpp
#define FINAL_EFFECT(fn_name) vec3(fn_name)(EffectInput in)
#define EFFECT(fn_name)       vec3(fn_name)(EffectInput in, int idx)
#define APPLY_NEXT_EFFECT() EffectsMixer::eval(in, idx)
```

This allows us to hide implementation details and only keep the meaningful parts :
```cpp
EFFECT(Identity)
{
    return APPLY_NEXT_EFFECT();
}
```