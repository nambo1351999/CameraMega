package com.zoomx.mega.cameramega.hdr

interface GainmapProducer {
    suspend fun build(
        source: GainmapSourceSet,
        strength: Float = HdrGainmapStrength.DEFAULT
    ): GainmapResult?
}
