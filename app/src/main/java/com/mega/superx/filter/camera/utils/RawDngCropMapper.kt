package com.mega.superx.filter.camera.utils

import kotlin.math.roundToInt

internal data class RawCropRect(
    val left: Int,
    val top: Int,
    val right: Int,
    val bottom: Int,
) {
    val width: Int get() = right - left
    val height: Int get() = bottom - top

    fun isValid(): Boolean = width > 0 && height > 0
}

internal object RawDngCropMapper {
    fun mapToDefaultCrop(
        preCorrectionActiveArray: RawCropRect,
        postCorrectionActiveArray: RawCropRect?,
        scalerCropRegion: RawCropRect?,
        zoomRatio: Float,
        usePreCorrectionCoordinateSystem: Boolean,
        targetWidth: Int,
        targetHeight: Int,
    ): RawCropRect {
        require(preCorrectionActiveArray.isValid())
        require(targetWidth > 0 && targetHeight > 0)

        val coordinateDomain = if (usePreCorrectionCoordinateSystem) {
            preCorrectionActiveArray
        } else {
            postCorrectionActiveArray?.takeIf { it.isValid() } ?: preCorrectionActiveArray
        }
        val localDomain = RawCropRect(0, 0, coordinateDomain.width, coordinateDomain.height)
        val localCrop = localizeCrop(scalerCropRegion, coordinateDomain) ?: localDomain
        val effectiveZoom = zoomRatio.takeIf { it.isFinite() && it >= 1f }?.toDouble() ?: 1.0
        val inverseZoom = 1.0 / effectiveZoom
        val zoomOriginX = (1.0 - inverseZoom) * 0.5
        val zoomOriginY = (1.0 - inverseZoom) * 0.5

        fun mapX(value: Int): Int {
            val localNormalized = value.toDouble() / localDomain.width.toDouble()
            return ((zoomOriginX + localNormalized * inverseZoom) * targetWidth)
                .roundToInt()
                .coerceIn(0, targetWidth)
        }

        fun mapY(value: Int): Int {
            val localNormalized = value.toDouble() / localDomain.height.toDouble()
            return ((zoomOriginY + localNormalized * inverseZoom) * targetHeight)
                .roundToInt()
                .coerceIn(0, targetHeight)
        }

        val left = mapX(localCrop.left)
        val top = mapY(localCrop.top)
        val right = mapX(localCrop.right).coerceAtLeast(left + 1).coerceAtMost(targetWidth)
        val bottom = mapY(localCrop.bottom).coerceAtLeast(top + 1).coerceAtMost(targetHeight)
        return RawCropRect(
            left = left.coerceAtMost(right - 1),
            top = top.coerceAtMost(bottom - 1),
            right = right,
            bottom = bottom,
        )
    }

    private fun localizeCrop(crop: RawCropRect?, domain: RawCropRect): RawCropRect? {
        if (crop == null) return null
        require(crop.isValid()) { "SCALER_CROP_REGION must be non-empty: $crop" }
        val localBounds = RawCropRect(0, 0, domain.width, domain.height)
        require(localBounds.contains(crop)) {
            "SCALER_CROP_REGION $crop is outside Camera2 coordinate bounds $localBounds"
        }
        return crop
    }

    private fun RawCropRect.contains(other: RawCropRect): Boolean {
        return other.left >= left && other.top >= top &&
            other.right <= right && other.bottom <= bottom
    }
}
