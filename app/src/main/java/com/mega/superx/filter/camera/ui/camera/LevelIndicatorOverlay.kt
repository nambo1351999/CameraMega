package com.mega.superx.filter.camera.ui.camera

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.withTransform
import androidx.compose.ui.platform.LocalContext
import com.mega.superx.filter.camera.utils.PLog
import kotlin.math.abs
import kotlin.math.atan2

private const val TAG = "LevelIndicatorOverlay"
private const val LevelThresholdDegrees = 3.0f
private const val DefaultLevelAspectRatio = 3f / 4f

@Composable
fun LevelIndicatorOverlay(
    aspectRatio: Float,
    modifier: Modifier = Modifier
) {
    val context = LocalContext.current

    var targetRotation by remember { mutableFloatStateOf(0f) }
    var isLevel by remember { mutableStateOf(false) }

    
    val animatedRotation by animateFloatAsState(
        targetValue = targetRotation,
        animationSpec = tween(durationMillis = 200),
        label = "rotation"
    )

    val lineColor by animateColorAsState(
        targetValue = if (isLevel) Color(0xFF00FF00) else Color.White.copy(alpha = 0.8f),
        animationSpec = tween(durationMillis = 300),
        label = "color"
    )

    DisposableEffect(Unit) {
        val sensorManager = context.getSystemService(Context.SENSOR_SERVICE) as SensorManager
        val gravitySensor = sensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY)
        var invalidSensorReadingLogged = false

        val listener = object : SensorEventListener {
            override fun onSensorChanged(event: SensorEvent?) {
                val reading = event?.toLevelSensorReading()
                if (reading == null) {
                    if (!invalidSensorReadingLogged) {
                        invalidSensorReadingLogged = true
                        PLog.w(TAG, "Ignored invalid gravity sensor reading: ${event.describeLevelSensorValues()}")
                    }
                    isLevel = false
                    return
                }

                isLevel = reading.isLevel
                targetRotation = reading.angleDegrees
            }

            override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {}
        }

        gravitySensor?.let {
            sensorManager.registerListener(listener, it, SensorManager.SENSOR_DELAY_UI)
        } ?: PLog.w(TAG, "Gravity sensor unavailable; level indicator is disabled")
        onDispose { sensorManager.unregisterListener(listener) }
    }

    Canvas(modifier = modifier.fillMaxSize()) {
        val canvasWidth = size.width
        val canvasHeight = size.height

        
        val containerRatio = DefaultLevelAspectRatio
        val targetRatio = aspectRatio.validLevelAspectRatioOrDefault()
        val (drawWidth, drawHeight, offsetX, offsetY) = if (targetRatio > containerRatio) {
            val h = canvasWidth / targetRatio
            Quadruple(canvasWidth, h, 0f, (canvasHeight - h) / 2f)
        } else {
            val w = canvasHeight * targetRatio
            Quadruple(w, canvasHeight, (canvasWidth - w) / 2f, 0f)
        }
        val centerX = offsetX + drawWidth / 2f
        val centerY = offsetY + drawHeight / 2f

        

        
        val lineLength = drawWidth * 0.4f
        val strokeWidth = 6f 

        
        withTransform({
            rotate(degrees = animatedRotation, pivot = Offset(centerX, centerY))
        }) {
            
            drawLine(
                color = lineColor,
                start = Offset(centerX - lineLength / 2f, centerY),
                end = Offset(centerX + lineLength / 2f, centerY),
                strokeWidth = strokeWidth,
                cap = StrokeCap.Round
            )

            
            if (isLevel) {
                drawCircle(
                    color = lineColor,
                    radius = 8f,
                    center = Offset(centerX, centerY)
                )
            }
        }

        
        if (!isLevel) {
            drawCircle(
                color = Color.White.copy(alpha = 0.3f),
                radius = 4f,
                center = Offset(centerX, centerY)
            )

            
            val gap = lineLength / 2f + 15f
            val markerLen = 15f
            
            drawLine(
                color = Color.White.copy(alpha = 0.3f),
                start = Offset(centerX - gap - markerLen, centerY),
                end = Offset(centerX - gap, centerY),
                strokeWidth = 4f,
                cap = StrokeCap.Round
            )
            
            drawLine(
                color = Color.White.copy(alpha = 0.3f),
                start = Offset(centerX + gap, centerY),
                end = Offset(centerX + gap + markerLen, centerY),
                strokeWidth = 4f,
                cap = StrokeCap.Round
            )
        }
    }
}

data class Quadruple<out A, out B, out C, out D>(
    val first: A, val second: B, val third: C, val fourth: D
)

private data class LevelSensorReading(
    val angleDegrees: Float,
    val isLevel: Boolean
)

private fun SensorEvent.toLevelSensorReading(): LevelSensorReading? {
    val x = values.getOrNull(0)?.takeIf { it.isFiniteValue() } ?: return null
    val y = values.getOrNull(1)?.takeIf { it.isFiniteValue() } ?: return null

    
    val angleDegrees = Math.toDegrees(atan2(x.toDouble(), y.toDouble())).toFloat()
        .takeIf { it.isFiniteValue() }
        ?: return null

    
    val deviation = abs(angleDegrees % 90f)
    val realDeviation = if (deviation > 45f) 90f - deviation else deviation

    return LevelSensorReading(
        angleDegrees = angleDegrees,
        isLevel = realDeviation < LevelThresholdDegrees
    )
}

private fun SensorEvent?.describeLevelSensorValues(): String {
    val x = this?.values?.getOrNull(0)
    val y = this?.values?.getOrNull(1)
    val z = this?.values?.getOrNull(2)
    return "x=$x, y=$y, z=$z"
}

private fun Float.validLevelAspectRatioOrDefault(): Float {
    return takeIf { it.isFiniteValue() && it > 0f } ?: DefaultLevelAspectRatio
}

private fun Float.isFiniteValue(): Boolean {
    return !isNaN() && !isInfinite()
}
