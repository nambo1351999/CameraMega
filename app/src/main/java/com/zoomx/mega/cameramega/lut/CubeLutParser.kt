package com.zoomx.mega.cameramega.lut

import com.zoomx.mega.cameramega.utils.BoundedTextLineReader
import com.zoomx.mega.cameramega.utils.PLog
import java.io.InputStream

object CubeLutParser {

    private const val TAG = "CubeLutParser"

    
    private const val KEYWORD_TITLE = "TITLE"
    private const val KEYWORD_LUT_3D_SIZE = "LUT_3D_SIZE"
    private const val KEYWORD_DOMAIN_MIN = "DOMAIN_MIN"
    private const val KEYWORD_DOMAIN_MAX = "DOMAIN_MAX"

    
    fun parse(inputStream: InputStream): LutConfig {
        var title = ""
        var size = 0
        var domainMin = floatArrayOf(0f, 0f, 0f)
        var domainMax = floatArrayOf(1f, 1f, 1f)
        var data: FloatArray? = null
        var dataIndex = 0

        
        val tempDataList = mutableListOf<FloatArray>()

        inputStream.bufferedReader().use { reader ->
            BoundedTextLineReader.forEachLine(reader) { line ->
                val trimmedLine = line.trim()

                
                if (trimmedLine.isEmpty() || trimmedLine.startsWith("#")) {
                    return@forEachLine
                }

                when {
                    
                    trimmedLine.startsWith(KEYWORD_TITLE) -> {
                        title = extractQuotedString(trimmedLine)
                    }

                    
                    trimmedLine.startsWith(KEYWORD_LUT_3D_SIZE) -> {
                        size = trimmedLine.substringAfter(KEYWORD_LUT_3D_SIZE).trim().toIntOrNull() ?: 0
                        if (size > 0) {
                            
                            data = FloatArray(size * size * size * 3)

                            
                            for (values in tempDataList) {
                                data!![dataIndex++] = normalizeValue(values[0], domainMin[0], domainMax[0])
                                data!![dataIndex++] = normalizeValue(values[1], domainMin[1], domainMax[1])
                                data!![dataIndex++] = normalizeValue(values[2], domainMin[2], domainMax[2])
                            }
                            tempDataList.clear()  
                        }
                    }

                    
                    trimmedLine.startsWith(KEYWORD_DOMAIN_MIN) -> {
                        domainMin = parseFloatTriple(trimmedLine.substringAfter(KEYWORD_DOMAIN_MIN))
                            ?: floatArrayOf(0f, 0f, 0f)
                    }

                    
                    trimmedLine.startsWith(KEYWORD_DOMAIN_MAX) -> {
                        domainMax = parseFloatTriple(trimmedLine.substringAfter(KEYWORD_DOMAIN_MAX))
                            ?: floatArrayOf(1f, 1f, 1f)
                    }

                    
                    else -> {
                        val values = parseFloatTriple(trimmedLine)
                        if (values != null) {
                            if (data != null && dataIndex < data!!.size) {
                                
                                data!![dataIndex++] = normalizeValue(values[0], domainMin[0], domainMax[0])
                                data!![dataIndex++] = normalizeValue(values[1], domainMin[1], domainMax[1])
                                data!![dataIndex++] = normalizeValue(values[2], domainMin[2], domainMax[2])
                            } else {
                                
                                tempDataList.add(values)
                            }
                        }
                    }
                }
            }
        }

        if (size == 0) {
            throw IllegalArgumentException("Invalid .cube file: LUT_3D_SIZE not specified")
        }

        val expectedDataSize = size * size * size * 3
        if (dataIndex != expectedDataSize) {
            PLog.w(TAG, "Data size mismatch: expected $expectedDataSize, got $dataIndex")
        }

        return LutConfig(
            size = size,
            data = data!!,
            title = title
        )
    }

    
    private fun extractQuotedString(line: String): String {
        val start = line.indexOf('"')
        val end = line.lastIndexOf('"')
        return if (start >= 0 && end > start) {
            line.substring(start + 1, end)
        } else {
            line.substringAfter(' ').trim()
        }
    }

    
    private fun parseFloatTriple(line: String): FloatArray? {
        val parts = line.trim().split(Regex("\\s+"))
        if (parts.size < 3) return null

        return try {
            floatArrayOf(
                parts[0].toFloat(),
                parts[1].toFloat(),
                parts[2].toFloat()
            )
        } catch (e: NumberFormatException) {
            null
        }
    }

    
    private fun normalizeValue(value: Float, min: Float, max: Float): Float {
        return if (min == 0f && max == 1f) {
            value.coerceIn(0f, 1f)
        } else {
            ((value - min) / (max - min)).coerceIn(0f, 1f)
        }
    }
}
