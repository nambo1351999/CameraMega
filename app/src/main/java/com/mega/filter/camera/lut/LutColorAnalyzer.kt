package com.mega.filter.camera.lut

import android.graphics.Color
import androidx.core.graphics.ColorUtils

object LutColorAnalyzer {

    
    fun analyzeTendency(lutConfig: LutConfig): FloatArray {
        val buffer = lutConfig.toFloatBuffer()
        val size = lutConfig.size
        val count = size * size * size
        
        var rSum = 0f
        var gSum = 0f
        var bSum = 0f

        buffer.position(0)
        for (i in 0 until count) {
            rSum += buffer.get()
            gSum += buffer.get()
            bSum += buffer.get()
        }

        
        val avgR = rSum / count
        val avgG = gSum / count
        val avgB = bSum / count

        
        return floatArrayOf(avgR, avgG, avgB)
    }

    
    fun calculateSuitability(targetColor: Int, tendency: FloatArray): Float {
        val targetHsl = FloatArray(3)
        ColorUtils.colorToHSL(targetColor, targetHsl)

        val tendencyRgb = IntArray(3) { i -> (tendency[i] * 255f).toInt().coerceIn(0, 255) }
        val tendencyHsl = FloatArray(3)
        ColorUtils.RGBToHSL(tendencyRgb[0], tendencyRgb[1], tendencyRgb[2], tendencyHsl)

        
        val hueDiff = Math.abs(targetHsl[0] - tendencyHsl[0])
        val hueScore = 1.0f - (Math.min(hueDiff, 360f - hueDiff) / 180f)

        
        val saturationScore = tendencyHsl[1]

        
        return hueScore * 0.8f + saturationScore * 0.2f
    }
}
