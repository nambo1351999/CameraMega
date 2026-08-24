package com.zoomx.mega.cameramega.lut

import android.graphics.Bitmap
import coil.size.Size
import coil.transform.Transformation
import android.content.Context
import com.zoomx.mega.cameramega.gallery.MediaMetadata
import com.zoomx.mega.cameramega.gallery.PhotoProcessor
import com.zoomx.mega.cameramega.raw.HncsFilmCurveMode
import com.zoomx.mega.cameramega.raw.HncsRenderIntent
import com.zoomx.mega.cameramega.raw.RawRenderingEngine
import com.zoomx.mega.cameramega.raw.RawToneMappingParameters

class PhotoTransformation(
    private val context: Context,
    private val metadata: MediaMetadata,
    private val photoProcessor: PhotoProcessor,
) : Transformation {
    
    override val cacheKey: String = "photo_${metadata.thumbnailTransformCacheKey()}"

    override suspend fun transform(input: Bitmap, size: Size): Bitmap {
        return photoProcessor.processBitmap(
            context,
            null,
            input,
            metadata.copy(sharpening = 0f, noiseReduction = 0f, chromaNoiseReduction = 0f),
            0f,
            0f,
            0f
        )
    }
}

private fun MediaMetadata.thumbnailTransformCacheKey(): Int {
    return copy(
        sharpening = null,
        noiseReduction = null,
        chromaNoiseReduction = null,
        rawDenoiseValue = null,
        rawChromaDenoiseValue = null,
        rawExposureCompensation = null,
        rawAutoExposure = null,
        rawHighlightsAdjustment = null,
        rawShadowsAdjustment = null,
        rawBlackPointCorrection = null,
        rawWhitePointCorrection = null,
        rawAutoWhiteBalanceEstimate = null,
        rawDcpId = null,
        rawHncsProfileId = null,
        rawHncsRenderIntent = HncsRenderIntent.Standard,
        rawHncsFilmCurveMode = HncsFilmCurveMode.Standard,
        rawRenderingEngine = RawRenderingEngine.AdobeCurve,
        rawToneMappingParameters = RawToneMappingParameters.DEFAULT,
        rawBlackLevelMode = null,
        rawCustomBlackLevel = null,
        rawWhiteLevelMode = null,
        rawCustomWhiteLevel = null,
        rawCfaCorrectionMode = null,
        cameraId = null,
        sourceUri = null,
        exportedUris = emptyList(),
        hasAiDenoisedBase = false,
        aiDenoiseStrength = null
    ).hashCode()
}
