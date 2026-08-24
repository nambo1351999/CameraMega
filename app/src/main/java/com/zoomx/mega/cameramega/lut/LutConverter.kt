package com.zoomx.mega.cameramega.lut

import java.io.InputStream
import java.io.OutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.math.max
import kotlin.math.min
import android.graphics.BitmapFactory
import android.graphics.Color
import com.zoomx.mega.cameramega.color.TransferCurve
import com.zoomx.mega.cameramega.raw.ColorSpace
import com.zoomx.mega.cameramega.utils.BoundedTextLineReader
import com.zoomx.mega.cameramega.utils.PLog

import kotlin.math.roundToInt

object LutConverter {

    private const val TAG = "LutConverter"
    private const val MAGIC_PLUT = "PLUT"
    private const val MAGIC_PLUT_INT = 0x54554C50  
    private const val VERSION = 3
    private const val VERSION_WITH_RECIPE = 4
    private const val DATA_TYPE_UINT16 = 1

    
    fun convertCubeToplut(
        cubeInputStream: InputStream,
        plutOutputStream: OutputStream,
        colorSpace: ColorSpace = ColorSpace.SRGB,
        curve: TransferCurve = TransferCurve.SRGB
    ): Boolean {
        return try {
            
            var cubeData = parseCubeFile(cubeInputStream)

            if (cubeData.size > 65) {
                cubeData = resampleSize(cubeData, 33)
            }

            
            
            writePLutFile(cubeData, plutOutputStream, colorSpace, curve)

            true
        } catch (e: IllegalArgumentException) {
            PLog.w(TAG, "Failed to convert cube LUT", e)
            false
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to convert cube LUT", e)
            false
        }
    }

    
    fun convertPngToplut(
        pngInputStream: InputStream,
        plutOutputStream: OutputStream,
        colorSpace: ColorSpace = ColorSpace.SRGB,
        curve: TransferCurve = TransferCurve.SRGB
    ): Boolean {
        return try {
            val options = BitmapFactory.Options()
            options.inScaled = false
            options.inPremultiplied = false

            val bitmap = BitmapFactory.decodeStream(pngInputStream, null, options)
            if (bitmap == null) {
                PLog.e("LutConverter", "convertPngToplut: decodeStream returned null")
                return false
            }
            val width = bitmap.width
            val height = bitmap.height

            val isHaldStr = isHald(width, height)
            val isUnwrappedCubeStr = isUnwrappedCube(width, height)
            
            PLog.d("LutConverter", "convertPngToplut: decoded size = ${width}x${height}, isHald = $isHaldStr, isUnwrappedCube = $isUnwrappedCubeStr")

            val lutSize: Int
            val values: ShortArray

            val pixels = IntArray(width * height)
            bitmap.getPixels(pixels, 0, width, 0, 0, width, height)

            if (isHaldStr) {
                val haldLevel = determineHaldLevel(width)
                if (haldLevel == null) {
                    PLog.e("LutConverter", "convertPngToplut: determineHaldLevel returned null for width = $width")
                    bitmap.recycle()
                    return false
                }
                lutSize = haldLevel * haldLevel
                values = ShortArray(lutSize * lutSize * lutSize * 3)
                
                var dataIndex = 0
                for (bIdx in 0 until lutSize) {
                    for (gIdx in 0 until lutSize) {
                        for (rIdx in 0 until lutSize) {
                            val squareX = bIdx % haldLevel
                            val squareY = bIdx / haldLevel
                            val x = squareX * lutSize + rIdx
                            val y = squareY * lutSize + gIdx
                            
                            val pixel = pixels[y * width + x]
                            val r = Color.red(pixel)
                            val g = Color.green(pixel)
                            val b = Color.blue(pixel)
                            
                            val rNorm = (r / 255f).coerceIn(0f, 1f)
                            val gNorm = (g / 255f).coerceIn(0f, 1f)
                            val bNorm = (b / 255f).coerceIn(0f, 1f)
                            
                            values[dataIndex++] = (rNorm * 65535f + 0.5f).toInt().toShort()
                            values[dataIndex++] = (gNorm * 65535f + 0.5f).toInt().toShort()
                            values[dataIndex++] = (bNorm * 65535f + 0.5f).toInt().toShort()
                        }
                    }
                }
            } else if (isUnwrappedCubeStr) {
                val root = determineUnwrappedCubeRoot(width, height)
                if (root == null) {
                    PLog.e("LutConverter", "convertPngToplut: determineUnwrappedCubeRoot returned null for ${width}x${height}")
                    bitmap.recycle()
                    return false
                }
                lutSize = root
                values = ShortArray(lutSize * lutSize * lutSize * 3)
                
                var dataIndex = 0
                for (bIdx in 0 until lutSize) {
                    for (gIdx in 0 until lutSize) {
                        for (rIdx in 0 until lutSize) {
                            val x = bIdx * lutSize + rIdx
                            val y = gIdx
                            
                            val pixel = pixels[y * width + x]
                            val r = Color.red(pixel)
                            val g = Color.green(pixel)
                            val b = Color.blue(pixel)
                            
                            val rNorm = (r / 255f).coerceIn(0f, 1f)
                            val gNorm = (g / 255f).coerceIn(0f, 1f)
                            val bNorm = (b / 255f).coerceIn(0f, 1f)
                            
                            values[dataIndex++] = (rNorm * 65535f + 0.5f).toInt().toShort()
                            values[dataIndex++] = (gNorm * 65535f + 0.5f).toInt().toShort()
                            values[dataIndex++] = (bNorm * 65535f + 0.5f).toInt().toShort()
                        }
                    }
                }
            } else {
                PLog.e("LutConverter", "convertPngToplut: neither Hald nor UnwrappedCube size format matched")
                bitmap.recycle()
                return false
            }

            bitmap.recycle()

            var cubeData = CubeData(lutSize, values)
            if (cubeData.size > 65) {
                cubeData = resampleSize(cubeData, 33)
            }
            writePLutFile(cubeData, plutOutputStream, colorSpace, curve)
            PLog.d("LutConverter", "convertPngToplut: successfully converted to plut with size = $lutSize")
            true
        } catch (e: Exception) {
            PLog.e("LutConverter", "convertPngToplut exception", e)
            false
        }
    }

