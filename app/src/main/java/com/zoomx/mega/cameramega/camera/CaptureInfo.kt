package com.zoomx.mega.cameramega.camera

import android.graphics.ColorSpace
import android.os.Build
import com.zoomx.mega.cameramega.utils.DeviceUtil

data class CaptureInfo(
    
    val exposureTime: Long? = null,          
    val iso: Int? = null,                    
    val aperture: Float? = null,             
    val focalLength: Float? = null,          
    val focalLength35mm: Int? = null,        
    val exposureBias: Float? = null,         
    
    
    val make: String = Build.MANUFACTURER,
    val model: String = DeviceUtil.exifModel, 
    val software: String = "CameraMega",
    
    
    val whiteBalance: Int? = null,           
    val flashState: Int? = null,             
    val orientation: Int = 0,                
    
    
    val imageWidth: Int = 0,
    val imageHeight: Int = 0,
    
    
    val captureTime: Long = System.currentTimeMillis(),
    
    
    val latitude: Double? = null,
    val longitude: Double? = null,
    val altitude: Double? = null,

    
    val colorSpace: ColorSpace.Named = ColorSpace.Named.SRGB,
) {
    
    fun formatExposureTime(): String? {
        val exposureNs = exposureTime ?: return null
        val exposureSec = exposureNs / 1_000_000_000.0
        
        return when {
            exposureSec >= 1.0 -> "${exposureSec.toInt()}s"
            exposureSec >= 0.1 -> String.format("%.1fs", exposureSec)
            else -> {
                val denominator = (1.0 / exposureSec).toInt()
                "1/${denominator}s"
            }
        }
    }
    
    
    fun formatAperture(): String? {
        val f = aperture ?: return null
        return if (f == f.toInt().toFloat()) {
            "f/${f.toInt()}"
        } else {
            "f/${String.format("%.1f", f)}"
        }
    }
    
    
    fun formatFocalLength(): String? {
        val fl = focalLength ?: return null
        return if (fl == fl.toInt().toFloat()) {
            "${fl.toInt()}mm"
        } else {
            String.format("%.1fmm", fl)
        }
    }

    
    fun formatFocalLength35mm(): String? {
        val fl = focalLength35mm ?: return null
        return "${fl}mm"
    }
    
    
    fun formatIso(): String? {
        val isoValue = iso ?: return null
        return "ISO $isoValue"
    }
}
