package com.zoomx.mega.cameramega.video

internal fun resolveSurfaceTextureVideoOrientationDegrees(
    deviceRotationDegrees: Int,
    calibrationOffsetDegrees: Int,
): Int {
    return Math.floorMod(deviceRotationDegrees + calibrationOffsetDegrees, 360)
}
