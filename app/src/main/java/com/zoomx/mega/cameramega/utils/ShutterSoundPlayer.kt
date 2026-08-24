package com.zoomx.mega.cameramega.utils

import android.content.Context
import android.media.AudioAttributes
import android.media.SoundPool
import android.util.Log
import java.io.IOException

class ShutterSoundPlayer(private val context: Context) {
    
    companion object {
        private const val TAG = "ShutterSoundPlayer"
        private const val SHUTTER_SOUND_PATH = "shutter.mp3"
        private const val BURST_SOUND_PATH = "burst.mp3"
    }
    
    private var soundPool: SoundPool? = null
    private var soundId: Int = -1
    private var burstSoundId: Int = -1
    private var burstStreamId: Int = -1
    private var isLoaded = false
    
    init {
        initializePlayer()
    }
    
    
    private fun initializePlayer() {
        try {
            val audioAttributes = AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_ASSISTANCE_SONIFICATION)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build()
                
            soundPool = SoundPool.Builder()
                .setMaxStreams(1)
                .setAudioAttributes(audioAttributes)
                .build()
                
            soundPool?.setOnLoadCompleteListener { _, _, status ->
                if (status == 0) {
                    isLoaded = true
                    PLog.d(TAG, "Shutter sound loaded successfully")
                } else {
                    PLog.e(TAG, "Failed to load shutter sound, status: $status")
                }
            }
            
            val afd = context.assets.openFd(SHUTTER_SOUND_PATH)
            val burstAfd = context.assets.openFd(BURST_SOUND_PATH)
            soundId = soundPool?.load(afd, 1) ?: -1
            burstSoundId = soundPool?.load(burstAfd, 1) ?: -1
            afd.close()
            
            PLog.d(TAG, "Shutter sound player initialized with SoundPool")
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to initialize shutter sound player", e)
            isLoaded = false
        }
    }
    
    
    fun play() {
        if (soundPool == null || soundId == -1 || !isLoaded) {
            PLog.w(TAG, "SoundPool not ready: soundPool=$soundPool, soundId=$soundId, isLoaded=$isLoaded")
            
            if (soundPool == null) {
                initializePlayer()
            }
            return
        }
        
        try {
            
            soundPool?.play(soundId, 1.0f, 1.0f, 1, 0, 1.0f)
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to play shutter sound", e)
        }
    }

    
    fun playBurst() {
        if (soundPool == null || burstSoundId == -1 || !isLoaded) {
            PLog.w(TAG, "SoundPool not ready: soundPool=$soundPool, burstSoundId=$soundId, isLoaded=$isLoaded")
            
            if (soundPool == null) {
                initializePlayer()
            }
            return
        }

        try {
            
            burstStreamId = soundPool?.play(burstSoundId, 1.0f, 1.0f, 1, -1, 1.0f) ?: -1
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to play shutter sound", e)
        }
    }

    fun stopBurst() {
        if (soundPool == null || burstStreamId == -1) return
        try {
            soundPool?.stop(burstStreamId)
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to stop shutter sound", e)
        }
    }
    
    
    fun release() {
        try {
            soundPool?.release()
            soundPool = null
            soundId = -1
            burstSoundId = -1
            isLoaded = false
            PLog.d(TAG, "Shutter sound player released")
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to release shutter sound player", e)
        }
    }
}
