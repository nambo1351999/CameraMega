package com.mega.filter.camera.lut.creator

import androidx.annotation.Keep

@Keep
data class LutRecipe(
    val controlPoints: List<ControlPoint> = emptyList(),
    val isMonochrome: Boolean = false
)

@Keep
data class ControlPoint(
    val sourceR: Float,
    val sourceG: Float,
    val sourceB: Float,
    val targetR: Float,
    val targetG: Float,
    val targetB: Float,
    val matchConfidence: Float = 1f
)
