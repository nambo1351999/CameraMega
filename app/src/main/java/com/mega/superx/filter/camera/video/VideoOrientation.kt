package com.mega.superx.filter.camera.video

internal fun resolveSurfaceTextureVideoOrientationDegrees(
    deviceRotationDegrees: Int,
    calibrationOffsetDegrees: Int,
): Int {
    return Math.floorMod(deviceRotationDegrees + calibrationOffsetDegrees, 360)
}
