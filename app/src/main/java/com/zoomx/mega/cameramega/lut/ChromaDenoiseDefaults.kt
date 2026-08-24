package com.zoomx.mega.cameramega.lut

import com.zoomx.mega.cameramega.processor.DenoiseStrength

object ChromaDenoiseDefaults {
    const val RAW_CAPTURE_DEFAULT_STRENGTH = 0.0f
    private const val MIN_ACTIVE_NOISE_BANDWIDTH = 1.0f
    private const val STRENGTH_ONE_NOISE_BANDWIDTH = 8.0f
    private const val FULL_OUTPUT_STRENGTH_POINT = 0.5f
    private const val EDGE_GUIDANCE_START_POINT = 0.75f

    fun forRawCapture(requested: Float): Float = DenoiseStrength.clamp(
        maxOf(requested, RAW_CAPTURE_DEFAULT_STRENGTH)
    )

    
    fun noiseBandwidth(strength: Float): Float {
        val clamped = DenoiseStrength.clamp(strength)
        if (clamped <= 0f) return 0f
        return MIN_ACTIVE_NOISE_BANDWIDTH +
            clamped * (STRENGTH_ONE_NOISE_BANDWIDTH - MIN_ACTIVE_NOISE_BANDWIDTH)
    }

    
    fun outputStrength(strength: Float): Float {
        val t = (DenoiseStrength.clamp(strength) / FULL_OUTPUT_STRENGTH_POINT)
            .coerceIn(0f, 1f)
        return t * t * (3f - 2f * t)
    }

    
    fun edgeGuidanceRelaxation(strength: Float): Float {
        val t = ((DenoiseStrength.clamp(strength) - EDGE_GUIDANCE_START_POINT) /
            (1f - EDGE_GUIDANCE_START_POINT)).coerceIn(0f, 1f)
        return t * t * (3f - 2f * t)
    }
}
