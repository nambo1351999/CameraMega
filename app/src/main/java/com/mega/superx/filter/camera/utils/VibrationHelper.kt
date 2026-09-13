package com.mega.superx.filter.camera.utils

import android.content.Context
import android.os.Build
import android.os.VibrationEffect
import android.os.Vibrator
import android.os.VibratorManager

class VibrationHelper(private val context: Context) {

    companion object {
        private const val TAG = "VibrationHelper"
        private const val VIBRATION_DURATION_MS = 50L 
    }

    private val vibrator: Vibrator? by lazy {
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                val vibratorManager = context.getSystemService(Context.VIBRATOR_MANAGER_SERVICE) as? VibratorManager
                vibratorManager?.defaultVibrator
            } else {
                @Suppress("DEPRECATION")
                context.getSystemService(Context.VIBRATOR_SERVICE) as? Vibrator
            }
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to get vibrator service", e)
            null
        }
    }

    
    fun vibrate() {
        try {
            vibrator?.let { vib ->
                if (!vib.hasVibrator()) {
                    PLog.w(TAG, "Device does not have vibrator")
                    return
                }

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    
                    val effect = VibrationEffect.createOneShot(
                        VIBRATION_DURATION_MS,
                        VibrationEffect.DEFAULT_AMPLITUDE
                    )
                    vib.vibrate(effect)
                } else {
                    
                    @Suppress("DEPRECATION")
                    vib.vibrate(VIBRATION_DURATION_MS)
                }

                PLog.d(TAG, "Vibration triggered")
            } ?: run {
                PLog.w(TAG, "Vibrator not available")
            }
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to vibrate", e)
        }
    }

    
    fun cancel() {
        try {
            vibrator?.cancel()
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to cancel vibration", e)
        }
    }
}
