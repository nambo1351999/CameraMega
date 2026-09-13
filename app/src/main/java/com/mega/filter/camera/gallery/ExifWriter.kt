package com.mega.filter.camera.gallery

import android.graphics.Bitmap
import androidx.exifinterface.media.ExifInterface
import com.mega.filter.camera.camera.CaptureInfo
import com.mega.filter.camera.utils.DeviceUtil
import com.mega.filter.camera.utils.OPPO_EXIF_USER_COMMENT
import com.mega.filter.camera.utils.PLog
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import kotlin.math.abs
import kotlin.math.roundToInt

object ExifWriter {

    private const val TAG = "ExifWriter"

    
    private val exifDateFormat = SimpleDateFormat("yyyy:MM:dd HH:mm:ss", Locale.US)

    
    fun writeExif(file: File, captureInfo: CaptureInfo) {
        try {
            val exif = ExifInterface(file)
            writeExifInternal(exif, captureInfo)
            exif.saveAttributes()
            PLog.d(
                TAG, "EXIF written to ${file.name}: ISO=${captureInfo.iso}, " +
                        "Exposure=${captureInfo.formatExposureTime()}, " +
                        "Aperture=${captureInfo.formatAperture()}, " +
                        "Focal=${captureInfo.formatFocalLength()}"
            )
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to write EXIF to ${file.name}", e)
        }
    }

    
    fun writeExif(fd: java.io.FileDescriptor, captureInfo: CaptureInfo) {
        try {
            val exif = ExifInterface(fd)
            writeExifInternal(exif, captureInfo)
            exif.saveAttributes()
            PLog.d(
                TAG, "EXIF written to FileDescriptor: ISO=${captureInfo.iso}, " +
                        "Exposure=${captureInfo.formatExposureTime()}, " +
                        "Aperture=${captureInfo.formatAperture()}, " +
                        "Focal=${captureInfo.formatFocalLength()}"
            )
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to write EXIF to FileDescriptor", e)
        }
    }

    fun buildExifBlock(cacheDir: File, captureInfo: CaptureInfo): ByteArray? {
        val tempFile = File(cacheDir, "temp_exif_${System.nanoTime()}.jpg")
        return try {
            val bitmap = Bitmap.createBitmap(1, 1, Bitmap.Config.ARGB_8888)
            try {
                tempFile.outputStream().use { output ->
                    bitmap.compress(Bitmap.CompressFormat.JPEG, 90, output)
                }
            } finally {
                bitmap.recycle()
            }
            writeExif(tempFile, captureInfo)
            extractJpegExifBlock(tempFile.readBytes())
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to build EXIF block", e)
            null
        } finally {
            tempFile.delete()
        }
    }

