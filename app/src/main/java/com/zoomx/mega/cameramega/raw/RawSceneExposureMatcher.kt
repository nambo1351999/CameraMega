package com.zoomx.mega.cameramega.raw

import android.content.Context
import android.graphics.Rect
import com.zoomx.mega.cameramega.camera.AspectRatio
import com.zoomx.mega.cameramega.camera.RawBlackBorderCrop
import com.zoomx.mega.cameramega.utils.PLog

internal object RawSceneExposureMatcher {
    private const val TAG = "RawSceneExposureMatcher"

    suspend fun prepareCaptureProfile(
        renderer: RawDemosaicProcessor,
        context: Context,
        input: RawDngCaptureProfileInput,
        aspectRatio: AspectRatio?,
        cropRegion: Rect?,
        rotation: Int,
        estimateSceneExposure: Boolean,
        generatePhotonPgtm: Boolean,
        statsBounds: Rect?,
        rawBlackPointCorrection: Float = 0f,
        rawWhitePointCorrection: Float = 0f,
        rawAutoWhiteBalanceEstimate: Boolean = false,
        applyLensShadingCorrection: Boolean = true,
        rawBlackBorderCrop: RawBlackBorderCrop = RawBlackBorderCrop(),
        rawNoiseProfileId: String = RawNoiseProfileManager.DEFAULT_PROFILE_ID,
    ): RawDngCaptureProfileResult? {
        val request = if (estimateSceneExposure) {
            RawSceneExposureRequest { frame ->
                val result = RawSceneExposureEstimator.estimate(
                    context = context.applicationContext,
                    frame = frame,
                    metadata = input.metadata,
                    deviceLimits = input.sceneExposureDeviceLimits,
                )
                if (result == null) {
                    PLog.w(TAG, "RAW scene exposure unavailable; automatic offset omitted")
                }
                result
            }
        } else {
            null
        }
        return renderer.prepareCaptureProfile(
            context = context,
            input = input,
            aspectRatio = aspectRatio,
            cropRegion = cropRegion,
            rotation = rotation,
            request = request,
            generatePhotonPgtm = generatePhotonPgtm,
            statsBounds = statsBounds,
            rawBlackPointCorrection = rawBlackPointCorrection,
            rawWhitePointCorrection = rawWhitePointCorrection,
            rawAutoWhiteBalanceEstimate = rawAutoWhiteBalanceEstimate,
            applyLensShadingCorrection = applyLensShadingCorrection,
            rawBlackBorderCrop = rawBlackBorderCrop,
            rawNoiseProfileId = rawNoiseProfileId,
        )
    }
}

internal fun interface RawSceneExposureRequest {
    fun solve(frame: RawSceneLinearFrame): RawSceneExposureSolution?
}
