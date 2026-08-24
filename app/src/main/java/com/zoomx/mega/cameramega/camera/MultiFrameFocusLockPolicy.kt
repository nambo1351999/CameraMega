package com.zoomx.mega.cameramega.camera

import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.CaptureResult

internal object MultiFrameFocusLockPolicy {
    fun isReadyForCapture(afState: Int?, lensState: Int?): Boolean {
        val afLocked = afState == CaptureResult.CONTROL_AF_STATE_FOCUSED_LOCKED ||
            afState == CaptureResult.CONTROL_AF_STATE_NOT_FOCUSED_LOCKED
        val lensStationary = lensState == null || lensState == CaptureResult.LENS_STATE_STATIONARY
        return afLocked && lensStationary
    }

    
    fun canFreezeSettledContinuousFocus(
        afMode: Int,
        afState: Int?,
        lensState: Int?,
        focusDistanceDiopters: Float?,
        supportsAfOff: Boolean,
    ): Boolean {
        val continuousAf = afMode == CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE ||
            afMode == CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_VIDEO
        val passiveAttemptFinished = afState == CaptureResult.CONTROL_AF_STATE_PASSIVE_FOCUSED ||
            afState == CaptureResult.CONTROL_AF_STATE_PASSIVE_UNFOCUSED
        val lensStationary = lensState == null || lensState == CaptureResult.LENS_STATE_STATIONARY
        val hasReusableFocusDistance = focusDistanceDiopters != null &&
            focusDistanceDiopters.isFinite() && focusDistanceDiopters >= 0f
        return continuousAf && passiveAttemptFinished && lensStationary &&
            hasReusableFocusDistance && supportsAfOff
    }
}
