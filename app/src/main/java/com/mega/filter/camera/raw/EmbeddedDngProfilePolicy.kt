package com.mega.filter.camera.raw

internal data class EmbeddedDngProfileDecision(
    val hasEmbeddedProfile: Boolean,
    val applyEmbeddedProfile: Boolean,
) {
    fun shouldRetainEmbeddedPgtm(): Boolean {
        return applyEmbeddedProfile
    }

    fun shouldGeneratePhotonPgtm(photonHdrRequested: Boolean): Boolean {
        return photonHdrRequested && !applyEmbeddedProfile
    }
}

internal object EmbeddedDngProfilePolicy {
    fun resolve(
        hasEmbeddedProfile: Boolean,
        colorEngine: RawRenderingEngine,
        profileToneMapMode: RawProfileToneMapMode,
        hasDcpSelection: Boolean,
    ): EmbeddedDngProfileDecision {
        val applyEmbeddedProfile = hasEmbeddedProfile &&
            colorEngine == RawRenderingEngine.AdobeCurve &&
            profileToneMapMode == RawProfileToneMapMode.Default &&
            !hasDcpSelection
        return EmbeddedDngProfileDecision(
            hasEmbeddedProfile = hasEmbeddedProfile,
            applyEmbeddedProfile = applyEmbeddedProfile,
        )
    }
}
