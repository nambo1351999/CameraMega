package com.zoomx.mega.cameramega.raw

import android.graphics.Bitmap
import androidx.annotation.Keep
import java.nio.ByteBuffer

@Keep
data class DngRawData @Keep constructor(
    val rawData: ByteBuffer,
    val width: Int,
    val height: Int,
    val rowStride: Int,
    val samplesPerPixel: Int = 1,
    val whiteLevel: Float,
    val blackLevel: FloatArray,
    val preMul: FloatArray,
    val whiteBalance: FloatArray,
    val colorMatrix: FloatArray,
    val cameraWhite: FloatArray,
    val whitePointXy: FloatArray,
    val cameraMake: String,
    val cameraModel: String,
    val cfaPattern: Int, 
    val rotation: Int,
    val baselineExposure: Float,
    val shadowScale: Float = 1.0f,
    val lensShadingMap: FloatArray?,
    val lensShadingMapWidth: Int,
    val lensShadingMapHeight: Int,
    val lensShadingMapGrid: FloatArray?,
    val exposureBias: Float,
    val iso: Int,
    val shutterSpeed: Long,
    val aperture: Float,
    val activeArray: IntArray?, 
    val defaultCrop: IntArray?, 
    val noiseProfile: FloatArray?, 
    val warpRectilinear: FloatArray?, 
    val warpRectilinearFlags: IntArray?, 
    val embeddedPreview: Bitmap? = null,
) : AutoCloseable {

    @Volatile
    private var isClosed = false

    
    override fun close() {
        if (!isClosed) {
            synchronized(this) {
                if (!isClosed) {
                    freeNativeBuffer(rawData)
                    isClosed = true
                }
            }
        }
    }

    
    private external fun freeNativeBuffer(buffer: ByteBuffer)

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as DngRawData

        if (rawData != other.rawData) return false
        if (width != other.width) return false
        if (height != other.height) return false
        if (rowStride != other.rowStride) return false
        if (samplesPerPixel != other.samplesPerPixel) return false
        if (whiteLevel != other.whiteLevel) return false
        if (!blackLevel.contentEquals(other.blackLevel)) return false
        if (!preMul.contentEquals(other.preMul)) return false
        if (!whiteBalance.contentEquals(other.whiteBalance)) return false
        if (!colorMatrix.contentEquals(other.colorMatrix)) return false
        if (!cameraWhite.contentEquals(other.cameraWhite)) return false
        if (!whitePointXy.contentEquals(other.whitePointXy)) return false
        if (cameraMake != other.cameraMake) return false
        if (cameraModel != other.cameraModel) return false
        if (cfaPattern != other.cfaPattern) return false
        if (rotation != other.rotation) return false
        if (baselineExposure != other.baselineExposure) return false
        if (shadowScale != other.shadowScale) return false
        if (lensShadingMap != null) {
            if (other.lensShadingMap == null) return false
            if (!lensShadingMap.contentEquals(other.lensShadingMap)) return false
        } else if (other.lensShadingMap != null) return false
        if (lensShadingMapWidth != other.lensShadingMapWidth) return false
        if (lensShadingMapHeight != other.lensShadingMapHeight) return false
        if (lensShadingMapGrid != null) {
            if (other.lensShadingMapGrid == null) return false
            if (!lensShadingMapGrid.contentEquals(other.lensShadingMapGrid)) return false
        } else if (other.lensShadingMapGrid != null) return false
        if (defaultCrop != null) {
            if (other.defaultCrop == null) return false
            if (!defaultCrop.contentEquals(other.defaultCrop)) return false
        } else if (other.defaultCrop != null) return false
        if (warpRectilinear != null) {
            if (other.warpRectilinear == null) return false
            if (!warpRectilinear.contentEquals(other.warpRectilinear)) return false
        } else if (other.warpRectilinear != null) return false
        if (warpRectilinearFlags != null) {
            if (other.warpRectilinearFlags == null) return false
            if (!warpRectilinearFlags.contentEquals(other.warpRectilinearFlags)) return false
        } else if (other.warpRectilinearFlags != null) return false
        if (embeddedPreview != null) {
            if (other.embeddedPreview == null) return false
            if (!embeddedPreview.sameAs(other.embeddedPreview)) return false
        } else if (other.embeddedPreview != null) return false

        return true
    }

    override fun hashCode(): Int {
        var result = rawData.hashCode()
        result = 31 * result + width
        result = 31 * result + height
        result = 31 * result + rowStride
        result = 31 * result + samplesPerPixel
        result = 31 * result + whiteLevel.hashCode()
        result = 31 * result + blackLevel.contentHashCode()
        result = 31 * result + preMul.contentHashCode()
        result = 31 * result + whiteBalance.contentHashCode()
        result = 31 * result + colorMatrix.contentHashCode()
        result = 31 * result + cameraWhite.contentHashCode()
        result = 31 * result + whitePointXy.contentHashCode()
        result = 31 * result + cameraMake.hashCode()
        result = 31 * result + cameraModel.hashCode()
        result = 31 * result + cfaPattern
        result = 31 * result + rotation
        result = 31 * result + baselineExposure.hashCode()
        result = 31 * result + shadowScale.hashCode()
        result = 31 * result + (lensShadingMap?.contentHashCode() ?: 0)
        result = 31 * result + lensShadingMapWidth
        result = 31 * result + lensShadingMapHeight
        result = 31 * result + (lensShadingMapGrid?.contentHashCode() ?: 0)
        result = 31 * result + (defaultCrop?.contentHashCode() ?: 0)
        result = 31 * result + (warpRectilinear?.contentHashCode() ?: 0)
        result = 31 * result + (warpRectilinearFlags?.contentHashCode() ?: 0)
        result = 31 * result + (embeddedPreview?.hashCode() ?: 0)
        return result
    }

    protected fun finalize() {
        
        
        if (!isClosed) {
            close()
        }
    }
}