    private fun isHald(width: Int, height: Int): Boolean {
        if (width != height || width < 1) return false
        val levelFloat = Math.cbrt(width.toDouble())
        val levelRound = levelFloat.roundToInt()
        return levelRound * levelRound * levelRound == width && levelRound >= 1
    }

    private fun determineHaldLevel(width: Int): Int? {
        if (width < 1) return null
        val levelFloat = Math.cbrt(width.toDouble())
        val levelRound = levelFloat.roundToInt()
        return if (levelRound * levelRound * levelRound == width && levelRound >= 1) {
            levelRound
        } else {
            null
        }
    }

    private fun isUnwrappedCube(width: Int, height: Int): Boolean {
        if (width <= height || height < 1) return false
        return width == height * height
    }

    private fun determineUnwrappedCubeRoot(width: Int, height: Int): Int? {
        if (width <= height || height < 1) return null
        return if (width == height * height) height else null
    }

    private fun resampleLut(cubeData: CubeData, curve: TransferCurve): CubeData {
        val size = cubeData.size
        
        val nativeData = try {
            LutProcessor.resampleLutNative(cubeData.data, size, curve.storageId)
        } catch (e: Throwable) {
            null
        }

        if (nativeData != null) {
            return CubeData(size, nativeData)
        }

        val newData = ShortArray(size * size * size * 3)
        val step = 1.0f / (size - 1)

        for (bIdx in 0 until size) {
            for (gIdx in 0 until size) {
                for (rIdx in 0 until size) {
                    val r = rIdx * step
                    val g = gIdx * step
                    val b = bIdx * step

                    
                    val rLin = TransferCurve.SRGB.logToLinear(r)
                    val gLin = TransferCurve.SRGB.logToLinear(g)
                    val bLin = TransferCurve.SRGB.logToLinear(b)

                    
                    val rLog = curve.linearToLog(rLin)
                    val gLog = curve.linearToLog(gLin)
                    val bLog = curve.linearToLog(bLin)

                    
                    val interpolated = trilinearSample(cubeData, rLog, gLog, bLog)

                    val index = ((bIdx * size + gIdx) * size + rIdx) * 3
                    newData[index] = interpolated[0]
                    newData[index + 1] = interpolated[1]
                    newData[index + 2] = interpolated[2]
                }
            }
        }
        return CubeData(size, newData)
    }

