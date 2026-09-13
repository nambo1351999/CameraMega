package com.mega.filter.camera.livephoto

internal fun resolveLivePhotoRotationDegrees(
    deviceRotationDegrees: Int,
    calibrationOffsetDegrees: Int,
): Int {
    return Math.floorMod(deviceRotationDegrees + calibrationOffsetDegrees, 360)
}
