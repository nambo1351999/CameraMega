package com.zoomx.mega.cameramega.lut

import com.zoomx.mega.cameramega.color.TransferCurve
import com.zoomx.mega.cameramega.model.ColorRecipeParams
import com.zoomx.mega.cameramega.raw.ColorSpace
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer

data class LutConfig(
    val size: Int,
    val data: FloatArray? = null,
    val byteBuffer: ByteBuffer? = null,
    val title: String = "",
    val configDataType: Int = CONFIG_DATA_TYPE_UINT8,
    val curve: TransferCurve = TransferCurve.SRGB,
    val colorSpace: ColorSpace = ColorSpace.SRGB,
    val outputColorSpace: android.graphics.ColorSpace = android.graphics.ColorSpace.get(android.graphics.ColorSpace.Named.SRGB),
) {
    companion object {
        const val CONFIG_DATA_TYPE_UINT8 = 0
        const val CONFIG_DATA_TYPE_UINT16 = 1
    }

    
    fun toFloatBuffer(): FloatBuffer {
        if (data != null) {
            return FloatBuffer.wrap(data)
        }

        val buffer = byteBuffer ?: throw IllegalStateException("No data available in LutConfig")
        val fb = FloatBuffer.allocate(size * size * size * 3)
        buffer.position(0)

        if (configDataType == CONFIG_DATA_TYPE_UINT16) {
            val shortBuffer = buffer.asShortBuffer()
            while (shortBuffer.hasRemaining()) {
                fb.put((shortBuffer.get().toInt() and 0xFFFF) / 65535f)
            }
        } else {
            while (buffer.hasRemaining()) {
                fb.put((buffer.get().toInt() and 0xFF) / 255f)
            }
        }
        fb.position(0)
        return fb
    }

    
    fun toByteBuffer(): ByteBuffer {
        if (byteBuffer != null) {
            
            return byteBuffer.duplicate().apply { position(0) }
        }

        val floatData = data ?: throw IllegalStateException("No data available in LutConfig")
        val buffer: ByteBuffer
        if (configDataType == CONFIG_DATA_TYPE_UINT16) {
            buffer = ByteBuffer.allocateDirect(floatData.size * 2)
                .order(ByteOrder.nativeOrder())
            val shortBuffer = buffer.asShortBuffer()
            for (f in floatData) {
                shortBuffer.put((f.coerceIn(0f, 1f) * 65535f + 0.5f).toInt().toShort())
            }
        } else {
            buffer = ByteBuffer.allocateDirect(floatData.size)
                .order(ByteOrder.nativeOrder())
            for (f in floatData) {
                buffer.put((f.coerceIn(0f, 1f) * 255f + 0.5f).toInt().toByte())
            }
        }
        buffer.position(0)
        return buffer
    }

    
    fun isValid(): Boolean {
        val count = size * size * size * 3
        val expectedCapacity = if (configDataType == CONFIG_DATA_TYPE_UINT16) count * 2 else count
        return size > 0 && (data?.size == count || byteBuffer?.capacity() == expectedCapacity)
    }
}

data class LutInfo(
    val id: String,
    val nameMap: Map<String, String>, 
    val fileName: String,
    val isBuiltIn: Boolean = true,
    val isDefault: Boolean = false, 
    val isVip: Boolean = false, 
    val category: String = "", 
    val isFavorite: Boolean = false, 
) {
    
    fun getName(locale: java.util.Locale = java.util.Locale.getDefault()): String {
        val language = if (locale.language == "zh") "zh" else "en"
        return nameMap[language] ?: nameMap["en"] ?: id
    }
}
