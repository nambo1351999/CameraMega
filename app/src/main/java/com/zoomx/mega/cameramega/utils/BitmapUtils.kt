package com.zoomx.mega.cameramega.utils

import android.graphics.*
import android.media.Image
import android.util.Log
import androidx.exifinterface.media.ExifInterface
import com.zoomx.mega.cameramega.camera.AspectRatio
import java.io.ByteArrayInputStream
import java.io.ByteArrayOutputStream
import kotlin.math.floor

object BitmapUtils {
    private const val TAG = "BitmapUtils"

    
    fun getBitmap(byteArray: ByteArray): Bitmap {
        return BitmapFactory.decodeByteArray(byteArray, 0, byteArray.size)
    }

    fun imageToBitmapAndRotate(image: Image, aspectRatio: AspectRatio): Bitmap {
        val plane = image.planes[0]
        val buffer = plane.buffer
        val bytes = ByteArray(buffer.remaining())
        buffer.get(bytes)

        val orientation = try {
            val exif = ExifInterface(ByteArrayInputStream(bytes))
            exif.getAttributeInt(ExifInterface.TAG_ORIENTATION, ExifInterface.ORIENTATION_NORMAL)
        } catch (e: Exception) {
            ExifInterface.ORIENTATION_NORMAL
        }

        return cropAndRotate(bytes, aspectRatio, orientation)
    }

    
    fun cropAndRotate(bytes: ByteArray, aspectRatio: AspectRatio, orientation: Int): Bitmap {
        val options = BitmapFactory.Options().apply { inJustDecodeBounds = true }
        BitmapFactory.decodeByteArray(bytes, 0, bytes.size, options)
        val width = options.outWidth
        val height = options.outHeight
        
        val matrix = Matrix()
        var isSwapped = false

        when (orientation) {
            ExifInterface.ORIENTATION_ROTATE_90 -> {
                matrix.postRotate(90f)
                isSwapped = true
            }

            ExifInterface.ORIENTATION_ROTATE_180 -> matrix.postRotate(180f)
            ExifInterface.ORIENTATION_ROTATE_270 -> {
                matrix.postRotate(270f)
                isSwapped = true
            }

            ExifInterface.ORIENTATION_FLIP_HORIZONTAL -> matrix.postScale(-1f, 1f)
            ExifInterface.ORIENTATION_FLIP_VERTICAL -> matrix.postScale(1f, -1f)
            ExifInterface.ORIENTATION_TRANSPOSE -> {
                matrix.postRotate(90f)
                matrix.postScale(-1f, 1f)
                isSwapped = true
            }

            ExifInterface.ORIENTATION_TRANSVERSE -> {
                matrix.postRotate(270f)
                matrix.postScale(-1f, 1f)
                isSwapped = true
            }
        }

        
        val visualWidth = if (isSwapped) height else width
        val visualHeight = if (isSwapped) width else height

        
        val visualIsLandscape = visualHeight <= visualWidth
        val targetRatio = aspectRatio.getValue(visualIsLandscape)

        var finalVisualW: Int
        var finalVisualH: Int

        val currentRatio = visualWidth.toFloat() / visualHeight.toFloat()
        if (currentRatio > targetRatio) {
            
            finalVisualH = visualHeight
            finalVisualW = (visualHeight * targetRatio).toInt()
        } else {
            
            finalVisualW = visualWidth
            finalVisualH = (visualWidth / targetRatio).toInt()
        }

        
        val cropRawWidth = if (isSwapped) finalVisualH else finalVisualW
        val cropRawHeight = if (isSwapped) finalVisualW else finalVisualH

        val x = (width - cropRawWidth) / 2
        val y = (height - cropRawHeight) / 2

        
        val safeX = x.coerceAtLeast(0)
        val safeY = y.coerceAtLeast(0)
        val safeW = cropRawWidth.coerceAtMost(width - safeX)
        val safeH = cropRawHeight.coerceAtMost(height - safeY)

        val cropRect = Rect(safeX, safeY, safeX + safeW, safeY + safeH)

        
        var decoder: BitmapRegionDecoder? = null
        var rawCroppedBitmap: Bitmap
        var finalBitmap: Bitmap

        return try {
            
            decoder = BitmapRegionDecoder.newInstance(bytes, 0, bytes.size, false)
            
            rawCroppedBitmap = decoder.decodeRegion(cropRect, null)
            
            if (matrix.isIdentity) {
                finalBitmap = rawCroppedBitmap
            } else {
                finalBitmap = Bitmap.createBitmap(
                    rawCroppedBitmap, 0, 0,
                    rawCroppedBitmap.width, rawCroppedBitmap.height,
                    matrix, true
                )
                
                if (rawCroppedBitmap != finalBitmap) {
                    rawCroppedBitmap.recycle()
                }
            }
            finalBitmap
        } catch (e: Exception) {
            e.printStackTrace()
            BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
        } finally {
            decoder?.recycle()
        }
    }

    
    fun flipHorizontal(bitmap: Bitmap): Bitmap {
        val matrix = Matrix().apply { postScale(-1f, 1f) }
        return Bitmap.createBitmap(bitmap, 0, 0, bitmap.width, bitmap.height, matrix, true)
    }

    
    fun rotate(bitmap: Bitmap, rotationDegrees: Float): Bitmap {
        val normalizedDegrees = ((rotationDegrees % 360) + 360) % 360
        if (normalizedDegrees == 0f) {
            return bitmap
        }

        val matrix = Matrix().apply { postRotate(normalizedDegrees) }
        val rotated = Bitmap.createBitmap(bitmap, 0, 0, bitmap.width, bitmap.height, matrix, true)
        return rotated
    }

