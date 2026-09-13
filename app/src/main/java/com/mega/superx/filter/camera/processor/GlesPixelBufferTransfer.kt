package com.mega.superx.filter.camera.processor

internal object GlesPixelBufferTransfer {
    init {
        System.loadLibrary("my-native-lib")
    }

    
    external fun uploadRgba16fPboToTexture(
        pixelBufferObject: Int,
        textureId: Int,
        width: Int,
        height: Int,
    ): Boolean
}
