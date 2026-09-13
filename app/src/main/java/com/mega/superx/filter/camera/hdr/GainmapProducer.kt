package com.mega.superx.filter.camera.hdr

interface GainmapProducer {
    suspend fun build(
        source: GainmapSourceSet,
        strength: Float = HdrGainmapStrength.DEFAULT
    ): GainmapResult?
}
