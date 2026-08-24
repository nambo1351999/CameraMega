package com.zoomx.mega.cameramega.camera

import android.graphics.ImageFormat
import android.hardware.camera2.CameraCharacteristics
import android.os.Build
import android.util.Log
import android.util.Size
import com.zoomx.mega.cameramega.utils.PLog

object HighResolutionHelper {
    
    private const val TAG = "HighResolutionHelper"
    
    
    const val MP_12 = 12_000_000L
    const val MP_24 = 24_000_000L
    const val MP_48 = 48_000_000L
    const val MP_108 = 108_000_000L
    const val MP_200 = 200_000_000L
    
    
    data class ResolutionConfig(
        val size: Size,
        val format: Int
    ) {
        val pixels: Long get() = size.width.toLong() * size.height.toLong()
        val megapixels: Double get() = pixels / 1_000_000.0
        
        override fun toString(): String {
            return "${size.width}x${size.height} (${String.format("%.1f", megapixels)}MP, format=$format)"
        }
    }
    
    
    data class MegapixelOptions(
        val mp12: ResolutionConfig? = null,  
        val mp24: ResolutionConfig? = null,  
        val mp48: ResolutionConfig? = null,  
        val mp108: ResolutionConfig? = null, 
        val mp200: ResolutionConfig? = null, 
        val maxResolution: ResolutionConfig? = null,
        val allResolutions: List<ResolutionConfig> = emptyList()
    ) {
        val maxMegapixels: Int
            get() = when {
                mp200 != null -> 200
                mp108 != null -> 108
                mp48 != null -> 48
                mp24 != null -> 24
                mp12 != null -> 12
                else -> 0
            }
    }
    
    
    fun getHighResolutionConfigs(
        characteristics: CameraCharacteristics,
        minPixels: Long = MP_12
    ): List<ResolutionConfig> {
        val configs = mutableListOf<ResolutionConfig>()
        val seen = mutableSetOf<String>()
        
        
        getFromStandardApi(characteristics, minPixels, configs, seen)
        
        
        getFromVendorKeys(characteristics, minPixels, configs, seen)
        
        
        return configs.sortedByDescending { it.pixels }
    }
    
    
    fun getMegapixelOptions(characteristics: CameraCharacteristics): MegapixelOptions {
        val allConfigs = getHighResolutionConfigs(characteristics, MP_12)
        
        if (allConfigs.isEmpty()) {
            return MegapixelOptions()
        }
        
        var mp12: ResolutionConfig? = null
        var mp24: ResolutionConfig? = null
        var mp48: ResolutionConfig? = null
        var mp108: ResolutionConfig? = null
        var mp200: ResolutionConfig? = null
        
        
        for (config in allConfigs.sortedBy { it.pixels }) {
            if (config.pixels >= MP_12 && mp12 == null) mp12 = config
            if (config.pixels >= MP_24 && mp24 == null) mp24 = config
            if (config.pixels >= MP_48 && mp48 == null) mp48 = config
            if (config.pixels >= MP_108 && mp108 == null) mp108 = config
            if (config.pixels >= MP_200 && mp200 == null) mp200 = config
        }
        
        return MegapixelOptions(
            mp12 = mp12,
            mp24 = mp24,
            mp48 = mp48,
            mp108 = mp108,
            mp200 = mp200,
            maxResolution = allConfigs.firstOrNull(),
            allResolutions = allConfigs
        )
    }
    
    
    private fun getFromStandardApi(
        characteristics: CameraCharacteristics,
        minPixels: Long,
        configs: MutableList<ResolutionConfig>,
        seen: MutableSet<String>
    ) {
        val streamConfigMap = characteristics.get(
            CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP
        ) ?: return
        
        
        val formats = listOf(
            ImageFormat.JPEG,           
            ImageFormat.YUV_420_888,    
            ImageFormat.RAW_SENSOR,     
            ImageFormat.RAW_PRIVATE,    
            ImageFormat.RAW10,          
            ImageFormat.RAW12           
        )
        
        for (format in formats) {
            try {
                
                streamConfigMap.getOutputSizes(format)?.forEach { size ->
                    addIfValid(size, format, minPixels, configs, seen)
                }
                
                
                streamConfigMap.getHighResolutionOutputSizes(format)?.forEach { size ->
                    addIfValid(size, format, minPixels, configs, seen)
                }
            } catch (e: Exception) {
                PLog.w(TAG, "Error getting output sizes for format $format: ${e.message}")
            }
        }
    }
    
    
    private fun getFromVendorKeys(
        characteristics: CameraCharacteristics,
        minPixels: Long,
        configs: MutableList<ResolutionConfig>,
        seen: MutableSet<String>
    ) {
        val vendorKeys = getVendorKeyList()
        
        for (keyName in vendorKeys) {
            try {
                val value = getVendorKeyValue(characteristics, keyName) ?: continue
                parseVendorStreamConfigurations(value, minPixels, configs, seen)
                PLog.d(TAG, "Found vendor key: $keyName with ${value.size} values")
            } catch (e: Exception) {
                
            }
        }
    }
    
    
    private fun getVendorKeyList(): List<String> {
        val manufacturer = (Build.MANUFACTURER ?: "unknown").lowercase()
        
        return when {
            manufacturer.contains("samsung") -> listOf(
                "samsung.android.scaler.availablePreviewStreamConfigurations",
                "samsung.android.scaler.availablePictureStreamConfigurations",
                "samsung.android.scaler.availableFullPreviewStreamConfigurations",
                "samsung.android.scaler.availableFullPictureStreamConfigurations",
                "samsung.android.scaler.availableCropPreviewStreamConfigurations",
                "samsung.android.scaler.availableCropPictureStreamConfigurations",
                "samsung.android.scaler.availableSuperNightRawStreamConfigurations",
                "samsung.android.scaler.availableSuperResolutionRawStreamConfigurations",
                "samsung.android.scaler.availableTetraPictureStreamConfigurations",
                "samsung.android.scaler.availableSubPictureStreamConfigurations"
            )
            
            manufacturer.contains("xiaomi") || manufacturer.contains("redmi") -> listOf(
                "com.xiaomi.scaler.availableStreamConfigurations",
                "com.xiaomi.scaler.availableSuperResolutionStreamConfigurations",
                "com.xiaomi.camera.extension.night_availableStreamConfigurations",
                "com.xiaomi.camera.extension.auto_availableStreamConfigurations",
                "com.xiaomi.camera.extension.bokeh_availableStreamConfigurations",
                "com.xiaomi.camera.extension.beauty_availableStreamConfigurations",
                "com.xiaomi.camera.extension.hdr_availableStreamConfigurations",
                "xiaomi.scaler.availableStreamConfigurations",
                "xiaomi.scaler.availableHeicStreamConfigurations",
                "xiaomi.scaler.availableLimitStreamConfigurations",
                "xiaomi.scaler.availableSuperResolutionStreamConfigurations"
            )
            
            manufacturer.contains("huawei") || manufacturer.contains("honor") -> listOf(
                "com.huawei.device.capabilities.hwCaptureRawStreamConfigurations",
                "com.huawei.device.capabilities.hwAvailableDepthStreamConfigurations",
                "com.huawei.device.capabilities.hwStreamConfigurations",
                "com.huawei.device.capabilities.HighPixelCallbackStreamSize"
            )
            
            manufacturer.contains("oppo") || manufacturer.contains("realme") -> listOf(
                "com.oplus.scaler.availableStreamConfigurations",
                "com.oplus.scaler.availablePictureStreamConfigurations",
                "com.oplus.scaler.availablePreviewStreamConfigurations",
                "com.oplus.scaler.availableVideoStreamConfigurations",
                "com.oplus.high.picturesize",
                "com.oplus.custom.jpeg.size",
                "com.oplus.upscale.input.size",
                "com.oplus.upscale.output.size"
            )
            
            manufacturer.contains("vivo") || manufacturer.contains("iqoo") -> listOf(
                "vivo.control.image_sizes",
                "vivo.scaler.availableStreamConfigurations",
                "com.vivo.scaler.availableStreamConfigurations"
            )
            
            manufacturer.contains("oneplus") -> listOf(
                "com.oplus.scaler.availableStreamConfigurations",
                "com.oplus.scaler.availablePictureStreamConfigurations"
            )
            
            manufacturer.contains("meizu") -> listOf(
                "com.meizu.scaler.availableStreamConfigurations",
                "com.meizu.scaler.availableSuperResolutionStreamConfigurations"
            )
            
            manufacturer.contains("infinix") || manufacturer.contains("tecno") -> listOf(
                "com.addParameters.availableStreamConfigurations",
                "com.sensor.info.availableStreamConfigurations"
            )
            
            manufacturer.contains("google") -> listOf(
                
            )
            
            else -> emptyList()
        }
    }
    
    
    @Suppress("UNCHECKED_CAST")
    private fun getVendorKeyValue(
        characteristics: CameraCharacteristics,
        keyName: String
    ): IntArray? {
        return try {
            val keyConstructor = CameraCharacteristics.Key::class.java
                .getDeclaredConstructor(String::class.java, Class::class.java)
            keyConstructor.isAccessible = true
            
            val key = keyConstructor.newInstance(keyName, IntArray::class.java)
                    as CameraCharacteristics.Key<IntArray>
            
            characteristics.get(key)
        } catch (e: Exception) {
            null
        }
    }
    
    
    private fun parseVendorStreamConfigurations(
        values: IntArray,
        minPixels: Long,
        configs: MutableList<ResolutionConfig>,
        seen: MutableSet<String>
    ) {
        if (values.size < 4) return
        
        var i = 0
        while (i + 3 < values.size) {
            val format = values[i]
            val width = values[i + 1]
            val height = values[i + 2]
            
            
            val pixels = width.toLong() * height.toLong()
            if (pixels >= minPixels && width > 0 && height > 0) {
                val size = Size(width, height)
                addIfValid(size, format, minPixels, configs, seen)
            }
            
            i += 4
        }
    }
    
    
    private fun addIfValid(
        size: Size,
        format: Int,
        minPixels: Long,
        configs: MutableList<ResolutionConfig>,
        seen: MutableSet<String>
    ) {
        val pixels = size.width.toLong() * size.height.toLong()
        if (pixels < minPixels) return
        
        val key = "${format}@${size.width}x${size.height}"
        if (seen.contains(key)) return
        
        seen.add(key)
        configs.add(ResolutionConfig(size, format))
    }
    
    
    fun getFormatName(format: Int): String {
        return when (format) {
            ImageFormat.JPEG -> "JPEG"
            ImageFormat.YUV_420_888 -> "YUV_420_888"
            ImageFormat.RAW_SENSOR -> "RAW_SENSOR"
            ImageFormat.RAW_PRIVATE -> "RAW_PRIVATE"
            ImageFormat.RAW10 -> "RAW10"
            ImageFormat.RAW12 -> "RAW12"
            ImageFormat.HEIC -> "HEIC"
            ImageFormat.DEPTH16 -> "DEPTH16"
            ImageFormat.DEPTH_POINT_CLOUD -> "DEPTH_POINT_CLOUD"
            else -> "UNKNOWN($format)"
        }
    }
    
    
    fun logResolutionCapabilities(characteristics: CameraCharacteristics) {
        val options = getMegapixelOptions(characteristics)
        
        PLog.i(TAG, "========== High Resolution Capabilities ==========")
        PLog.i(TAG, "Max Megapixels: ${options.maxMegapixels}MP")
        PLog.i(TAG, "Max Resolution: ${options.maxResolution}")
        PLog.i(TAG, "12MP: ${options.mp12}")
        PLog.i(TAG, "24MP: ${options.mp24}")
        PLog.i(TAG, "48MP: ${options.mp48}")
        PLog.i(TAG, "108MP: ${options.mp108}")
        PLog.i(TAG, "200MP: ${options.mp200}")
        PLog.i(TAG, "All resolutions (${options.allResolutions.size}):")
        options.allResolutions.forEach { config ->
            PLog.i(TAG, "  - $config")
        }
        PLog.i(TAG, "===================================================")
    }
}