    fun Bitmap.toByteArray(): ByteArray {
        val stream = ByteArrayOutputStream()
        this.compress(Bitmap.CompressFormat.JPEG, 100, stream)
        return stream.toByteArray()
    }

    
    fun calculateProcessedRect(
        width: Int,
        height: Int,
        aspectRatio: AspectRatio?,
        cropRegion: Rect?,
        rotation: Int = 0
    ): Rect {
        val bitmapBounds = Rect(0, 0, width, height)

        
        val currentIsLandscape = width >= height
        val safeRegion = if (cropRegion != null && !cropRegion.isEmpty) {
            val regionIsLandscape = cropRegion.width() >= cropRegion.height()
            val alignedRegion = if (regionIsLandscape != currentIsLandscape) {
                
                Rect(cropRegion.top, cropRegion.left, cropRegion.bottom, cropRegion.right)
            } else {
                Rect(cropRegion)
            }
            
            if (!alignedRegion.intersect(bitmapBounds)) {
                bitmapBounds
            } else {
                alignedRegion
            }
        } else {
            bitmapBounds
        }

        
        val cropIsLandscape = safeRegion.width() >= safeRegion.height()
        val targetRatio = aspectRatio?.getValue(cropIsLandscape) ?: (safeRegion.width().toFloat() / safeRegion.height().toFloat())

        
        val baseWidth = safeRegion.width()
        val baseHeight = safeRegion.height()
        val srcRatio = baseWidth.toFloat() / baseHeight.toFloat()

        var finalW: Float
        var finalH: Float

        if (srcRatio > targetRatio) {
            
            finalH = baseHeight.toFloat()
            finalW = baseHeight * targetRatio
        } else {
            
            finalW = baseWidth.toFloat()
            finalH = baseWidth / targetRatio
        }

        
        val x = (safeRegion.left + (baseWidth - finalW) / 2f).toInt().coerceAtLeast(0)
        val y = (safeRegion.top + (baseHeight - finalH) / 2f).toInt().coerceAtLeast(0)
        val finalWInt = alignDownToEven(finalW.toInt().coerceAtMost(width - x))
        val finalHInt = alignDownToEven(finalH.toInt().coerceAtMost(height - y))

        
        val isSwapped = rotation == 90 || rotation == 270
        return if (isSwapped) {
            Rect(y, x, y + finalHInt, x + finalWInt)
        } else {
            Rect(x, y, x + finalWInt, y + finalHInt)
        }
    }

    private fun alignDownToEven(value: Int): Int {
        if (value <= 1) return value
        return value and 1.inv()
    }
}
