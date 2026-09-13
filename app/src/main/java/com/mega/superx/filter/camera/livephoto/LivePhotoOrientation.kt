package com.mega.superx.filter.camera.livephoto

internal fun resolveLivePhotoRotationDegrees(
    deviceRotationDegrees: Int,
    calibrationOffsetDegrees: Int,
): Int {
    return Math.floorMod(deviceRotationDegrees + calibrationOffsetDegrees, 360)
}
