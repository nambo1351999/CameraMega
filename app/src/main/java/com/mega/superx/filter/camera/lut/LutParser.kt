package com.mega.superx.filter.camera.lut

import android.content.Context
import android.util.Log
import com.mega.superx.filter.camera.color.TransferCurve
import com.mega.superx.filter.camera.raw.ColorSpace
import com.mega.superx.filter.camera.utils.PLog
import org.json.JSONObject
import java.io.InputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder

object LutParser {
    private const val TAG = "LutParser"
    private const val MAGIC_PLUT = 0x54554C50 

    
    fun parse(inputStream: InputStream, title: String = ""): LutConfig {
        val stream = if (inputStream.markSupported()) inputStream else inputStream.buffered()

        
        val header = ByteArray(4)
        stream.mark(16)
        val read = stream.read(header)
        stream.reset()

        if (read == 4) {
            val magic = ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN).int
            if (magic == MAGIC_PLUT) {
                return parseBinary(stream, title)
            }
        }

        
        return CubeLutParser.parse(stream)
    }

    
    private fun parseBinary(inputStream: InputStream, title: String): LutConfig {
        val fullData = inputStream.readBytes()
        val buffer = ByteBuffer.wrap(fullData).order(ByteOrder.LITTLE_ENDIAN)

        val magic = buffer.int 
        val version = buffer.int
        val size = buffer.int
        val dataType = buffer.int
        val curveStorageId = if (version >= 2) buffer.int else TransferCurve.SRGB.storageId
        val curve = TransferCurve.fromStorageId(curveStorageId)
        
        val colorSpaceOrdinal = if (version >= 3) buffer.int else ColorSpace.SRGB.ordinal
        val colorSpace = ColorSpace.entries.getOrElse(colorSpaceOrdinal) { ColorSpace.SRGB }

        val count = size * size * size * 3
        val bytesPerComponent = if (dataType == 1) 2 else 1
        val expectedSize = count * bytesPerComponent

        
        if (dataType == 0 || dataType == 1) {
            val directBuffer = ByteBuffer.allocateDirect(expectedSize)
                .order(ByteOrder.nativeOrder())

            
            val data = ByteArray(expectedSize)
            buffer.get(data)
            directBuffer.put(data)
            directBuffer.position(0)

            return LutConfig(
                size = size,
                byteBuffer = directBuffer,
                title = title,
                configDataType = if (dataType == 1) LutConfig.CONFIG_DATA_TYPE_UINT16 else LutConfig.CONFIG_DATA_TYPE_UINT8,
                curve = curve,
                colorSpace = colorSpace
            )
        } else {
            
            throw UnsupportedOperationException("Unsupported data type: $dataType")
        }
    }

    
    fun parseFromAssets(context: Context, fileName: String): LutConfig {
        return context.assets.open(fileName).use { inputStream ->
            parse(inputStream, fileName.substringAfterLast('/').substringBeforeLast('.'))
        }
    }

    
    fun listAvailableLuts(context: Context, folder: String = "luts"): List<LutInfo> {
        return try {
            
            val configPath = "$folder/config.json"
            val configJson = context.assets.open(configPath).use {
                it.bufferedReader().readText()
            }

            val jsonObject = JSONObject(configJson)
            val lutsArray = jsonObject.getJSONArray("luts")

            
            val lutList = mutableListOf<LutInfo>()
            for (i in 0 until lutsArray.length()) {
                val lutObj = lutsArray.getJSONObject(i)
                val id = lutObj.getString("id")
                val path = lutObj.getString("path")
                val nameObj = lutObj.getJSONObject("name")
                val isDefault = lutObj.optBoolean("isDefault", false)
                val isVip = lutObj.getBoolean("isVip")
                val category = lutObj.optString("category", "").trim()
                val isFavorite = lutObj.optBoolean("isFavorite", false)

                
                val nameMap = mutableMapOf<String, String>()
                nameObj.keys().forEach { lang ->
                    nameMap[lang] = nameObj.getString(lang)
                }

                lutList.add(
                    LutInfo(
                        id = id,
                        nameMap = nameMap,
                        fileName = if (path.isBlank()) "" else "$folder/$path",
                        isBuiltIn = true,
                        isDefault = isDefault,
                        isVip = isVip,
                        category = category,
                        isFavorite = isFavorite
                    )
                )
            }

            PLog.d(TAG, "Loaded ${lutList.size} LUTs from config.json")
            lutList
        } catch (e: Exception) {
            PLog.w(TAG, "Failed to load LUT config", e)
            emptyList()
        }
    }
}
