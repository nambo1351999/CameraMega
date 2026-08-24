package com.zoomx.mega.cameramega.ml

import android.graphics.Bitmap

data class RelativeDepthMap(
    val width: Int,
    val height: Int,
    val values: FloatArray,
) {
    init {
        require(width > 0 && height > 0) { "Depth dimensions must be positive" }
        require(values.size.toLong() == width.toLong() * height.toLong()) {
            "Depth data size does not match dimensions"
        }
    }

    companion object {
        
        fun fromBitmap(bitmap: Bitmap): RelativeDepthMap {
            val pixels = IntArray(bitmap.width * bitmap.height)
            bitmap.getPixels(pixels, 0, bitmap.width, 0, 0, bitmap.width, bitmap.height)
            val values = FloatArray(pixels.size)
            for (index in pixels.indices) {
                values[index] = ((pixels[index] shr 16) and 0xff) / 255.0f
            }
            return RelativeDepthMap(bitmap.width, bitmap.height, values)
        }
    }
}
