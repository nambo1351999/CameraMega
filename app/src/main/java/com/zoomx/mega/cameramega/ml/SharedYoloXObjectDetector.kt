package com.zoomx.mega.cameramega.ml

import android.content.Context
import com.zoomx.mega.cameramega.data.AiFocusTargetMode
import kotlinx.coroutines.asCoroutineDispatcher
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext
import java.util.concurrent.Executors

object SharedYoloXObjectDetector {
    private val mutex = Mutex()
    
    private val detectorDispatcher = Executors.newSingleThreadExecutor { runnable ->
        Thread(runnable, "SharedYoloXObjectDetector").apply {
            isDaemon = true
        }
    }.asCoroutineDispatcher()
    private var detector: YoloXObjectDetector? = null

    suspend fun prewarm(context: Context) {
        withDetector(context) { }
    }

    suspend fun detect(
        context: Context,
        bitmap: android.graphics.Bitmap,
        targetMode: AiFocusTargetMode,
        scoreThreshold: Float,
    ): List<YoloXObjectDetector.Detection> {
        return withDetector(context) { detector ->
            detector.targetMode = targetMode
            detector.scoreThreshold = scoreThreshold.coerceIn(0.05f, 0.95f)
            detector.detect(bitmap)
        }
    }

    suspend fun release() {
        withContext(detectorDispatcher) {
            mutex.withLock {
                detector?.close()
                detector = null
            }
        }
    }

    private suspend fun <T> withDetector(
        context: Context,
        block: (YoloXObjectDetector) -> T,
    ): T = withContext(detectorDispatcher) {
        mutex.withLock {
            val activeDetector = detector ?: YoloXObjectDetector(context.applicationContext).also {
                detector = it
            }
            block(activeDetector)
        }
    }
}
