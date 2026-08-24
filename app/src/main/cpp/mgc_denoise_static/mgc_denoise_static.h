#pragma once

#include <cstdint>

namespace photon::mgc_denoise {

struct DenoiseNoiseBuffers {
    float read[8] = {};
    float shot[8] = {};
    float quadratic[8] = {};
    float outlier_distance[5] = {};
    float revert_factor[5] = {};
};

struct ChromaDenoiseNoiseBuffers {
    
    
    
    float quadratic[24] = {};
    float shot[24] = {};
    float read[24] = {};
    uint8_t outlier_threshold[8] = {};
};

enum class SpatialStrengthInputLayout {
    Bayer = 0,
    Rgb = 1,
};

struct SpatialStrengthResult {
    
    
    
    float output_noise_model_0[3] = {};
    float output_noise_model_1[3] = {};
    float output_weights_sum_total_diag_0[3] = {};
    float output_weights_sum_total_diag_1[3] = {};
};

struct SharpenCurveSelection {
    float lower_snr = 0.0f;
    float upper_snr = 0.0f;
    float interpolation = 0.0f;
    
    float curves[30] = {};
    
    
    float relative_corner_acutance_correction[3] = {};
};

bool BuildNoiseBuffers(
    float read_noise,
    float shot_noise,
    float quadratic_noise,
    const float correlation[128],
    float response_offset,
    float response_cosine_offset,
    const float strength[5],
    const float outlier_distance[5],
    const float revert_factor[5],
    DenoiseNoiseBuffers* output);

bool BuildChromaNoiseBuffers(
    const float read_noise[3],
    float shot_noise,
    float quadratic_noise,
    const float correlation[128],
    float response_offset,
    float response_cosine_offset,
    const float strength[5],
    const float outlier_threshold[5],
    ChromaDenoiseNoiseBuffers* output);

int ComputeStrengthMap(
    const uint16_t* input,
    int width,
    int height,
    int origin_x,
    int origin_y,
    const float* gain_map,
    int gain_width,
    int gain_height,
    float sample_rate_x,
    float sample_rate_y,
    uint16_t* output);

int ComputeSpatialStrengthMap(
    SpatialStrengthInputLayout layout,
    const int16_t* fused_fixed16,
    int width,
    int height,
    int cfa_pattern,
    const float* alignment,
    int alignment_width,
    int alignment_height,
    const uint8_t* rejection,
    int rejection_width,
    int rejection_height,
    int frame_count,
    const float* input_read_noise,
    const float* input_shot_noise,
    const float* frame_weights,
    const float* kernel_sigmas,
    float rejected_denoise_multiplier,
    uint16_t* output_strength_q8,
    SpatialStrengthResult* diagnostics);

int RunRgbRawToYuv(
    const uint16_t* input,
    int width,
    int height,
    int16_t* output);

int RunDefaultBayerRawToYuv(
    const uint16_t* packed_input,
    int width,
    int height,
    int cfa_pattern,
    const float channel_gains[4],
    const float* gain_map,
    int gain_map_width,
    int gain_map_height,
    float gain_map_sample_rate_x,
    float gain_map_sample_rate_y,
    int16_t* output);

bool PrepareDefaultBayerDenoiseNoiseModel(
    const float input_read[3],
    const float input_shot[3],
    const float input_quadratic[3],
    const float input_correlation[128],
    float output_read[3],
    float output_shot[3],
    float output_quadratic[3],
    float output_correlation[128],
    float* correlation_mean);

int RunMeasureMoireS16(
    const int16_t* input,
    int width,
    int height,
    const uint16_t* strength_map,
    int strength_width,
    int strength_height,
    uint16_t* output_strength_map);

int RunChromaDenoise(
    const int16_t* input,
    int width,
    int height,
    const uint16_t* strength_map,
    int strength_width,
    int strength_height,
    const ChromaDenoiseNoiseBuffers& noise,
    int16_t* output);

int RunPecan(
    const uint16_t* strength_map,
    int strength_width,
    int strength_height,
    const DenoiseNoiseBuffers& noise,
    const int16_t* input,
    int width,
    int height,
    int16_t* output);

int RunYuvToRgb(
    const int16_t* input,
    int width,
    int height,
    int16_t* output);

bool BuildDefaultSharpenCurves(
    float snr,
    const float interpolation_scales[3],
    SharpenCurveSelection* output);

int RunSharpenTo16Bit(
    const int16_t* input_yuv,
    int width,
    int height,
    const float curves[30],
    const float relative_corner_acutance_correction[3],
    float sharpen_attenuation_scale,
    uint16_t* output_interleaved_rgb);

}  
