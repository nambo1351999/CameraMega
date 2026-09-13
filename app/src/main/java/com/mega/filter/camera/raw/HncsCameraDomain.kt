package com.mega.filter.camera.raw

import kotlin.math.max

internal data class HncsCameraDomainContract(
    val normalizedGain: FloatArray,
    val inputEv: Float,
    val hrTrunc: Float,
    val hrMax: Float,
    val cameraToWorkingMatrix: FloatArray,
) {
    init {
        require(normalizedGain.size == 3 && normalizedGain.all { it.isFinite() && it > 0f })
        require(inputEv.isFinite() && inputEv > 0f)
        require(hrTrunc.isFinite() && hrTrunc > 0f)
        require(hrMax.isFinite() && hrMax > 0f)
        require(cameraToWorkingMatrix.size == 9 && cameraToWorkingMatrix.all(Float::isFinite))
    }

    internal fun applyCameraDomain(rgb: FloatArray): FloatArray {
        require(rgb.size >= 3)
        return FloatArray(3) { channel ->
            val gained = rgb[channel] * normalizedGain[channel] / hrTrunc
            gained.coerceIn(0f, hrMax) * inputEv
        }
    }
}

internal object HncsCameraDomain {
    const val DEFAULT_HR_TRUNC = 1f
    const val DEFAULT_HR_MAX = 1f

    private const val MIN_CAMERA_GAIN = 1e-3f
    private const val MAX_CAMERA_GAIN = 64f

    
    fun composeWhiteBalancedCameraMatrix(
        whiteBalancedCameraToWorkingMatrix: FloatArray,
        cameraGains: FloatArray,
    ): FloatArray {
        require(
            whiteBalancedCameraToWorkingMatrix.size == 9 &&
                whiteBalancedCameraToWorkingMatrix.all(Float::isFinite)
        )
        val gains = canonicalizeCameraGains(cameraGains)
        return whiteBalancedCameraToWorkingMatrix.copyOf().also { composite ->
            for (row in 0 until 3) {
                for (column in 0 until 3) {
                    composite[row * 3 + column] *= gains[column]
                }
            }
        }
    }

    
    fun resolve(
        compositeCameraToWorkingMatrix: FloatArray,
        cameraGains: FloatArray,
        baselineExposureEv: Float,
        additionalExposureEv: Float = 0f,
    ): HncsCameraDomainContract {
        require(
            compositeCameraToWorkingMatrix.size == 9 &&
                compositeCameraToWorkingMatrix.all(Float::isFinite)
        )
        val gains = canonicalizeCameraGains(cameraGains)
        val maximumGain = max(gains[0], max(gains[1], gains[2]))
        val normalizedGain = FloatArray(3) { channel ->
            gains[channel] / maximumGain
        }
        val cameraToWorkingMatrix = compositeCameraToWorkingMatrix.copyOf()
        for (row in 0 until 3) {
            for (column in 0 until 3) {
                cameraToWorkingMatrix[row * 3 + column] /= gains[column]
            }
        }
        val exposureGain = DngBaselineExposure.exactGain(
            DngBaselineExposure.sanitize(baselineExposureEv) +
                DngBaselineExposure.sanitize(additionalExposureEv)
        )
        return HncsCameraDomainContract(
            normalizedGain = normalizedGain,
            inputEv = exposureGain * maximumGain,
            hrTrunc = DEFAULT_HR_TRUNC,
            hrMax = DEFAULT_HR_MAX,
            cameraToWorkingMatrix = cameraToWorkingMatrix,
        )
    }

    
    fun fromWhiteBalanceGains(whiteBalanceGains: FloatArray): FloatArray {
        fun component(index: Int, fallback: Float): Float {
            val value = whiteBalanceGains.getOrElse(index) { fallback }
            return value.takeIf { it.isFinite() && it > 0f } ?: fallback
        }

        val greenEven = component(1, 1f)
        val greenOdd = component(2, greenEven)
        val green = (greenEven + greenOdd) * 0.5f
        return canonicalizeCameraGains(
            floatArrayOf(
                component(0, green),
                green,
                component(3, green),
            )
        )
    }

    fun canonicalizeCameraGains(cameraGains: FloatArray): FloatArray =
        sanitizeCameraGains(cameraGains)

    private fun sanitizeCameraGains(cameraGains: FloatArray): FloatArray {
        return FloatArray(3) { channel ->
            cameraGains.getOrElse(channel) { 1f }
                .takeIf { it.isFinite() && it > 0f }
                ?.coerceIn(MIN_CAMERA_GAIN, MAX_CAMERA_GAIN)
                ?: 1f
        }
    }
}
