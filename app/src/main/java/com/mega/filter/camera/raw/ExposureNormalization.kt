package com.mega.filter.camera.raw

import kotlin.math.pow

object ExposureNormalization {
    private const val TAG = "ExposureNormalization"

    fun compute(metadata: RawMetadata): Float {
        var gain = 1f
        val baselineExposure = DngBaselineExposure.sanitize(metadata.baselineExposure)
        if (baselineExposure != 0f) {
            gain *= 2.0f.pow(baselineExposure)
        }
        return gain.coerceIn(1f, 8f)
    }
}
