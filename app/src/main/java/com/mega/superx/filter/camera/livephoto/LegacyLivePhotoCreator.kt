package com.mega.superx.filter.camera.livephoto

import com.mega.superx.filter.camera.utils.PLog
import java.io.BufferedInputStream
import java.io.BufferedOutputStream
import java.io.DataOutputStream
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.nio.charset.StandardCharsets

class LegacyLivePhotoCreator : LivePhotoCreator {
    private val TAG = "LegacyLivePhotoCreator"

    override fun create(
        jpegPath: String,
        videoPath: String,
        outputPath: String,
        presentationTimestampUs: Long
    ): Boolean {
        try {
            val videoLength = File(videoPath).length()
            
            FileOutputStream(outputPath).use { fos ->
                val bos = BufferedOutputStream(fos)
                val dos = DataOutputStream(bos)
                
                
                FileInputStream(jpegPath).use { fis ->
                    fis.copyTo(dos)
                }

                
                FileInputStream(videoPath).use { fis ->
                    fis.copyTo(dos)
                }

                
                val markerstr1 = "500:1046"
                val markerStr2 = "LIVE_$videoLength"
                val paddedMarker = markerstr1.padEnd(20) + markerStr2.padEnd(20)
                dos.write(paddedMarker.toByteArray(StandardCharsets.UTF_8))
                
                dos.flush()
            }
            return true
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to create Legacy Live Photo", e)
            return false
        }
    }
}
