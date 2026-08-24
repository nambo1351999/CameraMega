package com.zoomx.mega.cameramega.processor

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