    private fun extractJpegExifBlock(jpegBytes: ByteArray): ByteArray? {
        var offset = 2
        if (jpegBytes.size < offset || jpegBytes[0] != 0xFF.toByte() || jpegBytes[1] != 0xD8.toByte()) {
            return null
        }

        while (offset + 4 <= jpegBytes.size) {
            if (jpegBytes[offset] != 0xFF.toByte()) return null
            val marker = jpegBytes[offset + 1].toInt() and 0xFF
            if (marker == 0xDA || marker == 0xD9) return null

            val segmentLength = ((jpegBytes[offset + 2].toInt() and 0xFF) shl 8) or
                    (jpegBytes[offset + 3].toInt() and 0xFF)
            if (segmentLength < 2 || offset + 2 + segmentLength > jpegBytes.size) return null

            val payloadOffset = offset + 4
            val payloadLength = segmentLength - 2
            if (marker == 0xE1 && payloadLength >= EXIF_HEADER.size) {
                val hasExifHeader = EXIF_HEADER.indices.all { index ->
                    jpegBytes[payloadOffset + index] == EXIF_HEADER[index]
                }
                if (hasExifHeader) {
                    return jpegBytes.copyOfRange(payloadOffset, payloadOffset + payloadLength)
                }
            }
            offset += 2 + segmentLength
        }
        return null
    }

    
    private fun writeExifInternal(exif: ExifInterface, captureInfo: CaptureInfo) {
        try {
            
            exif.setAttribute(ExifInterface.TAG_MAKE, captureInfo.make)
            exif.setAttribute(ExifInterface.TAG_MODEL, captureInfo.model)
            DeviceUtil.buildExifLensModel(
                focalLength35mm = captureInfo.focalLength35mm,
                aperture = captureInfo.aperture,
                model = captureInfo.model,
            )?.let { lensModel ->
                exif.setAttribute(ExifInterface.TAG_LENS_MODEL, lensModel)
            }
            if (DeviceUtil.isOppo) {
                exif.setAttribute(ExifInterface.TAG_USER_COMMENT, OPPO_EXIF_USER_COMMENT)
            }
            exif.setAttribute(ExifInterface.TAG_SOFTWARE, captureInfo.software)

            
            val dateTime = exifDateFormat.format(Date(captureInfo.captureTime))
            exif.setAttribute(ExifInterface.TAG_DATETIME, dateTime)
            exif.setAttribute(ExifInterface.TAG_DATETIME_ORIGINAL, dateTime)
            exif.setAttribute(ExifInterface.TAG_DATETIME_DIGITIZED, dateTime)

            
            
            
            exif.setAttribute(ExifInterface.TAG_ORIENTATION, ExifInterface.ORIENTATION_NORMAL.toString())

            
            if (captureInfo.imageWidth > 0) {
                exif.setAttribute(ExifInterface.TAG_IMAGE_WIDTH, captureInfo.imageWidth.toString())
                exif.setAttribute(ExifInterface.TAG_PIXEL_X_DIMENSION, captureInfo.imageWidth.toString())
            }
            if (captureInfo.imageHeight > 0) {
                exif.setAttribute(ExifInterface.TAG_IMAGE_LENGTH, captureInfo.imageHeight.toString())
                exif.setAttribute(ExifInterface.TAG_PIXEL_Y_DIMENSION, captureInfo.imageHeight.toString())
            }

            
            captureInfo.exposureTime?.let { exposureNs ->
                
                val exposureSec = exposureNs / 1_000_000_000.0
                if (exposureSec >= 1.0) {
                    
                    exif.setAttribute(ExifInterface.TAG_EXPOSURE_TIME, exposureSec.toString())
                } else {
                    
                    val denominator = (1.0 / exposureSec).toLong()
                    exif.setAttribute(ExifInterface.TAG_EXPOSURE_TIME, "1/$denominator")
                }
            }

            captureInfo.iso?.let { iso ->
                exif.setAttribute(ExifInterface.TAG_ISO_SPEED_RATINGS, iso.toString())
                
                exif.setAttribute(ExifInterface.TAG_PHOTOGRAPHIC_SENSITIVITY, iso.toString())
            }

            captureInfo.aperture?.let { fNumber ->
                
                exif.setAttribute(ExifInterface.TAG_F_NUMBER, formatRational(fNumber))
                exif.setAttribute(ExifInterface.TAG_APERTURE_VALUE, formatApexAperture(fNumber))
            }

            captureInfo.focalLength?.let { fl ->
                
                exif.setAttribute(ExifInterface.TAG_FOCAL_LENGTH, formatRational(fl))
            }

            captureInfo.focalLength35mm?.let { fl35 ->
                exif.setAttribute(ExifInterface.TAG_FOCAL_LENGTH_IN_35MM_FILM, fl35.toString())
            }

            captureInfo.exposureBias?.takeIf { it.isFinite() }?.let { exposureBias ->
                exif.setAttribute(
                    ExifInterface.TAG_EXPOSURE_BIAS_VALUE,
                    formatSignedRational(exposureBias),
                )
            }

            
            captureInfo.whiteBalance?.let { wb ->
                
                val whiteBalanceValue = if (wb == 0) 0 else 1
                exif.setAttribute(ExifInterface.TAG_WHITE_BALANCE, whiteBalanceValue.toString())
            }

            
            captureInfo.flashState?.let { flash ->
                exif.setAttribute(ExifInterface.TAG_FLASH, flash.toString())
            }

            
            captureInfo.latitude?.let { lat ->
                captureInfo.longitude?.let { lng ->
                    exif.setLatLong(lat, lng)
                }
            }
            captureInfo.altitude?.let { alt ->
                exif.setAltitude(alt)
            }

            
            exif.setAttribute(ExifInterface.TAG_COLOR_SPACE, "1") 
            exif.setAttribute(ExifInterface.TAG_EXIF_VERSION, "0230") 

        } catch (e: Exception) {
            throw e
        }
    }

    
    private fun formatRational(value: Float): String {
        
        
        val multiplier = 10
        val numerator = (value * multiplier).toInt()

        
        val gcd = gcd(numerator, multiplier)
        return "${numerator / gcd}/${multiplier / gcd}"
    }

    
    private fun formatSignedRational(value: Float): String {
        val denominator = 1_000_000
        val numerator = (value * denominator).roundToInt()
        val divisor = gcd(numerator, denominator)
        return "${numerator / divisor}/${denominator / divisor}"
    }

    
    private fun formatApexAperture(fNumber: Float): String {
        val apex = 2.0 * (kotlin.math.ln(fNumber.toDouble()) / kotlin.math.ln(2.0))
        return formatRational(apex.toFloat())
    }

    
    private fun gcd(a: Int, b: Int): Int {
        var x = abs(a)
        var y = abs(b)
        while (y != 0) {
            val temp = y
            y = x % y
            x = temp
        }
        return if (x == 0) 1 else x
    }

    private val EXIF_HEADER = byteArrayOf(
        'E'.code.toByte(),
        'x'.code.toByte(),
        'i'.code.toByte(),
        'f'.code.toByte(),
        0,
        0
    )
}
