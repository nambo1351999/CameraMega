package com.mega.superx.filter.camera.camera

import android.graphics.Rect
import android.util.Range
import android.util.Size
import com.mega.superx.filter.camera.video.CaptureMode
import com.mega.superx.filter.camera.video.QuickShotCapabilities
import com.mega.superx.filter.camera.video.QuickShotConfig
import com.mega.superx.filter.camera.video.VideoCapabilities
import com.mega.superx.filter.camera.video.VideoConfig
import com.mega.superx.filter.camera.video.VideoRecordingState
import kotlin.math.abs

class AspectRatio private constructor(
    val name: String,
    val widthRatio: Int,
    val heightRatio: Int
) {
    override fun equals(other: Any?): Boolean {
        return other is AspectRatio && name == other.name
    }

    override fun hashCode(): Int {
        return name.hashCode()
    }

    override fun toString(): String {
        return name
    }

    fun getValue(isLandscape: Boolean): Float {
        return if (isLandscape) {
            widthRatio.toFloat() / heightRatio
        } else {
            heightRatio.toFloat() / widthRatio
        }
    }

    fun getDisplayName(): String {
        if (this == XPAN) {
            return "XPAN"
        }
        return "$widthRatio:$heightRatio"
    }

    companion object {
        const val TOP_SHEET_MAX_COUNT = 5
        private const val CUSTOM_PREFIX = "CUSTOM_"

        val RATIO_3_2 = AspectRatio("RATIO_3_2", 3, 2)
        val RATIO_4_3 = AspectRatio("RATIO_4_3", 4, 3)
        val RATIO_16_9 = AspectRatio("RATIO_16_9", 16, 9)
        val RATIO_1_1 = AspectRatio("RATIO_1_1", 1, 1)
        val RATIO_21_9 = AspectRatio("RATIO_21_9", 21, 9)
        val XPAN = AspectRatio("XPAN", 65, 24)

        val entries: List<AspectRatio> = listOf(
            RATIO_3_2,
            RATIO_4_3,
            RATIO_16_9,
            RATIO_1_1,
            RATIO_21_9,
            XPAN
        )

        val defaultTopSheetRatios: List<AspectRatio> = listOf(
            RATIO_3_2,
            RATIO_4_3,
            RATIO_16_9,
            RATIO_1_1,
            XPAN
        )

        fun fromString(string: String): AspectRatio {
            return valueOfOrNull(string) ?: entries.firstOrNull { it.getDisplayName() == string } ?: RATIO_4_3
        }

        fun valueOf(name: String): AspectRatio {
            return valueOfOrNull(name) ?: throw IllegalArgumentException("Unknown AspectRatio: $name")
        }

        fun valueOfOrNull(name: String): AspectRatio? {
            entries.firstOrNull { it.name == name }?.let { return it }
            if (!name.startsWith(CUSTOM_PREFIX)) return null
            val parts = name.removePrefix(CUSTOM_PREFIX).split("_")
            if (parts.size != 2) return null
            val width = parts[0].toIntOrNull() ?: return null
            val height = parts[1].toIntOrNull() ?: return null
            return custom(width, height)
        }

        fun custom(widthRatio: Int, heightRatio: Int): AspectRatio {
            val width = widthRatio.coerceIn(1, 999)
            val height = heightRatio.coerceIn(1, 999)
            val divisor = gcd(width, height)
            val normalizedWidth = width / divisor
            val normalizedHeight = height / divisor
            entries.firstOrNull {
                it.widthRatio == normalizedWidth && it.heightRatio == normalizedHeight
            }?.let { return it }
            return AspectRatio(
                name = "${CUSTOM_PREFIX}${normalizedWidth}_${normalizedHeight}",
                widthRatio = normalizedWidth,
                heightRatio = normalizedHeight
            )
        }

        fun sanitizeTopSheetRatios(ratios: List<AspectRatio>): List<AspectRatio> {
            val sanitized = ratios
                .distinct()
                .take(TOP_SHEET_MAX_COUNT)
            return sanitized.ifEmpty { defaultTopSheetRatios }
        }

        fun sanitizeCustomRatios(ratios: List<AspectRatio>): List<AspectRatio> {
            return ratios
                .map { custom(it.widthRatio, it.heightRatio) }
                .filter { ratio -> entries.none { it.name == ratio.name } }
                .distinctBy { it.name }
        }

        private tailrec fun gcd(a: Int, b: Int): Int {
            return if (b == 0) kotlin.math.abs(a) else gcd(b, a % b)
        }
    }
}

