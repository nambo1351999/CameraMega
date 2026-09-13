package com.mega.superx.filter.camera.camera

import android.content.Context
import android.graphics.ImageFormat
import android.graphics.SurfaceTexture
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.util.Log
import android.util.Size
import com.mega.superx.filter.camera.utils.PLog
import kotlin.math.abs

object CameraUtils {

    
    fun getFixedPreviewSize(
        context: Context,
        cameraId: String,
        aspectRatio: AspectRatio
    ): Size {
        return try {
            val cameraManager = context.getSystemService(Context.CAMERA_SERVICE) as CameraManager
            val characteristics = cameraManager.getCameraCharacteristics(cameraId)
            getFixedPreviewSize(characteristics, aspectRatio)
        } catch (e: Exception) {
            PLog.d("CameraUtils", "getFixedPreviewSize: ${e.message}")
            Size(1440, 1080)
        }
    }

    fun getFixedPreviewSize(
        characteristics: CameraCharacteristics,
        aspectRatio: AspectRatio
    ): Size {
        return try {
            val map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
                ?: return Size(1440, 1080)

            
            val sensorRatio = aspectRatio.getValue(true)

            val previewSizes = map.getOutputSizes(SurfaceTexture::class.java)?.toList() ?: emptyList()

            
            val matchingSizes = previewSizes.filter {
                val ratio = it.width.toFloat() / it.height
                abs(ratio - sensorRatio) < 0.01
            }

            
            val bestMatching = matchingSizes.filter { it.height <= 1080 }.maxByOrNull { it.width * it.height }
                ?: matchingSizes.minByOrNull { it.width * it.height }

            if (bestMatching != null) return bestMatching

            
            
            
            
            
            
            
            
            val captureSizes = map.getOutputSizes(android.graphics.ImageFormat.YUV_420_888)?.toList() ?: emptyList()
            val bestCaptureSize = captureSizes.maxByOrNull { it.width * it.height }
            
            if (bestCaptureSize != null) {
                val captureRatio = bestCaptureSize.width.toFloat() / bestCaptureSize.height
                val captureMatchingSizes = previewSizes.filter {
                    val ratio = it.width.toFloat() / it.height
                    abs(ratio - captureRatio) < 0.01
                }
                val bestCaptureMatching = captureMatchingSizes
                    .filter { it.height <= 1080 }
                    .maxByOrNull { it.width * it.height }
                    ?: captureMatchingSizes.minByOrNull { it.width * it.height }
                
                if (bestCaptureMatching != null) return bestCaptureMatching
            }

            
            return previewSizes.filter { it.height >= 1080 }.minByOrNull { it.width } ?: Size(1440, 1080)
        } catch (e: Exception) {
            PLog.d("CameraUtils", "getFixedPreviewSize: ${e.message}")
            return Size(1440, 1080)
        }
    }

    
    fun getBestCaptureSize(
        context: Context,
        cameraId: String,
        aspectRatio: AspectRatio,
        format: Int = ImageFormat.YUV_420_888
    ): Size {
        return try {
            val cameraManager = context.getSystemService(Context.CAMERA_SERVICE) as CameraManager
            val characteristics = cameraManager.getCameraCharacteristics(cameraId)
            getBestCaptureSize(characteristics, aspectRatio, format)
        } catch (e: Exception) {

            Size(1920, 1080)
        }
    }

    fun getBestCaptureSize(
        characteristics: CameraCharacteristics,
        aspectRatio: AspectRatio,
        format: Int = ImageFormat.YUV_420_888
    ): Size {
        return try {
            val map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            val sizes = map?.getOutputSizes(format)
                ?: map?.getOutputSizes(ImageFormat.YUV_420_888)
                ?: arrayOf(Size(1920, 1080))
            val sensorRatio = aspectRatio.getValue(true)

            
            val matchingSizes = sizes.filter {
                val ratio = it.width.toFloat() / it.height
                abs(ratio - sensorRatio) < 0.01
            }

            val bestMatching = matchingSizes.filter { it.height >= 2160 }.maxByOrNull { it.width * it.height }

            if (bestMatching != null) return bestMatching

            
            sizes.maxByOrNull { it.width * it.height } ?: Size(1920, 1080)
        } catch (e: Exception) {

            Size(1920, 1080)
        }
    }
    
    
    fun getRawCaptureSize(context: Context, cameraId: String): Size? {
        return try {
            val cameraManager = context.getSystemService(Context.CAMERA_SERVICE) as CameraManager
            val characteristics = cameraManager.getCameraCharacteristics(cameraId)
            getRawCaptureSize(characteristics)
        } catch (e: Exception) {
            PLog.e("CameraUtils", "Failed to get RAW capture size: ${e.message}")
            null
        }
    }

    fun getRawCaptureSize(characteristics: CameraCharacteristics): Size? {
        return try {
            val map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            val sizes = map?.getOutputSizes(ImageFormat.RAW_SENSOR) ?: return null
            
            if (sizes.isEmpty()) return null

            val pixelArraySize = characteristics.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE)
            val preCorrectionSize = characteristics.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE)

            
            val bestMatch = sizes.find {
                (it.width == pixelArraySize?.width && it.height == pixelArraySize.height) ||
                (it.width == preCorrectionSize?.width() && it.height == preCorrectionSize.height())
            }

            
            bestMatch ?: sizes.maxByOrNull { it.width * it.height }
        } catch (e: Exception) {
            PLog.e("CameraUtils", "Failed to get RAW capture size: ${e.message}")
            null
        }
    }
    
    
    fun formatShutterSpeed(exposureTimeNanos: Long): String {
        return when {
            exposureTimeNanos >= 1_000_000_000L -> {
                val seconds = exposureTimeNanos / 1_000_000_000.0
                String.format("%.1f\"", seconds)
            }
            else -> {
                val fraction = (1_000_000_000.0 / exposureTimeNanos).toInt()
                "1/$fraction"
            }
        }
    }
    
    
    fun formatExposureCompensation(value: Int, step: Float): String {
        val ev = value * step
        return when {
            ev > 0 -> "+${String.format("%.1f", ev)}"
            ev < 0 -> String.format("%.1f", ev)
            else -> "0"
        }
    }
}
