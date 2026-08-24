package com.zoomx.mega.cameramega.livephoto

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.media.*
import android.opengl.*
import android.view.Surface
import androidx.core.app.ActivityCompat
import com.zoomx.mega.cameramega.lut.LutConfig
import com.zoomx.mega.cameramega.model.ColorRecipeParams
import com.zoomx.mega.cameramega.utils.PLog
import kotlinx.coroutines.*
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import java.io.File
import java.nio.ByteBuffer
import java.util.concurrent.Executors

class LivePhotoRecorder(
    private val context: Context,
    private val bufferDurationMs: Long = 1500L,
    private val postCaptureDurationMs: Long = 1500L,
    private val frameRateHz: Int = 30
) {
    companion object {
        private const val TAG = "LivePhotoRecorder"
        private const val MIME_TYPE_VIDEO = MediaFormat.MIMETYPE_VIDEO_AVC
        private const val VIDEO_BITRATE = 8_000_000
        private const val I_FRAME_INTERVAL = 1

        private const val MIME_TYPE_AUDIO = MediaFormat.MIMETYPE_AUDIO_AAC
        private const val AUDIO_SAMPLE_RATE = 44100
        private const val AUDIO_BITRATE = 64000
        private const val AUDIO_CHANNEL_COUNT = 1
    }

    
    
    private val bufferCapacity = bufferDurationMs + postCaptureDurationMs + 2000L
    private val circularVideoRecorder = CircularSampleRecorder(bufferCapacity)
    private val circularAudioRecorder = CircularSampleRecorder(bufferCapacity)

    @Volatile
    private var videoEncoder: MediaCodec? = null
    private var inputSurface: Surface? = null

    @Volatile
    private var audioEncoder: MediaCodec? = null
    private var audioRecord: AudioRecord? = null

    @Volatile
    private var videoFormat: MediaFormat? = null

    @Volatile
    private var audioFormat: MediaFormat? = null

    
    private var lutRenderer: HardwareLutVideoRenderer? = null

    
    @Volatile
    private var isRunning = false
    @Volatile
    private var isCapturing = false
    private var snapshotTimestampUs: Long = 0

    private val scope = CoroutineScope(Dispatchers.Default + SupervisorJob())
    private var videoDrainJob: Job? = null
    private var audioDrainJob: Job? = null
    private var audioRecordJob: Job? = null
    private val renderDispatcher = Executors.newSingleThreadExecutor().asCoroutineDispatcher()
    private val hardwareMutex = Mutex()

    private var lastFrameTimestampUs: Long = 0
    private var lastSharedContext: EGLContext = EGL14.EGL_NO_CONTEXT
    private var lastWidth: Int = 0
    private var lastHeight: Int = 0
    @Volatile
    private var pendingRelease = false

    
    fun updateConfig(lutConfig: LutConfig?, params: ColorRecipeParams?) {
        lutRenderer?.updateConfig(lutConfig, params)
    }

    
    fun startRecording() {
        if (isRunning) return
        circularVideoRecorder.startRecording()
        circularAudioRecorder.startRecording()
        isRunning = true
        lastFrameTimestampUs = 0
        PLog.d(TAG, "Live Photo recording prepared")
    }

    
    fun stopRecording() {
        isRunning = false
        circularVideoRecorder.stopRecording()
        circularAudioRecorder.stopRecording()
        if (isCapturing) {
            PLog.d(TAG, "Stop requested while capture in progress, delaying encoder release")
            pendingRelease = true
        } else {
            release()
        }
        PLog.d(TAG, "Live Photo recording stopped")
    }

    
    fun onPreviewFrame(
        textureId: Int,
        transformMatrix: FloatArray,
        width: Int,
        height: Int,
        timestampNs: Long,
        lutConfig: LutConfig?,
        params: ColorRecipeParams?,
        sharedContext: EGLContext,
        sharedDisplay: EGLDisplay
    ) {
        if (!isRunning) return

        lastFrameTimestampUs = timestampNs / 1000

        val matrix = transformMatrix.clone()
        scope.launch(renderDispatcher) {
            hardwareMutex.withLock {
                if (!isRunning) return@withLock

                
                if (videoEncoder != null && (
                            (lastSharedContext != EGL14.EGL_NO_CONTEXT && lastSharedContext != sharedContext) ||
                                    (lastWidth != width || lastHeight != height)
                            )
                ) {
                    PLog.w(TAG, "Shared EGL Context or dimensions changed, forcing encoder re-init.")
                    releaseHardwareLocked()
                }

                if (!isRunning) return@withLock

                
                if (videoEncoder == null && sharedContext != EGL14.EGL_NO_CONTEXT) {
                    initEncoder(width, height, lutConfig, params, sharedContext, sharedDisplay)
                    lastSharedContext = sharedContext
                    lastWidth = width
                    lastHeight = height
                    pendingRelease = false
                }

                if (!isRunning) return@withLock

                try {
                    lutRenderer?.renderFrame(textureId, matrix, timestampNs / 1000)
                } catch (e: Exception) {
                }
            }
        }
    }

    private fun initEncoder(
        width: Int,
        height: Int,
        lutConfig: LutConfig?,
        params: ColorRecipeParams?,
        sharedContext: EGLContext,
        sharedDisplay: EGLDisplay
    ) {
        try {
            var w = width
            var h = height

            
            if (w % 2 != 0) w--
            if (h % 2 != 0) h--

            
            val videoFormat = MediaFormat.createVideoFormat(MIME_TYPE_VIDEO, w, h).apply {
                setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface)
                setInteger(MediaFormat.KEY_BIT_RATE, VIDEO_BITRATE)
                setInteger(MediaFormat.KEY_FRAME_RATE, frameRateHz)
                setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, I_FRAME_INTERVAL)
            }

            videoEncoder = MediaCodec.createEncoderByType(MIME_TYPE_VIDEO).apply {
                configure(videoFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
                inputSurface = createInputSurface()
                start()
            }

            
            initAudio()

            lutRenderer = HardwareLutVideoRenderer(w, h, lutConfig, params).apply {
                initialize(inputSurface!!, sharedContext, sharedDisplay)
            }

            startDraining()
            PLog.d(TAG, "Encoders initialized: ${w}x${h}")
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to init encoders", e)
        }
    }

    private fun initAudio() {
        try {
            val bufferSize = AudioRecord.getMinBufferSize(
                AUDIO_SAMPLE_RATE,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT
            )
            if (bufferSize <= 0) return

            if (ActivityCompat.checkSelfPermission(
                    context,
                    Manifest.permission.RECORD_AUDIO
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                return
            }
            audioRecord = AudioRecord(
                MediaRecorder.AudioSource.CAMCORDER,
                AUDIO_SAMPLE_RATE,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize * 2
            ).apply {
                if (state == AudioRecord.STATE_INITIALIZED) {
                    startRecording()
                } else {
                    PLog.e(TAG, "AudioRecord state not initialized")
                    return
                }
            }

            val format = MediaFormat.createAudioFormat(MIME_TYPE_AUDIO, AUDIO_SAMPLE_RATE, AUDIO_CHANNEL_COUNT).apply {
                setInteger(MediaFormat.KEY_BIT_RATE, AUDIO_BITRATE)
                setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC)
            }

            audioEncoder = MediaCodec.createEncoderByType(MIME_TYPE_AUDIO).apply {
                configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
                start()
            }

            startAudioRecordingLoop()
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to init audio", e)
        }
    }

    private fun startAudioRecordingLoop() {
        audioRecordJob = scope.launch {
            val buffer = ByteArray(2048)
            while (isActive && isRunning) {
                try {
                    val record = audioRecord ?: break
                    val encoder = audioEncoder ?: break
                    val len = record.read(buffer, 0, buffer.size)
                    if (len > 0 && isRunning) {
                        val inputBufferIndex = try {
                            encoder.dequeueInputBuffer(10_000L)
                        } catch (e: IllegalStateException) {
                            break
                        }
                        if (!isActive || !isRunning) break
                        if (inputBufferIndex >= 0) {
                            val inputBuffer = encoder.getInputBuffer(inputBufferIndex)
                            inputBuffer?.clear()
                            if (inputBuffer == null || inputBuffer.remaining() < len) {
                                PLog.w(
                                    TAG,
                                    "Audio input buffer too small: len=$len, remaining=${inputBuffer?.remaining() ?: -1}"
                                )
                                try {
                                    encoder.queueInputBuffer(
                                        inputBufferIndex,
                                        0,
                                        0,
                                        android.os.SystemClock.elapsedRealtimeNanos() / 1000,
                                        0
                                    )
                                } catch (e: IllegalStateException) {
                                    break
                                }
                                continue
                            }
                            inputBuffer.put(buffer, 0, len)
                            try {
                                encoder.queueInputBuffer(
                                    inputBufferIndex,
                                    0,
                                    len,
                                    android.os.SystemClock.elapsedRealtimeNanos() / 1000,
                                    0
                                )
                            } catch (e: IllegalStateException) {
                                break
                            }
                        }
                    }
                } catch (e: Exception) {
                    if (e !is IllegalStateException) {
                        PLog.e(TAG, "Error in audio recording loop", e)
                    }
                    break
                }
            }
        }
    }

    
    fun snapshot(): List<CircularSampleRecorder.Sample> {
        if (!isRunning) return emptyList()

        
        snapshotTimestampUs = lastFrameTimestampUs
        if (snapshotTimestampUs == 0L) {
            snapshotTimestampUs = android.os.SystemClock.elapsedRealtimeNanos() / 1000
        }

        isCapturing = true
        PLog.d(TAG, "Snapshot triggered at $snapshotTimestampUs")
        return circularVideoRecorder.snapshot()
    }

    
    fun recordVideo(
        imageTimestampUs: Long? = null,
        onCaptured: (File, Long) -> Unit
    ) {
        scope.launch {
            try {
                
                delay(postCaptureDurationMs + 500L)

                val currentVideoSamples = circularVideoRecorder.snapshot()
                val currentAudioSamples = circularAudioRecorder.snapshot()
                if (currentVideoSamples.isEmpty()) {
                    PLog.e(TAG, "No video samples in buffer")
                    isCapturing = false
                    return@launch
                }

                
                val centerTs = imageTimestampUs ?: snapshotTimestampUs

                
                var startTimeUs = centerTs - bufferDurationMs * 1000
                val endTimeUs = centerTs + postCaptureDurationMs * 1000

                
                
                val firstSampleTs = currentVideoSamples.first().info.presentationTimeUs
                val lastSampleTs = currentVideoSamples.last().info.presentationTimeUs

                if (centerTs < firstSampleTs || centerTs > lastSampleTs) {
                    PLog.w(
                        TAG,
                        "Center timestamp $centerTs out of buffer range [$firstSampleTs, $lastSampleTs]. Fallback to latest available."
                    )
                    
                    startTimeUs = lastSampleTs - (bufferDurationMs + postCaptureDurationMs) * 1000
                }

                PLog.d(TAG, "Filtering samples: center=$centerTs, range=[$startTimeUs, $endTimeUs]")

                
                var startIndex = -1
                for (i in currentVideoSamples.indices.reversed()) {
                    val sample = currentVideoSamples[i]
                    if (sample.info.presentationTimeUs <= startTimeUs &&
                        (sample.info.flags and MediaCodec.BUFFER_FLAG_KEY_FRAME) != 0
                    ) {
                        startIndex = i
                        break
                    }
                }

                
                if (startIndex == -1) {
                    startIndex = currentVideoSamples.indexOfFirst {
                        (it.info.flags and MediaCodec.BUFFER_FLAG_KEY_FRAME) != 0
                    }
                }

                if (startIndex == -1 || videoFormat == null) {
                    PLog.e(TAG, "No keyframe or format to mux (format=${videoFormat != null})")
                    onCaptured(File("error"), 0L) 
                    return@launch
                }

                
                val muxVideoSamples = mutableListOf<CircularSampleRecorder.Sample>()
                for (i in startIndex until currentVideoSamples.size) {
                    val sample = currentVideoSamples[i]
                    muxVideoSamples.add(sample)
                    
                    if (sample.info.presentationTimeUs >= endTimeUs && muxVideoSamples.size > 5) break
                }

                if (muxVideoSamples.size < 2) {
                    PLog.e(TAG, "Too few video samples for video: ${muxVideoSamples.size}")
                    onCaptured(File("error"), 0L) 
                    return@launch
                }

                val finalVideoStartTs = muxVideoSamples.first().info.presentationTimeUs
                val finalVideoEndTs = muxVideoSamples.last().info.presentationTimeUs

                
                val muxAudioSamples = currentAudioSamples.filter {
                    it.info.presentationTimeUs in finalVideoStartTs..finalVideoEndTs
                }

                if (muxAudioSamples.isEmpty() && !currentAudioSamples.isEmpty()) {
                    PLog.w(
                        TAG,
                        "Audio samples exist but none match video range. Video: [$finalVideoStartTs, $finalVideoEndTs], Audio range: [${currentAudioSamples.first().info.presentationTimeUs}, ${currentAudioSamples.last().info.presentationTimeUs}]"
                    )
                }

                PLog.d(TAG, "Selected ${muxVideoSamples.size} video, ${muxAudioSamples.size} audio samples")
                PLog.d(
                    TAG,
                    "Selected range: [$finalVideoStartTs, $finalVideoEndTs], duration: ${(finalVideoEndTs - finalVideoStartTs) / 1000}ms"
                )

                val videoFile = File(context.cacheDir, "livephoto_${System.currentTimeMillis()}.mp4")

                
                muxVideo(videoFile, muxVideoSamples, muxAudioSamples)

                
                val presentationTimestampUs = centerTs - finalVideoStartTs

                onCaptured(videoFile, presentationTimestampUs)

            } catch (e: Exception) {
                PLog.e(TAG, "Failed to finish Live Photo capture", e)
                onCaptured(File("error"), 0L)
            } finally {
                isCapturing = false
                if (pendingRelease) {
                    PLog.d(TAG, "Executing delayed encoder release")
                    release()
                    pendingRelease = false
                }
            }
        }
    }

    private fun startDraining() {
        videoDrainJob = scope.launch {
            val bufferInfo = MediaCodec.BufferInfo()
            while (isActive && isRunning) {
                try {
                    val encoderRef = videoEncoder ?: break
                    val outputBufferIndex = try {
                        encoderRef.dequeueOutputBuffer(bufferInfo, 10_000L)
                    } catch (e: IllegalStateException) {
                        
                        PLog.d(TAG, "Video encoder dequeue cancelled (encoder stopped)")
                        break
                    }
                    if (!isActive || !isRunning) break

                    if (outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                        synchronized(this@LivePhotoRecorder) {
                            videoFormat = encoderRef.outputFormat
                        }
                        PLog.d(TAG, "Video encoder format changed: $videoFormat")
                    } else if (outputBufferIndex == MediaCodec.INFO_TRY_AGAIN_LATER) {
                        
                    } else if (outputBufferIndex >= 0) {
                        val outputBuffer = encoderRef.getOutputBuffer(outputBufferIndex)
                        if (outputBuffer != null && bufferInfo.size > 0) {
                            circularVideoRecorder.addSample(outputBuffer, bufferInfo)
                        }
                        encoderRef.releaseOutputBuffer(outputBufferIndex, false)
                    }
                } catch (e: Exception) {
                    PLog.e(TAG, "Error draining video encoder")
                    break
                }
            }
        }

        audioDrainJob = scope.launch {
            val bufferInfo = MediaCodec.BufferInfo()
            while (isActive && isRunning) {
                try {
                    val encoderRef = audioEncoder ?: break
                    val outputBufferIndex = try {
                        encoderRef.dequeueOutputBuffer(bufferInfo, 10_000L)
                    } catch (e: IllegalStateException) {
                        PLog.d(TAG, "Audio encoder dequeue cancelled (encoder stopped)")
                        break
                    }
                    if (!isActive || !isRunning) break

                    if (outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                        synchronized(this@LivePhotoRecorder) {
                            audioFormat = encoderRef.outputFormat
                        }
                        PLog.d(TAG, "Audio encoder format changed: $audioFormat")
                    } else if (outputBufferIndex >= 0) {
                        val outputBuffer = encoderRef.getOutputBuffer(outputBufferIndex)
                        if (outputBuffer != null && bufferInfo.size > 0) {
                            circularAudioRecorder.addSample(outputBuffer, bufferInfo)
                        }
                        encoderRef.releaseOutputBuffer(outputBufferIndex, false)
                    }
                } catch (e: IllegalStateException) {
                    PLog.w(TAG, "Error draining audio encoder", e)
                    break
                } catch (e: Exception) {
                    PLog.e(TAG, "Error draining audio encoder", e)
                    break
                }
            }
        }
    }

    private fun muxVideo(
        outputFile: File,
        videoSamples: List<CircularSampleRecorder.Sample>,
        audioSamples: List<CircularSampleRecorder.Sample>
    ) {
        val vFormat = synchronized(this) { videoFormat }
        val aFormat = synchronized(this) { audioFormat }
        if (vFormat == null) {
            PLog.e(TAG, "Cannot mux video: videoFormat is null")
            return
        }
        val muxer = MediaMuxer(outputFile.absolutePath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4)
        val videoTrackIndex = muxer.addTrack(vFormat)
        val audioTrackIndex = if (aFormat != null && audioSamples.isNotEmpty()) muxer.addTrack(aFormat) else -1
        muxer.start()

        val baseTimeUs = videoSamples.first().info.presentationTimeUs
        videoSamples.forEach { sample ->
            val info = MediaCodec.BufferInfo()
            info.set(0, sample.info.size, sample.info.presentationTimeUs - baseTimeUs, sample.info.flags)
            val buffer = ByteBuffer.wrap(sample.data)
            muxer.writeSampleData(videoTrackIndex, buffer, info)
        }

        if (audioTrackIndex != -1) {
            audioSamples.forEach { sample ->
                val info = MediaCodec.BufferInfo()
                info.set(0, sample.info.size, sample.info.presentationTimeUs - baseTimeUs, sample.info.flags)
                val buffer = ByteBuffer.wrap(sample.data)
                muxer.writeSampleData(audioTrackIndex, buffer, info)
            }
        }

        muxer.stop()
        muxer.release()
        PLog.d(TAG, "Muxed ${videoSamples.size} video and ${audioSamples.size} audio samples to ${outputFile.name}")
    }

    fun release() {
        isRunning = false
        
        scope.launch(renderDispatcher) {
            hardwareMutex.withLock {
                releaseHardwareLocked()
            }
        }
    }

    private suspend fun releaseHardwareLocked() {
        val jobs = listOfNotNull(videoDrainJob, audioDrainJob, audioRecordJob)
        videoDrainJob = null
        audioDrainJob = null
        audioRecordJob = null

        jobs.forEach { it.cancel() }
        jobs.joinAll()

        internalReleaseLocked()
    }

    private fun internalReleaseLocked() {
        videoEncoder?.let {
            try {
                it.stop()
                it.release()
            } catch (e: Exception) {
                PLog.w(TAG, "Error releasing video encoder: ${e.message}")
            }
        }
        videoEncoder = null
        inputSurface = null

        audioEncoder?.let {
            try {
                it.stop()
                it.release()
            } catch (e: Exception) {
                PLog.w(TAG, "Error releasing audio encoder: ${e.message}")
            }
        }
        audioEncoder = null

        audioRecord?.let {
            try {
                it.stop()
                it.release()
            } catch (e: Exception) {
                PLog.w(TAG, "Error releasing audio record: ${e.message}")
            }
        }
        audioRecord = null

        lutRenderer?.release()
        lutRenderer = null

        
        
        
        

        lastSharedContext = EGL14.EGL_NO_CONTEXT
        PLog.d(TAG, "Live Photo hardware resources released")
    }

    
    fun isRecording(): Boolean = isRunning
    fun isCapturing(): Boolean = isCapturing
}
