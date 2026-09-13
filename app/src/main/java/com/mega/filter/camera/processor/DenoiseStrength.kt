package com.mega.filter.camera.processor

object DenoiseStrength {
    const val MIN_VALUE = 0.0f
    const val MAX_VALUE = 2.0f

    val valueRange: ClosedFloatingPointRange<Float>
        get() = MIN_VALUE..MAX_VALUE

    fun clamp(value: Float?): Float = value
        ?.takeIf { it.isFinite() }
        ?.coerceIn(MIN_VALUE, MAX_VALUE)
        ?: MIN_VALUE

    
    fun noiseVarianceScale(value: Float): Float {
        val sigmaScale = clamp(value).coerceAtLeast(1.0f)
        return sigmaScale * sigmaScale
    }

    fun outputMix(value: Float): Float = clamp(value).coerceAtMost(1.0f)
}
