package com.mega.filter.camera.raw

import kotlin.math.max

internal data class DenoiseProfileOffset(
    val x: Int,
    val y: Int,
)

internal data class DenoiseProfileWeightTuning(
    val expectedFineDistance: Float,
    val expectedGuideDistance: Float,
    val inverseBandwidth: Float,
    val coarseGuideWeight: Float,
)

internal object DenoiseProfileNlmConfig {
    private const val COLOR_CHANNEL_COUNT = 3
    private const val DIFFERENCE_VARIANCE = 2.0f

    
    private const val GUIDE_FILTER_ENERGY = 36.0f / 256.0f

    
    
    
    const val COARSE_GUIDE_WEIGHT = 8.0f

    val searchOffsets: List<DenoiseProfileOffset> = buildSearchOffsets(
        DenoiseProfileShaders.SEARCH_RADIUS
    )

    fun buildSearchOffsets(radius: Int): List<DenoiseProfileOffset> {
        require(radius >= 0) { "radius must be non-negative" }
        return buildList {
            for (qy in -radius..0) {
                
                
                val qxEnd = if (qy == 0) 0 else radius
                for (qx in -radius..qxEnd) {
                    add(DenoiseProfileOffset(qx, qy))
                }
            }
        }
    }

    fun weightTuning(patchRadius: Int): DenoiseProfileWeightTuning {
        require(patchRadius >= 0) { "patchRadius must be non-negative" }
        val patchWidth = 2 * patchRadius + 1
        val patchPixels = patchWidth * patchWidth
        val expectedFineDistance =
            DIFFERENCE_VARIANCE * COLOR_CHANNEL_COUNT * patchPixels.toFloat()
        val expectedGuideDistance =
            DIFFERENCE_VARIANCE * GUIDE_FILTER_ENERGY * patchPixels.toFloat()

        return DenoiseProfileWeightTuning(
            expectedFineDistance = expectedFineDistance,
            expectedGuideDistance = expectedGuideDistance,
            
            inverseBandwidth = 1.0f / max(expectedFineDistance, 1.0f),
            coarseGuideWeight = COARSE_GUIDE_WEIGHT,
        )
    }
}
