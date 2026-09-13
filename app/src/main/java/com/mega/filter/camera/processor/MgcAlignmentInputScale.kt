package com.mega.filter.camera.processor

internal object MgcAlignmentInputScale {
    const val S16_DOMAIN_SCALE = 16384f

    fun compute(frameGain: Float, staticMetadataWhiteLevel: Float): Float {
        require(frameGain.isFinite() && frameGain > 0f)
        require(staticMetadataWhiteLevel.isFinite() && staticMetadataWhiteLevel >= 0f)
        return frameGain * S16_DOMAIN_SCALE / (staticMetadataWhiteLevel + 1f)
    }
}
