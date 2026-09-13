package com.mega.superx.filter.camera.hdr

import android.graphics.Bitmap
import android.graphics.Gainmap

enum class SourceKind {
    RAW,
    SDR_BITMAP,
}

enum class HdrBufferEncoding {
    LINEAR_SRGB,
}

enum class LuminanceGainMapEncoding {
    LINEAR_RATIO,
}

data class LuminanceGainMap(
    val bitmap: Bitmap,
    val encoding: LuminanceGainMapEncoding,
)

data class HdrBuffer(
    val bitmap: Bitmap,
    val encoding: HdrBufferEncoding,
    val description: String? = null,
)

data class GainmapSourceSet(
    val sdrBase: Bitmap,
    
    val lutLuminanceGainMap: LuminanceGainMap? = null,
    val hdrReference: HdrBuffer? = null,
    val sourceKind: SourceKind,
    val confidence: Float = 1.0f,
    val displayHdrSdrRatio: Float = 0f,
)

data class GainmapResult(
    val gainmap: Gainmap,
    val sourceKind: SourceKind,
    val confidence: Float = 1.0f,
)
