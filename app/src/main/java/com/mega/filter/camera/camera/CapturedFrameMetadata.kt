package com.mega.filter.camera.camera

enum class MultiFrameCaptureRole {
    BASE,
    SHORT,
    LONG,
}

internal object RawExposureMath {
    fun product(exposureTimeNs: Long, sensitivityIso: Int): Double {
        require(exposureTimeNs > 0L) { "RAW exposure time must be positive" }
        require(sensitivityIso > 0) { "RAW sensitivity must be positive" }
        return exposureTimeNs.toDouble() * sensitivityIso.toDouble()
    }

    fun productOrNull(exposureTimeNs: Long?, sensitivityIso: Int?): Double? {
        val exposure = exposureTimeNs?.takeIf { it > 0L } ?: return null
        val sensitivity = sensitivityIso?.takeIf { it > 0 } ?: return null
        return product(exposure, sensitivity)
    }
}

data class CapturedFrameMetadata(
    val sensorTimestampNs: Long,
    val frameNumber: Long,
    val exposureTimeNs: Long,
    val sensitivityIso: Int,
    val exposureProduct: Double,
    val focusDistanceDiopters: Float,
    val lensState: Int?,
    val rollingShutterSkewNs: Long?,
    val gyroWindow: GyroExposureWindow?,
    val channelNoiseProfile: FloatArray? = null,
    val multiFrameCaptureRole: MultiFrameCaptureRole? = null,
    
    val desiredExposureProduct: Double? = null,
    
    val dynamicBlackLevelByCfaPosition: FloatArray? = null,
)
