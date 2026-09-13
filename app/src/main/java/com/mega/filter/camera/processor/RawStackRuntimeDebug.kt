package com.mega.filter.camera.processor

import com.mega.filter.camera.BuildConfig
import com.mega.filter.camera.utils.PLog
import com.mega.filter.camera.utils.SystemPropertiesUtil

internal enum class MgcSpatialDiagnosticMode {
    NONE,
    REFERENCE_ONLY,
    IDENTITY_TEMPORAL_WEIGHTS,
    MAIN_REJECTION_ONLY,
    DISABLE_UNBLOCKER,
    DISABLE_LINEAR_KERNEL,
    FORCE_LINEAR_KERNEL,
}

internal object RawStackRuntimeDebug {
    val enabled: Boolean
        get() = BuildConfig.DEBUG

    
    val mgcSpatialDiagnosticMode: MgcSpatialDiagnosticMode
        get() {
            if (!enabled) return MgcSpatialDiagnosticMode.NONE
            return when (
                SystemPropertiesUtil.get("debug.photon.mgc_spatial.mode")?.uppercase()
            ) {
                "REFERENCE_ONLY" -> MgcSpatialDiagnosticMode.REFERENCE_ONLY
                "IDENTITY_TEMPORAL_WEIGHTS" ->
                    MgcSpatialDiagnosticMode.IDENTITY_TEMPORAL_WEIGHTS
                "MAIN_REJECTION_ONLY" ->
                    MgcSpatialDiagnosticMode.MAIN_REJECTION_ONLY
                "DISABLE_UNBLOCKER" ->
                    MgcSpatialDiagnosticMode.DISABLE_UNBLOCKER
                "DISABLE_LINEAR_KERNEL" ->
                    MgcSpatialDiagnosticMode.DISABLE_LINEAR_KERNEL
                "FORCE_LINEAR_KERNEL" ->
                    MgcSpatialDiagnosticMode.FORCE_LINEAR_KERNEL
                else -> MgcSpatialDiagnosticMode.NONE
            }
        }

    
    val mgcSpatialInputDiagnosticsEnabled: Boolean
        get() = enabled &&
            SystemPropertiesUtil.get("debug.photon.mgc_spatial.input_diagnostics")
                ?.toBooleanStrictOrNull() == true

    
    val mgcFullResolutionDenoiseDiagnosticsEnabled: Boolean
        get() = enabled &&
            SystemPropertiesUtil.get("debug.photon.mgc_denoise.diagnostics")
                ?.toBooleanStrictOrNull() == true

    inline fun d(tag: String, message: () -> String) {
        if (enabled) {
            PLog.d(tag, message())
        }
    }

    inline fun i(tag: String, message: () -> String) {
        if (enabled) {
            PLog.i(tag, message())
        }
    }
}
