package com.zoomx.mega.cameramega.raw

import com.zoomx.mega.cameramega.processor.DenoiseStrength

object RawDenoiseDefaults {
    const val RAW_LUMA_STRENGTH = 0.4f
    const val RAW_CHROMA_STRENGTH = 1.0f
    const val RAW_MAX_LUMA_STRENGTH = 1.0f
    const val RAW_MAX_CHROMA_STRENGTH = 1.0f

    fun normalize(value: Float): Float = DenoiseStrength.clamp(value)
}