enum class LensType {
    FRONT,
    BACK_MAIN,
    BACK_ULTRA_WIDE,
    BACK_TELEPHOTO,
    BACK_MACRO
}

enum class MeteringMode {
    SYSTEM_DEFAULT,    
    CENTER_WEIGHTED,   
    SPOT,              
    AVERAGE,           
    HIGHLIGHT_PRIORITY; 
}

data class CameraInfo(
    val cameraId: String,
    val logicalCameraId: String? = null,
    val outputPhysicalCameraId: String? = null,
    val physicalCameras: List<CameraPhysicalInfo> = emptyList(),
    val lensFacing: Int,
    val lensType: LensType,
    val physicalCameraIds: List<String>,
    val isoRange: Range<Int>?,
    val exposureTimeRange: Range<Long>?,
    val exposureCompensationRange: Range<Int>,
    val exposureCompensationStep: Float,
    val maxZoom: Float,
    val minZoom: Float = 1f,  
    val sensorOrientation: Int,
    val activeArraySize: Rect?,
    val focalLength: Float = 0f,  
    val focalLength35mmEquivalent: Float = 0f,  
    val zoomSteps: List<Float> = listOf(1f),  
    val intrinsicZoomRatio: Float = 1f,  
    val displayIntrinsicZoomRatio: Float = intrinsicZoomRatio, 
    val hardwareLevel: Int = -1,  
    val supportsManualProcessing: Boolean = false, 
    val supportsRaw: Boolean = false, 
    val isCustomLensId: Boolean = false, 
    val isVirtualIszLens: Boolean = false, 
    val isVirtualIszMacroLens: Boolean = false, 
    val baseCameraId: String? = null, 
    val iszZoomRatio: Float = 1f, 
    val rawBlackBorderCrop: RawBlackBorderCrop = RawBlackBorderCrop(), 
    val minimumFocusDistance: Float = 0f 
) {
    fun getOpenCameraId(): String = logicalCameraId ?: cameraId

    fun getBoundPhysicalCameraId(): String? = outputPhysicalCameraId

    fun getBoundPhysicalCameraId(zoomRatioByMain: Float): String? {
        if (physicalCameras.isEmpty()) return outputPhysicalCameraId
        return physicalCameras.minByOrNull {
            abs(it.intrinsicZoomRatio - zoomRatioByMain)
        }?.cameraId ?: outputPhysicalCameraId
    }

    
    fun getLensDisplayName(): String {
        val name = when (lensType) {
            LensType.FRONT -> "前置"
            LensType.BACK_MAIN -> "主摄 (1x)"
            LensType.BACK_ULTRA_WIDE -> "广角 (0.5x)"
            LensType.BACK_TELEPHOTO -> "长焦 (${String.format("%.1f", focalLength35mmEquivalent / 24f)}x)"
            LensType.BACK_MACRO -> "微距"
        }
        return if (isCustomLensId) "$name *" else name
    }

    
    fun hasWideAngle(): Boolean = minZoom < 0.9f

    
    fun hasTelephoto(): Boolean = maxZoom > 2f

    
    fun getHardwareLevelName(): String {
        return when (hardwareLevel) {
            0 -> "LIMITED"
            1 -> "FULL"
            2 -> "LEGACY"
            3 -> "LEVEL_3"
            4 -> "EXTERNAL"
            else -> "UNKNOWN"
        }
    }
}

data class CameraPhysicalInfo(
    val cameraId: String,
    val intrinsicZoomRatio: Float,
    val focalLength35mmEquivalent: Float = 0f,
    val focalLength: Float = 0f
)

enum class FocusPointSource {
    MANUAL,
    AI
}

data class WhiteBalanceGains(
    val red: Float,
    val greenEven: Float,
    val greenOdd: Float,
    val blue: Float
)

