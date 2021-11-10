#pragma once

#include <stdint.h>

#define MAX_RIBBONS_COUNT 8
#define PRESETS_COUNT 8
#define PALETTES_COUNT 8

struct state_t {

  struct palette_t {
    struct params_t {
      uint8_t min_value;
      uint8_t max_value;
      uint8_t frequency_times_60;
      uint8_t phase;
    } params[3];
  } palettes[PALETTES_COUNT];

  struct setup_t {
    uint8_t ribbons_count;
    uint8_t ribbons_lengths[MAX_RIBBONS_COUNT];
  } setup;

  struct triggers_t {
    uint8_t save;
    uint8_t load;
    uint8_t next_preset;
    uint8_t prev_preset;

    uint8_t reset_bpm;
    uint8_t correct_bpm;
    uint8_t sync_left;
    uint8_t sync_right;

  } triggers;

  struct master_t {
    float   bpm;
    uint8_t sync_correction;

    uint8_t brightness;
    uint8_t strobe_speed;

    uint8_t blur_enable;
    uint8_t blur_qty;
    
  } master;

  struct preset_t {
    uint8_t palette;

    uint8_t colormod_enable;
    uint8_t colormod_osc;
    uint8_t colormod_width;
    uint8_t colormod_move;
    
    uint8_t maskmod_enable;
    uint8_t maskmod_osc;
    uint8_t maskmod_width;
    uint8_t maskmod_move;

    uint8_t slicer_nslices;
    uint8_t slicer_useuneven;
    uint8_t slicer_nuneven;
    uint8_t slicer_mergeribbon;
    uint8_t slicer_useflip;

    uint8_t feedback_enable;
    uint8_t feedback_qty;

    uint8_t strobe_enable;

    uint8_t speed_scale;

    uint8_t brightness;

  } presets[PRESETS_COUNT];
};
