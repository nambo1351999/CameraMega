package com.zoomx.mega.cameramega.utils

import java.nio.ByteBuffer

object DirectBufferPixelPacker {
    init {
        System.loadLibrary("my-native-lib")
    }

    external fun unpackRgba16TileToRgb16(
        source: ByteBuffer,
        sourceWidth: Int,
        sourceHeight: Int,
        destination: ByteBuffer,
        destinationWidth: Int,
        destinationHeight: Int,
        destinationLeft: Int,
        destinationTop: Int,
    ): Boolean

    
    external fun packRgba16fToRgb16(
        source: ByteBuffer,
        width: Int,
        height: Int,
        destination: ByteBuffer,
    ): Boolean
}
