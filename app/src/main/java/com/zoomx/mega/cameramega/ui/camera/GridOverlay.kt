package com.zoomx.mega.cameramega.ui.camera

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color

@Composable
fun GridOverlay(
    aspectRatio: Float,
    modifier: Modifier = Modifier
) {
    Canvas(modifier = modifier.fillMaxSize()) {
        val canvasWidth = size.width
        val canvasHeight = size.height
        
        
        val containerRatio = 3f / 4f
        
        
        val targetRatio = aspectRatio  
        
        
        val (drawWidth, drawHeight, offsetX, offsetY) = if (targetRatio > containerRatio) {
            
            val h = canvasWidth / targetRatio
            Quadruple(canvasWidth, h, 0f, (canvasHeight - h) / 2f)
        } else {
            
            val w = canvasHeight * targetRatio
            Quadruple(w, canvasHeight, (canvasWidth - w) / 2f, 0f)
        }
        
        
        val gridColor = Color.White.copy(alpha = 0.5f)
        val strokeWidth = 1.5f
        
        
        for (i in 1..2) {
            val x = offsetX + drawWidth * i / 3f
            drawLine(
                color = gridColor,
                start = androidx.compose.ui.geometry.Offset(x, offsetY),
                end = androidx.compose.ui.geometry.Offset(x, offsetY + drawHeight),
                strokeWidth = strokeWidth
            )
        }
        
        
        for (i in 1..2) {
            val y = offsetY + drawHeight * i / 3f
            drawLine(
                color = gridColor,
                start = androidx.compose.ui.geometry.Offset(offsetX, y),
                end = androidx.compose.ui.geometry.Offset(offsetX + drawWidth, y),
                strokeWidth = strokeWidth
            )
        }
    }
}