    private fun trilinearSample(cubeData: CubeData, r: Float, g: Float, b: Float): ShortArray {
        val size = cubeData.size
        val data = cubeData.data

        
        val x = (r * (size - 1)).coerceIn(0f, size - 1.0001f)
        val y = (g * (size - 1)).coerceIn(0f, size - 1.0001f)
        val z = (b * (size - 1)).coerceIn(0f, size - 1.0001f)

        val x0 = x.toInt()
        val x1 = x0 + 1
        val y0 = y.toInt()
        val y1 = y0 + 1
        val z0 = z.toInt()
        val z1 = z0 + 1

        val dx = x - x0
        val dy = y - y0
        val dz = z - z0

        val result = ShortArray(3)
        for (c in 0..2) {
            val v000 = data[((z0 * size + y0) * size + x0) * 3 + c].toInt() and 0xFFFF
            val v100 = data[((z0 * size + y0) * size + x1) * 3 + c].toInt() and 0xFFFF
            val v010 = data[((z0 * size + y1) * size + x0) * 3 + c].toInt() and 0xFFFF
            val v110 = data[((z0 * size + y1) * size + x1) * 3 + c].toInt() and 0xFFFF
            val v001 = data[((z1 * size + y0) * size + x0) * 3 + c].toInt() and 0xFFFF
            val v101 = data[((z1 * size + y0) * size + x1) * 3 + c].toInt() and 0xFFFF
            val v011 = data[((z1 * size + y1) * size + x0) * 3 + c].toInt() and 0xFFFF
            val v111 = data[((z1 * size + y1) * size + x1) * 3 + c].toInt() and 0xFFFF

            val v00 = v000 * (1 - dx) + v100 * dx
            val v10 = v010 * (1 - dx) + v110 * dx
            val v01 = v001 * (1 - dx) + v101 * dx
            val v11 = v011 * (1 - dx) + v111 * dx

            val v0 = v00 * (1 - dy) + v10 * dy
            val v1 = v01 * (1 - dy) + v11 * dy

            val v = v0 * (1 - dz) + v1 * dz
            result[c] = (v + 0.5f).toInt().toShort()
        }
        return result
    }

