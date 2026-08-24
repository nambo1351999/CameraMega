package com.zoomx.mega.cameramega.livephoto

import android.media.MediaCodec
import com.zoomx.mega.cameramega.utils.PLog
import java.nio.ByteBuffer
import java.util.concurrent.ConcurrentLinkedDeque

class CircularSampleRecorder(
    private val bufferDurationMs: Long = 1500L
) {
    
    data class Sample(
        val data: ByteArray,
        val info: MediaCodec.BufferInfo
    )

    private val samples = ConcurrentLinkedDeque<Sample>()

    @Volatile
    var isRecording: Boolean = false
        private set

    
    fun startRecording() {
        samples.clear()
        isRecording = true
    }

    
    fun stopRecording() {
        isRecording = false
    }

    
    fun addSample(byteBuffer: ByteBuffer, info: MediaCodec.BufferInfo) {
        if (!isRecording) return

        

        
        val data = ByteArray(info.size)
        byteBuffer.position(info.offset)
        byteBuffer.get(data)

        
        val sampleInfo = MediaCodec.BufferInfo()
        sampleInfo.set(0, info.size, info.presentationTimeUs, info.flags)

        samples.addLast(Sample(data, sampleInfo))

        
        
        trimStorage()
    }

    private fun trimStorage() {
        if (samples.isEmpty()) return

        val lastTimestamp = samples.last().info.presentationTimeUs
        val threshold = lastTimestamp - bufferDurationMs * 1000

        while (samples.size > 1) {
            val first = samples.first()
            if (first.info.presentationTimeUs < threshold) {
                samples.pollFirst()
            } else {
                break
            }
        }
    }

    
    fun getLatestTimestamp(): Long? {
        return samples.peekLast()?.info?.presentationTimeUs
    }

    
    fun snapshot(): List<Sample> = samples.toList()

    
    fun getSamplesAfter(timestampUs: Long): List<Sample> {
        return samples.filter { it.info.presentationTimeUs > timestampUs }
    }

    
    fun clear() {
        samples.clear()
    }

    
    fun release() {
        isRecording = false
        
        
    }
}
