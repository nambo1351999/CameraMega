package com.mega.filter.camera.raw

import android.graphics.Rect
import android.hardware.camera2.CameraCharacteristics
import com.mega.filter.camera.processor.GpuLinearRgbSource
import java.nio.ByteBuffer

data class RawSceneExposureDeviceLimits(
    val maxExposureTimeMs: Float,
    val referenceSensitivityIso: Int,
    val deviceMaxOverallGain: Float,
) {
    val minTetMs: Float
        get() = MIN_TET_MS

    val deviceMaxTetMs: Float
        get() = maxExposureTimeMs * deviceMaxOverallGain

    fun isValid(): Boolean =
        maxExposureTimeMs.isFinite() && maxExposureTimeMs >= MIN_TET_MS &&
            referenceSensitivityIso > 0 &&
            deviceMaxOverallGain.isFinite() && deviceMaxOverallGain >= 1f &&
            deviceMaxTetMs.isFinite() && deviceMaxTetMs >= MIN_TET_MS

    companion object {
        
        const val MIN_TET_MS = 0.04f
        
        const val MAX_CAMERA2_EXPOSURE_TIME_MS = 1_000f

        fun fromCameraCharacteristics(
            characteristics: CameraCharacteristics,
        ): RawSceneExposureDeviceLimits? {
            val exposureRange = characteristics.get(
                CameraCharacteristics.SENSOR_INFO_EXPOSURE_TIME_RANGE,
            ) ?: return null
            val sensitivityRange = characteristics.get(
                CameraCharacteristics.SENSOR_INFO_SENSITIVITY_RANGE,
            ) ?: return null
            return fromCamera2Ranges(
                maxExposureTimeNs = exposureRange.upper,
                minSensitivityIso = sensitivityRange.lower,
                maxSensitivityIso = sensitivityRange.upper,
            )
        }

        fun fromCamera2Ranges(
            maxExposureTimeNs: Long,
            minSensitivityIso: Int,
            maxSensitivityIso: Int,
        ): RawSceneExposureDeviceLimits? {
            if (maxExposureTimeNs <= 0L || minSensitivityIso <= 0 ||
                maxSensitivityIso < minSensitivityIso
            ) {
                return null
            }
            val maxExposureTimeMs = (maxExposureTimeNs.toDouble() / 1_000_000.0)
                .coerceAtMost(MAX_CAMERA2_EXPOSURE_TIME_MS.toDouble())
                .toFloat()
            val deviceMaxOverallGain =
                maxSensitivityIso.toFloat() / minSensitivityIso.toFloat()
            return RawSceneExposureDeviceLimits(
                maxExposureTimeMs = maxExposureTimeMs,
                referenceSensitivityIso = minSensitivityIso,
                deviceMaxOverallGain = deviceMaxOverallGain,
            ).takeIf(RawSceneExposureDeviceLimits::isValid)
        }
    }
}

data class RawDngProfilePreparationOptions(
    val generatePhotonPgtm: Boolean = false,
    val statsBounds: Rect? = null,
    val captureProfilePreparer: RawDngCaptureProfilePreparer? = null,
)

fun interface RawDngCaptureProfilePreparer {
    suspend fun prepare(input: RawDngCaptureProfileInput): RawDngCaptureProfileResult?
}

data class RawDngCaptureProfileInput(
    val rawData: ByteBuffer?,
    val width: Int,
    val height: Int,
    val rowStride: Int,
    val samplesPerPixel: Int,
    val metadata: RawMetadata,
    val meteringRenderPlan: DcpRenderPlan,
    val gpuLinearRgbSource: GpuLinearRgbSource? = null,
    val sceneExposureDeviceLimits: RawSceneExposureDeviceLimits? = null,
)

data class RawDngCaptureProfileResult(
    val exposureOffsetEv: Float?,
    val profileGainTableMap: DngProfileGainTableMap?,
)

internal fun DcpRenderPlan.toAdobeDefaultMeteringPlan(): DcpRenderPlan {
    return copy(
        defaultBlackRender = DcpDefaultBlackRender.None,
        hueSatMap = null,
        lookTable = null,
        toneCurveLut = null,
    )
}

data class RawDngProfilePreparation(
    val baselineExposureEv: Float,
    val profileGainTableMap: DngProfileGainTableMap?,
)