    private fun resampleSize(cubeData: CubeData, targetSize: Int): CubeData {
        PLog.d("resampleSize", "${cubeData.size} $targetSize")
        val nativeData = try {
            LutProcessor.resampleSizeNative(cubeData.data, cubeData.size, targetSize)
        } catch (e: Throwable) {
            PLog.e("resampleSize", "error", e)
            null
        }

        if (nativeData != null) {
            PLog.d("resampleSize", "complete")
            return CubeData(targetSize, nativeData)
        }

        val newData = ShortArray(targetSize * targetSize * targetSize * 3)
        val step = 1.0f / (targetSize - 1)

        for (bIdx in 0 until targetSize) {
            for (gIdx in 0 until targetSize) {
                for (rIdx in 0 until targetSize) {
                    val r = rIdx * step
                    val g = gIdx * step
                    val b = bIdx * step

                    val interpolated = trilinearSample(cubeData, r, g, b)

                    val index = ((bIdx * targetSize + gIdx) * targetSize + rIdx) * 3
                    newData[index] = interpolated[0]
                    newData[index + 1] = interpolated[1]
                    newData[index + 2] = interpolated[2]
                }
            }
        }
        return CubeData(targetSize, newData)
    }

    
    private fun parseCubeFile(inputStream: InputStream): CubeData {
        var size = 0
        var domainMin = floatArrayOf(0f, 0f, 0f)
        var domainMax = floatArrayOf(1f, 1f, 1f)
        var data: ShortArray? = null
        var dataIndex = 0

        
        val tempDataList = mutableListOf<FloatArray>()
        
        
        val floatValues = FloatArray(3)

        fun parseFloats(line: String, startIndex: Int = 0): Boolean {
            var count = 0
            var i = startIndex
            val len = line.length
            while (i < len && count < 3) {
                while (i < len && line[i].isWhitespace()) {
                    i++
                }
                if (i >= len) break
                val start = i
                while (i < len && !line[i].isWhitespace()) {
                    i++
                }
                try {
                    floatValues[count++] = line.substring(start, i).toFloat()
                } catch (e: NumberFormatException) {
                    return false
                }
            }
            return count == 3
        }

        inputStream.bufferedReader(Charsets.UTF_8).use { reader ->
            BoundedTextLineReader.forEachLine(reader) { line ->
                val trimmed = line.trim()

                
                if (trimmed.isEmpty() || trimmed.startsWith('#')) {
                    return@forEachLine
                }

                when {
                    trimmed.startsWith("LUT_3D_SIZE") -> {
                        val spaceIdx = trimmed.indexOfFirst { it.isWhitespace() }
                        if (spaceIdx != -1) {
                            try {
                                size = trimmed.substring(spaceIdx).trim().toInt()
                                
                                data = ShortArray(size * size * size * 3)

                                
                                for (rgb in tempDataList) {
                                    for (i in 0..2) {
                                        var value = (rgb[i] - domainMin[i]) / (domainMax[i] - domainMin[i])
                                        value = max(0f, min(1f, value))
                                        data[dataIndex++] = (value * 65535f + 0.5f).toInt().toShort()
                                    }
                                }
                                tempDataList.clear()  
                            } catch (e: NumberFormatException) {
                                
                            }
                        }
                    }

                    trimmed.startsWith("DOMAIN_MIN") -> {
                        val spaceIdx = trimmed.indexOfFirst { it.isWhitespace() }
                        if (spaceIdx != -1 && parseFloats(trimmed, spaceIdx)) {
                            domainMin = floatArrayOf(floatValues[0], floatValues[1], floatValues[2])
                        }
                    }

                    trimmed.startsWith("DOMAIN_MAX") -> {
                        val spaceIdx = trimmed.indexOfFirst { it.isWhitespace() }
                        if (spaceIdx != -1 && parseFloats(trimmed, spaceIdx)) {
                            domainMax = floatArrayOf(floatValues[0], floatValues[1], floatValues[2])
                        }
                    }

                    !trimmed.startsWith("TITLE") && !trimmed.startsWith("LUT_1D_SIZE") -> {
                        
                        if (parseFloats(trimmed)) {
                            if (data != null && dataIndex < data!!.size) {
                                
                                for (i in 0..2) {
                                    var value = (floatValues[i] - domainMin[i]) / (domainMax[i] - domainMin[i])
                                    value = max(0f, min(1f, value))
                                    data!![dataIndex++] = (value * 65535f + 0.5f).toInt().toShort()
                                }
                            } else {
                                
                                tempDataList.add(floatArrayOf(floatValues[0], floatValues[1], floatValues[2]))
                            }
                        }
                    }
                }
            }
        }

        if (size == 0) {
            throw IllegalArgumentException("Could not find LUT_3D_SIZE in .cube file")
        }

        return CubeData(size, data!!)
    }

    
    private fun writePLutFile(cubeData: CubeData, outputStream: OutputStream, colorSpace: ColorSpace, curve: TransferCurve) {
        val buffer = ByteBuffer.allocate(24 + cubeData.data.size * 2)
            .order(ByteOrder.LITTLE_ENDIAN)

        
        buffer.put(MAGIC_PLUT.toByteArray(Charsets.US_ASCII))

        
        buffer.putInt(VERSION)

        
        buffer.putInt(cubeData.size)

        
        buffer.putInt(DATA_TYPE_UINT16)

        
        buffer.putInt(curve.storageId)
        
        
        buffer.putInt(colorSpace.ordinal)

        
        val shortBuffer = buffer.asShortBuffer()
        shortBuffer.put(cubeData.data)

        outputStream.write(buffer.array())
        outputStream.flush()
    }

    
    fun exportToPlut(lutConfig: LutConfig, outputStream: OutputStream, recipeJson: String? = null) {
        val pixelBuf = lutConfig.toByteBuffer()
        val pixelBytes = ByteArray(pixelBuf.capacity())
        pixelBuf.position(0)
        pixelBuf.get(pixelBytes)

        val recipeBytes = recipeJson?.toByteArray(Charsets.UTF_8)
        val version = if (recipeBytes != null) VERSION_WITH_RECIPE else VERSION
        val recipeSection = if (recipeBytes != null) 4 + recipeBytes.size else 0

        val outBuffer = ByteBuffer.allocate(24 + pixelBytes.size + recipeSection)
            .order(ByteOrder.LITTLE_ENDIAN)
        outBuffer.put(MAGIC_PLUT.toByteArray(Charsets.US_ASCII))
        outBuffer.putInt(version)
        outBuffer.putInt(lutConfig.size)
        outBuffer.putInt(lutConfig.configDataType)
        outBuffer.putInt(lutConfig.curve.storageId)
        outBuffer.putInt(lutConfig.colorSpace.ordinal)
        outBuffer.put(pixelBytes)
        if (recipeBytes != null) {
            outBuffer.putInt(recipeBytes.size)
            outBuffer.put(recipeBytes)
        }

        outputStream.write(outBuffer.array())
        outputStream.flush()
    }

    
    fun importPlutStrippingRecipe(inputStream: InputStream, outputStream: OutputStream): Boolean {
        return try {
            val fullData = inputStream.readBytes()
            val inBuffer = ByteBuffer.wrap(fullData).order(ByteOrder.LITTLE_ENDIAN)

            val magic = inBuffer.int
            if (magic != MAGIC_PLUT_INT) return false

            val version = inBuffer.int
            val size = inBuffer.int
            val dataType = inBuffer.int
            val curveStorageId = if (version >= 2) inBuffer.int else TransferCurve.SRGB.storageId
            val colorSpaceOrdinal = if (version >= 3) inBuffer.int else ColorSpace.SRGB.ordinal

            val bytesPerComponent = if (dataType == DATA_TYPE_UINT16) 2 else 1
            val dataByteCount = size * size * size * 3 * bytesPerComponent
            val dataBytes = ByteArray(dataByteCount)
            inBuffer.get(dataBytes)

            
            val outBuffer = ByteBuffer.allocate(24 + dataByteCount).order(ByteOrder.LITTLE_ENDIAN)
            outBuffer.put(MAGIC_PLUT.toByteArray(Charsets.US_ASCII))
            outBuffer.putInt(VERSION)
            outBuffer.putInt(size)
            outBuffer.putInt(dataType)
            outBuffer.putInt(curveStorageId)
            outBuffer.putInt(colorSpaceOrdinal)
            outBuffer.put(dataBytes)

            outputStream.write(outBuffer.array())
            outputStream.flush()
            true
        } catch (e: Exception) {
            e.printStackTrace()
            false
        }
    }

    
    fun extractRecipeJsonFromPlut(inputStream: InputStream): String? {
        return try {
            val fullData = inputStream.readBytes()
            val buffer = ByteBuffer.wrap(fullData).order(ByteOrder.LITTLE_ENDIAN)

            val magic = buffer.int
            if (magic != MAGIC_PLUT_INT) return null

            val version = buffer.int
            if (version < VERSION_WITH_RECIPE) return null

            val size = buffer.int
            val dataType = buffer.int
            buffer.int  
            buffer.int  

            val bytesPerComponent = if (dataType == DATA_TYPE_UINT16) 2 else 1
            val dataByteCount = size * size * size * 3 * bytesPerComponent
            if (buffer.remaining() < dataByteCount + 4) return null
            buffer.position(buffer.position() + dataByteCount)

            val recipeLength = buffer.int
            if (recipeLength <= 0 || buffer.remaining() < recipeLength) return null

            val recipeBytes = ByteArray(recipeLength)
            buffer.get(recipeBytes)
            String(recipeBytes, Charsets.UTF_8)
        } catch (e: Exception) {
            null
        }
    }

    
    private data class CubeData(
        val size: Int,
        val data: ShortArray
    ) {
        override fun equals(other: Any?): Boolean {
            if (this === other) return true
            if (javaClass != other?.javaClass) return false

            other as CubeData

            if (size != other.size) return false
            if (!data.contentEquals(other.data)) return false

            return true
        }

        override fun hashCode(): Int {
            var result = size
            result = 31 * result + data.contentHashCode()
            return result
        }
    }
}