data class CameraState(
    
    val currentCameraId: String = "",
    val currentLensType: LensType = LensType.BACK_MAIN,
    val availableCameras: List<CameraInfo> = emptyList(),
    val currentPreviewSize: Size = Size(1440, 1080),
    val currentCaptureSize: Size = Size(1440, 1080),

    
    val exposureCompensation: Int = 0,
    val isIsoAuto: Boolean = true,
    val isShutterSpeedAuto: Boolean = true,
    val iso: Int = 100,
    val shutterSpeed: Long = 1_000_000_000L / 60, 
    val exposureBias: Float = 0f,

    val awbMode: Int = 1, 
    val awbTemperature: Int = 5000, 
    val actualAwbTemperature: Int? = null, 
    val actualAwbTint: Int? = null,
    val actualAwbGains: WhiteBalanceGains? = null,
    val canAdjustWhiteBalance: Boolean = false,
    val supportsCctWhiteBalance: Boolean = false,
    val awbTemperatureMin: Int = 2000,
    val awbTemperatureMax: Int = 8000,

    
    val meteringMode: MeteringMode = MeteringMode.SYSTEM_DEFAULT,

    val isVirtualApertureEnabled: Boolean = false,
    val physicalAperture: Float = 2.0f, 
    val virtualAperture: Float = 2.0f,  
    
    val isAutoFocus: Boolean = true,
    val focusDistance: Float = 0f, 
    val isHyperfocalFocusEnabled: Boolean = false,
    val hyperfocalDistanceMeters: Float = 0f,
    val minimumFocusDistance: Float = 0f, 
    val focusPoint: Pair<Float, Float>? = null, 
    val focusPointSource: FocusPointSource = FocusPointSource.MANUAL,
    val isFocusLocked: Boolean = false,
    val isFocusing: Boolean = false,
    val focusSuccess: Boolean? = null,
    val currentAfMode: Int? = null, 

    
    val flashMode: Int = 0, 

    
    val zoomRatio: Float = 1.0f,

    
    val aspectRatio: AspectRatio = AspectRatio.RATIO_4_3,

    
    val deviceRotation: Int = 0, 

    
    val isPreviewActive: Boolean = false,

    
    val isCapturing: Boolean = false,

    
    val currentLutName: String? = null,
    val lutEnabled: Boolean = false,
    val isLogLutActive: Boolean = false,
    val availableLuts: List<String> = emptyList(),

    
    val histogram: IntArray? = null,

    
    val timerSeconds: Int = 0, 
    val countdownValue: Int = 0, 

    
    val showGrid: Boolean = false, 

    
    val nrLevel: Int = 5,
    val availableNrModes: IntArray = intArrayOf(),
    val vendorCaptureSettingsByLens: VendorCaptureSettingsByLens = VendorCaptureSettingsByLens.Empty,
    val customVendorKeySettings: CustomVendorKeySettings = CustomVendorKeySettings.Empty,

    val isRawSupported: Boolean = false,
    val multiFrameOutputScale: Float? = null,
    val multiFrameCount: Int = MultiFrameConfig.DEFAULT_FRAME_COUNT,
    val useJpgMaxHdrComposition: Boolean = false,
    val useRawMaxHdrComposition: Boolean = MultiFrameConfig.DEFAULT_RAW_MAX_HDR_COMPOSITION,
    val useRaw: Boolean = false,
    val useMultipleExposure: Boolean = false,
    val rawMinShutterSpeedNs: Long = 0L,
    val useLivePhoto: Boolean = false,
    val droMode: String = "OFF",
    val tonemapMode: String = "SYSTEM_DEFAULT",
    val fixTonemapPreview: Boolean = false,
    val fixTonemapCapture: Boolean = false,
    
    val isCapturingLivePhoto: Boolean = false,
    val applyUltraHDR: Boolean = true,
    val useP010: Boolean = false,
    val useHlg10: Boolean = false,
    val useP3ColorSpace: Boolean = false,
    val isP010Supported: Boolean = false,
    val isHlg10Supported: Boolean = false,
    val burstCapturing: Boolean = false,
    val hdrBracketCapturing: Boolean = false,
    val hdrBracketFrameCount: Int = 0,
    val latitude: Double? = null,
    val longitude: Double? = null,
    val isP3Supported: Boolean = false,
    val currentDynamicRangeProfile: String = "STANDARD",
    val captureMode: CaptureMode = CaptureMode.PHOTO,
    val quickShotConfig: QuickShotConfig = QuickShotConfig(),
    val quickShotCapabilities: QuickShotCapabilities = QuickShotCapabilities(),
    val videoConfig: VideoConfig = VideoConfig(),
    val videoCapabilities: VideoCapabilities = VideoCapabilities(),
    val videoRecordingState: VideoRecordingState = VideoRecordingState(),
) {
    companion object {
        const val MAX_PHOTO_PREVIEW_SHUTTER_SPEED_NS = 1_000_000_000L / 15
        const val MAX_VIDEO_MANUAL_SHUTTER_SPEED_NS = 1_000_000_000L / 6
    }

    
    val isAutoExposure: Boolean
        get() = isIsoAuto && isShutterSpeedAuto

    val isHLG: Boolean
        get() = currentDynamicRangeProfile == "HLG10"

    val isMultiFrameEnabled: Boolean
        get() = multiFrameOutputScale != null && (!useRaw || isRawSupported)

    val isJpgMaxEnabled: Boolean
        get() = isMultiFrameEnabled && !useRaw

    val isJpgMaxHdrEnabled: Boolean
        get() = isJpgMaxEnabled && useJpgMaxHdrComposition

    val isRawMaxEnabled: Boolean
        get() = isMultiFrameEnabled && useRaw

    val isRawMaxHdrEnabled: Boolean
        get() = isRawMaxEnabled && useRawMaxHdrComposition

    
    fun getCurrentCameraInfo(): CameraInfo? {
        return availableCameras.find { it.cameraId == currentCameraId }
    }

    
    fun getExposureCompensationRange(): Range<Int> {
        return getCurrentCameraInfo()?.exposureCompensationRange ?: Range(0, 0)
    }

    fun getExposureCompensationStep(): Float {
        return getCurrentCameraInfo()?.exposureCompensationStep ?: 0.333f
    }

    
    fun getIsoRange(): Range<Int> {
        return getCurrentCameraInfo()?.isoRange ?: Range(100, 3200)
    }

    
    fun getShutterSpeedRange(): Range<Long> {
        return getCurrentCameraInfo()?.exposureTimeRange ?: Range(1_000_000L, 1_000_000_000L)
    }

    fun getManualShutterSpeedRange(): Range<Long> {
        val sensorRange = getShutterSpeedRange()
        val upper = when (captureMode) {
            CaptureMode.VIDEO -> sensorRange.upper.coerceAtMost(MAX_VIDEO_MANUAL_SHUTTER_SPEED_NS)
            CaptureMode.PHOTO,
            CaptureMode.QUICK_SHOT -> sensorRange.upper
        }.coerceAtLeast(sensorRange.lower)
        return Range(sensorRange.lower, upper)
    }

    fun getPreviewExposureTimeLimitNs(): Long {
        return when (captureMode) {
            CaptureMode.VIDEO -> MAX_VIDEO_MANUAL_SHUTTER_SPEED_NS
            CaptureMode.PHOTO,
            CaptureMode.QUICK_SHOT -> MAX_PHOTO_PREVIEW_SHUTTER_SPEED_NS
        }
    }

    fun isPreviewExposureLimited(): Boolean {
        return shutterSpeed >= getPreviewExposureTimeLimitNs()
    }

    
    fun getMaxZoom(): Float {
        return getCurrentCameraInfo()?.maxZoom ?: 1.0f
    }

    
    fun getMinZoom(): Float {
        return getCurrentCameraInfo()?.minZoom ?: 1.0f
    }

    
    fun getZoomSteps(): List<Float> {
        return getCurrentCameraInfo()?.zoomSteps ?: listOf(1f)
    }

    fun getAvgLuma(): Float {
        val histogram = histogram ?: return 0.18f
        var totalCount = 0L
        var weightedSum = 0L
        for (i in histogram.indices) {
            val count = histogram[i].toLong()
            weightedSum += i * count
            totalCount += count
        }
        if (totalCount == 0L) return 0.18f
        return (weightedSum.toFloat() / totalCount) / 255f
    }

    fun getPreviewAspectRatio(): Float {
        return when (captureMode) {
            CaptureMode.PHOTO, CaptureMode.QUICK_SHOT -> aspectRatio.getValue(isLandscape = false)
            CaptureMode.VIDEO -> videoConfig.aspectRatio.getPortraitAspectRatio(
                videoCapabilities.openGatePortraitAspectRatio
            )
        }
    }
}
