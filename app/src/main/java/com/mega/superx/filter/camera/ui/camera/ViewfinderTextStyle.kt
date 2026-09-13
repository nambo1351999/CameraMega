package com.mega.superx.filter.camera.ui.camera

import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Shadow

internal val ViewfinderTextShadow = Shadow(
    color = Color.Black.copy(alpha = 0.9f),
    offset = Offset.Zero,
    blurRadius = 3f
)
