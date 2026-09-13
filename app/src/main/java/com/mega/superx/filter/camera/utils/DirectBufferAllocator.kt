package com.mega.superx.filter.camera.utils

import java.nio.ByteBuffer

object DirectBufferAllocator {
    init {
        System.loadLibrary("my-native-lib")
    }

    
    external fun allocateNative(capacity: Long): ByteBuffer?

    
    external fun freeNative(buffer: ByteBuffer)
}
