package com.zoomx.mega.cameramega.utils

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.view.OrientationEventListener
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import kotlin.math.abs

object OrientationObserver {
    private const val TAG = "OrientationObserver"
    private const val FLAT_AXIS_THRESHOLD = 7.5f

    private var sensorManager: SensorManager? = null
    private var orientationListener: OrientationEventListener? = null
    private var gravityListener: SensorEventListener? = null
    private var isNearFlat by mutableStateOf(false)

    
    var isLandscape by mutableStateOf(false)
        private set

    
    var rotationDegrees by mutableStateOf(0f)
        private set

    
    
    var continuousOrientationDegrees by mutableFloatStateOf(0f)
        private set

    
    var captureRotationDegrees by mutableStateOf(0f)
        private set

    
    fun updateOrientation(orientation: Int) {
        if (orientation == OrientationEventListener.ORIENTATION_UNKNOWN) {
            return
        }

        continuousOrientationDegrees = orientation.toFloat()

        if (isNearFlat) {
            return
        }

        
        when (orientation) {
            in 45..135 -> {
                captureRotationDegrees = 90f
                if (!isLandscape || rotationDegrees != 90f) {
                    isLandscape = true
                    rotationDegrees = 90f
                    PLog.d(TAG, "Orientation locked to landscape-right, orientation=$orientation")
                }
            }
            
            in 225..315 -> {
                captureRotationDegrees = 270f
                if (!isLandscape || rotationDegrees != 270f) {
                    isLandscape = true
                    rotationDegrees = 270f
                    PLog.d(TAG, "Orientation locked to landscape-left, orientation=$orientation")
                }
            }
            
            in 135..225 -> {
                captureRotationDegrees = 180f
                if (isLandscape || rotationDegrees != 0f) {
                    isLandscape = false
                    rotationDegrees = 0f
                    PLog.d(TAG, "Orientation locked to reverse-portrait, orientation=$orientation")
                }
            }
            
            else -> {
                captureRotationDegrees = 0f
                if (isLandscape || rotationDegrees != 0f) {
                    isLandscape = false
                    rotationDegrees = 0f
                    PLog.d(TAG, "Orientation locked to portrait, orientation=$orientation")
                }
            }
        }
    }

    fun observe(context: Context) {
        if (orientationListener != null) {
            return
        }

        sensorManager = context.applicationContext.getSystemService(Context.SENSOR_SERVICE) as SensorManager

        gravityListener = object : SensorEventListener {
            override fun onSensorChanged(event: SensorEvent?) {
                val values = event?.values ?: return
                val nearFlat = abs(values[2]) > FLAT_AXIS_THRESHOLD
                if (nearFlat != isNearFlat) {
                    isNearFlat = nearFlat

                }
            }

            override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) = Unit
        }

        sensorManager?.getDefaultSensor(Sensor.TYPE_GRAVITY)?.let { gravitySensor ->
            sensorManager?.registerListener(
                gravityListener,
                gravitySensor,
                SensorManager.SENSOR_DELAY_UI
            )
        }

        orientationListener = object : OrientationEventListener(context.applicationContext) {
            override fun onOrientationChanged(orientation: Int) {
                updateOrientation(orientation)
            }
        }
        orientationListener?.enable()
    }
}
