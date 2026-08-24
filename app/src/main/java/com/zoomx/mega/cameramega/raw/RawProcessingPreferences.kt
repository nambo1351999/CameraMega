package com.zoomx.mega.cameramega.raw

object RawProcessingPreferences {
    enum class DROMode(val captureExposureReductionEv: Float) {
        OFF(0f),
        DR100(0f),
        DR200(1f),
        DR400(2f);

        val isEnabled: Boolean
            get() = this != OFF

        companion object {
            fun fromPersistedName(name: String?): DROMode {
                return when (name) {
                    DR100.name -> DR100
                    DR200.name -> DR200
                    DR400.name -> DR400
                    OFF.name -> OFF
                    else -> OFF
                }
            }
        }
    }
}
