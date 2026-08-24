package com.zoomx.mega.cameramega.livephoto

import android.content.Context
import com.zoomx.mega.cameramega.utils.PLog
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.nio.charset.StandardCharsets

class VivoLivePhotoCreator(private val context: Context) : LivePhotoCreator {

    override fun create(
        jpegPath: String,
        videoPath: String,
        outputPath: String,
        presentationTimestampUs: Long
    ): Boolean {
        val timestamp = System.currentTimeMillis().toString()

        try {
            
            writeVivoFile(jpegPath, File(outputPath), timestamp)

            if (File(videoPath).exists()) {
                
                val videoOutputPath = if (outputPath.lowercase().endsWith(".jpg")) {
                    outputPath.substring(0, outputPath.length - 4) + ".mp4"
                } else if (outputPath.lowercase().endsWith(".jpeg")) {
                    outputPath.substring(0, outputPath.length - 5) + ".mp4"
                } else {
                    "$outputPath.mp4"
                }
                
                writeVivoFile(videoPath, File(videoOutputPath), timestamp)
            }

            PLog.d(TAG, "Vivo Live Photo created: $outputPath")
            return true
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to create Vivo Live Photo", e)
            return false
        }
    }

    private fun writeVivoFile(inputPath: String, outputFile: File, timestamp: String) {
        FileInputStream(inputPath).use { fis ->
            FileOutputStream(outputFile).use { fos ->
                
                fis.copyTo(fos)

                
                
                val vivoJson = String.format(
                    "vivo{\"com.android.camera.joint.fullview.orientation\":0,\"com.android.camera.fisheye\":-1,\"com.android.camera.takenmodel\":\"%s\",\"com.android.camera.watermarkVersion\":null,\"com.android.camera.camerafacing\":\"0\",\"com.android.camera.moduleid\":\"live_photo\",\"com.android.camera.livephoto\":\"%s%s\",\"version\":2014,\"com.android.camera.joint.fullview\":false}",
                    "vivo X100 Pro", timestamp, String(magic3)
                )
                fos.write(vivoJson.toByteArray(StandardCharsets.US_ASCII))

                
                for (b in magic1) {
                    fos.write(b)
                }

                
                for (c in timestamp) {
                    fos.write(c.code)
                }

                
                fos.write(magic3)

                
                for (b in magic2) {
                    fos.write(b)
                }
            }
        }
    }

    companion object {
        private const val TAG = "VivoLivePhotoCreator"

        private val magic1 = intArrayOf(0, 0, 1, 118, 99, 97, 109, 101, 114, 97, 108, 98, 117, 109, 33, 0, 0, 0, 47)

        private val magic2 = intArrayOf(255, 255, 255, 255, 27, 42, 57, 72, 87, 102, 117, 132, 147, 162, 179)

        private val magic3 = byteArrayOf(54, 51, 53, 56, 53, 53, 53, 48, 48, 48, 48, 48, 48, 48, 48)

        fun isVivoLivePhoto(filePath: String): Boolean {
            val file = File(filePath)
            if (!file.exists() || file.length() < 200) return false

            return try {
                FileInputStream(file).use { fis ->
                    val fileSize = file.length()
                    
                    val readSize = (1024 * 1024).toLong().coerceAtMost(fileSize).toInt()
                    if (fileSize > readSize) {
                        fis.skip(fileSize - readSize)
                    }
                    val buffer = ByteArray(readSize)
                    var bytesRead = 0
                    while (bytesRead < readSize) {
                        val n = fis.read(buffer, bytesRead, readSize - bytesRead)
                        if (n == -1) break
                        bytesRead += n
                    }

                    
                    val sig = "vivo{\"".toByteArray(StandardCharsets.US_ASCII)
                    val livePhotoField = "\"com.android.camera.livephoto\":\"".toByteArray(StandardCharsets.US_ASCII)
                    val albumMarker = "cameralbum!".toByteArray(StandardCharsets.US_ASCII)
                    val magic2Marker = "*9HW".toByteArray(StandardCharsets.US_ASCII)

                    
                    val sigIndex = findBytes(buffer, sig)
                    if (sigIndex == -1) return false

                    val fieldIndex = findBytes(buffer, livePhotoField, sigIndex)
                    if (fieldIndex == -1) return false

                    val albumIndex = findBytes(buffer, albumMarker, fieldIndex)
                    if (albumIndex == -1) return false

                    val m2Index = findBytes(buffer, magic2Marker, albumIndex)
                    m2Index != -1
                }
            } catch (e: Exception) {
                false
            }
        }

        fun isVivoPhoto(filePath: String): Boolean {
            return extractVivoMetadata(filePath) != null
        }

        fun extractVivoMetadata(filePath: String): ByteArray? {
            val file = File(filePath)
            if (!file.exists() || file.length() < 200) return null

            return try {
                FileInputStream(file).use { fis ->
                    val fileSize = file.length()
                    
                    val readSize = (1024 * 1024).toLong().coerceAtMost(fileSize).toInt()
                    val offset = (fileSize - readSize).coerceAtLeast(0)
                    if (offset > 0) {
                        fis.skip(offset)
                    }
                    val buffer = ByteArray(readSize)
                    var bytesRead = 0
                    while (bytesRead < readSize) {
                        val n = fis.read(buffer, bytesRead, readSize - bytesRead)
                        if (n == -1) break
                        bytesRead += n
                    }

                    
                    val sig = "vivo{\"".toByteArray(StandardCharsets.US_ASCII)
                    val albumMarker = "cameralbum!".toByteArray(StandardCharsets.US_ASCII)
                    val magic2Marker = "*9HW".toByteArray(StandardCharsets.US_ASCII)

                    
                    val sigIndex = findBytes(buffer, sig)
                    if (sigIndex == -1) return null

                    val albumIndex = findBytes(buffer, albumMarker, sigIndex)
                    if (albumIndex == -1) return null

                    val m2Index = findBytes(buffer, magic2Marker, albumIndex)
                    if (m2Index == -1) return null

                    
                    val metadataSize = bytesRead - sigIndex
                    val metadata = ByteArray(metadataSize)
                    System.arraycopy(buffer, sigIndex, metadata, 0, metadataSize)
                    metadata
                }
            } catch (e: Exception) {
                null
            }
        }

        private fun findBytes(data: ByteArray, pattern: ByteArray, start: Int = 0): Int {
            if (pattern.isEmpty() || data.size - start < pattern.size) return -1
            for (i in start..(data.size - pattern.size)) {
                var found = true
                for (j in pattern.indices) {
                    if (data[i + j] != pattern[j]) {
                        found = false
                        break
                    }
                }
                if (found) return i
            }
            return -1
        }
    }
}
