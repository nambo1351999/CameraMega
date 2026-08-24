package com.zoomx.mega.cameramega.raw

object DcpNativeBridge {
    init {
        System.loadLibrary("my-native-lib")
    }

    external fun parseDcpToJson(filePath: String): String
}
