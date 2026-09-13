package com.mega.superx.filter.camera.raw

import com.mega.superx.filter.camera.processor.DenoiseStrength

object RawDenoiseDefaults {
    const val RAW_LUMA_STRENGTH = 0.4f
    const val RAW_CHROMA_STRENGTH = 1.0f
    const val RAW_MAX_LUMA_STRENGTH = 1.0f
    const val RAW_MAX_CHROMA_STRENGTH = 1.0f

    fun normalize(value: Float): Float = DenoiseStrength.clamp(value)
}
