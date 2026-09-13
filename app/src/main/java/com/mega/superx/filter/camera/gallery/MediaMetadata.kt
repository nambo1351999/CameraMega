package com.mega.superx.filter.camera.gallery

import android.content.Context
import android.graphics.BitmapFactory
import android.graphics.ColorSpace
import android.graphics.Rect
import android.net.Uri
import android.os.Build
import androidx.exifinterface.media.ExifInterface
import com.mega.superx.filter.camera.camera.AspectRatio
import com.mega.superx.filter.camera.camera.RawBlackBorderCrop
import com.mega.superx.filter.camera.lut.BaselineColorCorrectionTarget
import com.mega.superx.filter.camera.model.ColorRecipeParams
import com.mega.superx.filter.camera.hdr.HdrGainmapStrength
import com.mega.superx.filter.camera.utils.PLog
import com.mega.superx.filter.camera.raw.RawMetadata
import com.mega.superx.filter.camera.raw.HncsFilmCurveMode
import com.mega.superx.filter.camera.raw.HncsRenderIntent
import com.mega.superx.filter.camera.raw.RawRenderingEngine
import com.mega.superx.filter.camera.raw.RawToneMappingParameters
import com.mega.superx.filter.camera.utils.DeviceUtil
import org.json.JSONObject
import java.text.SimpleDateFormat
import java.util.*
import kotlin.math.log2

const val TONEMAP_MODE_NATURAL_LIGHT = "NATURAL_LIGHT"

private fun JSONObject.optCompatibleRawAutoExposure(): Boolean? {
    if (!isNull("rawAutoExposureMode")) {
        when (optString("rawAutoExposureMode")) {
            "OFF" -> return false
            "VIEWFINDER_MATCH", "DYNAMIC_SCENE_ESTIMATION" -> return true
        }
    }
    return if (isNull("rawAutoExposure")) null else optBoolean("rawAutoExposure")
}

data class MediaMetadata(
    val version: Int = 30,
    val mediaType: MediaType = MediaType.IMAGE,
    
    val lutId: String? = null,
    val tonemapMode: String? = null,
    
    val colorRecipeParams: ColorRecipeParams? = null,
    val baselineTarget: BaselineColorCorrectionTarget? = null,
    val baselineLutId: String? = null,
    val baselineColorRecipeParams: ColorRecipeParams? = null,
    
    val sharpening: Float? = null,
    val noiseReduction: Float? = null,
    val chromaNoiseReduction: Float? = null,
    val captureNoiseReductionLevel: Int? = null,
    
    val rawDenoiseValue: Float? = null,
    
    val rawChromaDenoiseValue: Float? = null,
    val rawExposureCompensation: Float? = null,
    val rawAutoExposure: Boolean? = null,
    val rawHighlightsAdjustment: Float? = null,
    val rawShadowsAdjustment: Float? = null,
    val rawBlackPointCorrection: Float? = null,
    val rawWhitePointCorrection: Float? = null,
    val rawAutoWhiteBalanceEstimate: Boolean? = null,
    val rawLensShadingCorrectionEnabled: Boolean? = null,
    val rawDcpId: String? = null,
    val rawHncsProfileId: String? = null,
    val rawHncsRenderIntent: HncsRenderIntent = HncsRenderIntent.Standard,
    val rawHncsFilmCurveMode: HncsFilmCurveMode = HncsFilmCurveMode.Standard,
    val rawRenderingEngine: RawRenderingEngine = RawRenderingEngine.AdobeCurve,
    val rawToneMappingParameters: RawToneMappingParameters = RawToneMappingParameters.DEFAULT,
    val cameraId: String? = null,
    
    val frameId: String? = null,
    
    val width: Int = 0,
    val height: Int = 0,
    val ratio: AspectRatio? = null,
    val cropRegion: Rect? = null,
    val rotation: Int = 0,
    
    val deviceModel: String? = null, 
    val brand: String? = null,
    val dateTaken: Long? = null,
    val location: String? = null,
    
    val latitude: Double? = null,
    val longitude: Double? = null,
    val altitude: Double? = null,
    val iso: Int? = null,
    val shutterSpeed: String? = null,
    val focalLength: String? = null,
    val focalLength35mm: String? = null,
    val aperture: String? = null,
    val exposureBias: Float? = null,
    val isImported: Boolean = false,
    val sourceUri: String? = null, 
    val mimeType: String? = null,
    val durationMs: Long? = null,
    val frameRate: Int? = null,
    val bitrate: Long? = null,
    val rotationDegrees: Int? = null,
    val hasAudio: Boolean? = null,
    val videoWidth: Int? = null,
    val videoHeight: Int? = null,
    
    val customProperties: Map<String, String> = emptyMap(),
    
    val exportedUris: List<String> = emptyList(),
    
    val computationalAperture: Float? = null,
    val focusPointX: Float? = null,
    val focusPointY: Float? = null,
    val postCropRegion: Rect? = null,
    val postRotationDegrees: Int = 0,
    val postStraightenDegrees: Float = 0f,
    val postMirrorHorizontal: Boolean = false,
    
    val presentationTimestampUs: Long? = null,
    
    val droMode: String? = null,
    val software: String? = null,
    val isMirrored: Boolean = false,
    val colorSpace: ColorSpace.Named = ColorSpace.Named.SRGB,
    val manualHdrEffectEnabled: Boolean = false,
    val hdrEffectStrength: Float = HdrGainmapStrength.DEFAULT,
    val hasEmbeddedGainmap: Boolean = false,
    val dynamicRangeProfile: String? = null,
    val captureMode: String? = null,
    val multipleExposureFrameCount: Int? = null,
    val hasAiDenoisedBase: Boolean = false,
    val aiDenoiseStrength: Float? = null,
    val rawBlackLevelMode: String? = null,
    val rawCustomBlackLevel: Float? = null,
    val rawWhiteLevelMode: String? = null,
    val rawCustomWhiteLevel: Float? = null,
    val rawCfaCorrectionMode: String? = null,
    val rawBlackBorderCrop: RawBlackBorderCrop = RawBlackBorderCrop(),
    val applyEffectsToVideo: Boolean = false,
    val spectralFilmStock: String? = null,
    val spectralFilmPrint: String? = null,
    val spectralFilmCDensityGain: Float = 1f,
    val spectralFilmMDensityGain: Float = 1f,
    val spectralFilmYDensityGain: Float = 1f,
) {
    
    fun toCaptureInfo(): com.mega.superx.filter.camera.camera.CaptureInfo {
        return com.mega.superx.filter.camera.camera.CaptureInfo(
            iso = iso,
            make = brand ?: Build.MANUFACTURER,
            model = if (isImported) {
                deviceModel?.takeIf { it.isNotBlank() } ?: DeviceUtil.exifModel
            } else {
                DeviceUtil.exifModel
            },
            captureTime = dateTaken ?: System.currentTimeMillis(),
            imageWidth = width,
            imageHeight = height,
            aperture = aperture?.substringAfter("/")?.toFloatOrNull(),
            focalLength = focalLength?.substringBefore("mm")?.toFloatOrNull(),
            focalLength35mm = focalLength35mm?.substringBefore("mm")?.toIntOrNull(),
            exposureBias = exposureBias,
            exposureTime = parseExposureTime(shutterSpeed),
            software = software ?: "CameraMega",
            latitude = latitude,
            longitude = longitude,
            altitude = altitude,
        )
    }

    private fun parseExposureTime(s: String?): Long? {
        if (s == null) return null
        return try {
            if (s.contains("/")) {
                val clean = s.substringBefore("s").substringBefore("\"")
                val parts = clean.split("/")
                val numerator = parts[0].toDouble()
                val denominator = parts[1].toDouble()
                (numerator / denominator * 1_000_000_000).toLong()
            } else {
                val clean = s.substringBefore("s").substringBefore("\"")
                (clean.toDouble() * 1_000_000_000).toLong()
            }
        } catch (e: Exception) {
            null
        }
    }

    val lv: Float
        get() {
            val aperture = aperture?.substringAfter("/")?.toFloatOrNull() ?: return 0f
            val shutterSpeed = parseExposureTime(shutterSpeed)?.let { it * 1f / 1_000_000_000L } ?: return 0f
            val iso = iso ?: return 0f
            val ev = log2((aperture * aperture) / shutterSpeed)
            return ev - log2(iso / 100f)
        }

    
    val resolution: String
        get() = "${width}x${height}"

    fun usesNaturalLightToneMap(): Boolean {
        return tonemapMode == TONEMAP_MODE_NATURAL_LIGHT
    }

    fun shouldApplyNaturalLightDefaultChromaDenoise(): Boolean {
        return usesNaturalLightToneMap() && captureNoiseReductionLevel in LOW_HARDWARE_NOISE_REDUCTION_LEVELS
    }

    
    fun merge(raw: RawMetadata): MediaMetadata {
        val resolvedRotation = raw.rotation ?: rotation
        val shouldSwap = resolvedRotation == 90 || resolvedRotation == 270
        val rawWidth = raw.defaultCrop?.width() ?: raw.width
        val rawHeight = raw.defaultCrop?.height() ?: raw.height
        val resolvedWidth = if (shouldSwap) rawHeight else rawWidth
        val resolvedHeight = if (shouldSwap) rawWidth else rawHeight
        return copy(
            iso = mergeIso(raw.iso),
            shutterSpeed = formatShutterSpeed(raw.shutterSpeed).takeIf { it.isNotEmpty() } ?: shutterSpeed,
            aperture = formatAperture(raw.aperture).takeIf { it.isNotEmpty() } ?: aperture,
            exposureBias = raw.exposureBias,
            width = resolvedWidth.takeIf { it > 0 } ?: width,
            height = resolvedHeight.takeIf { it > 0 } ?: height,
            rotation = resolvedRotation
        )
    }

    private fun mergeIso(rawIso: Int): Int? {
        if (rawIso <= 0) return iso
        val currentIso = iso?.takeIf { it > 0 }
        if (currentIso != null && rawIso == RAW_ISO_FALLBACK_VALUE && currentIso != RAW_ISO_FALLBACK_VALUE) {
            return currentIso
        }
        return rawIso
    }

    private fun formatShutterSpeed(shutterSpeedNs: Long): String {
        if (shutterSpeedNs <= 0) return ""
        val seconds = shutterSpeedNs / 1_000_000_000.0
        return if (seconds >= 1.0) {
            "${seconds.toInt()}\""
        } else {
            "1/${(1.0 / seconds).toInt()}"
        }
    }

    private fun formatAperture(aperture: Float): String {
        if (aperture <= 0) return ""
        return "f/${String.format("%.1f", aperture)}"
    }

    companion object {
        private const val TAG = "PhotoMetadata"
        private const val RAW_ISO_FALLBACK_VALUE = 100
        private val LOW_HARDWARE_NOISE_REDUCTION_LEVELS = setOf(0, 4)

        
        fun fromLegacyJson(json: String): MediaMetadata? {
            return try {
                val obj = JSONObject(json)

                
                val exportedUris = mutableListOf<String>()
                val urisArray = obj.optJSONArray("exportedUris")
                if (urisArray != null) {
                    for (i in 0 until urisArray.length()) {
                        exportedUris.add(urisArray.getString(i))
                    }
                }

                
                val colorRecipeParamsObj = obj.optJSONObject("colorRecipeParams")
                val colorRecipeParams = if (colorRecipeParamsObj != null && !obj.isNull("colorRecipeParams")) {
                    
                    if (colorRecipeParamsObj.has("vibrance") && !colorRecipeParamsObj.has("color")) {
                        colorRecipeParamsObj.put("color", colorRecipeParamsObj.optDouble("vibrance", 0.0))
                    }
                    ColorRecipeParams.fromJson(colorRecipeParamsObj.toString())
                } else {
                    null
                }
                val baselineColorRecipeParamsObj = obj.optJSONObject("baselineColorRecipeParams")
                val baselineColorRecipeParams =
                    if (baselineColorRecipeParamsObj != null && !obj.isNull("baselineColorRecipeParams")) {
                        ColorRecipeParams.fromJson(baselineColorRecipeParamsObj.toString())
                    } else {
                        null
                    }

                MediaMetadata(
                    version = obj.optInt("version", 1),
                    mediaType = obj.optString("mediaType", MediaType.IMAGE.name).let {
                        MediaType.entries.firstOrNull { type ->
                            type.name.equals(it, ignoreCase = true)
                        } ?: MediaType.IMAGE
                    },
                    lutId = if (obj.isNull("lutId")) null else obj.optString("lutId"),
                    tonemapMode = if (obj.isNull("tonemapMode")) null else obj.optString("tonemapMode"),
                    colorRecipeParams = colorRecipeParams,
                    baselineTarget = if (obj.isNull("baselineTarget")) null else runCatching {
                        BaselineColorCorrectionTarget.valueOf(obj.optString("baselineTarget"))
                    }.getOrNull(),
                    baselineLutId = if (obj.isNull("baselineLutId")) null else obj.optString("baselineLutId"),
                    baselineColorRecipeParams = baselineColorRecipeParams,
                    sharpening = if (obj.isNull("sharpening")) null else obj.optDouble("sharpening").toFloat(),
                    noiseReduction = if (obj.isNull("noiseReduction")) null else obj.optDouble("noiseReduction")
                        .toFloat(),
                    chromaNoiseReduction = if (obj.isNull("chromaNoiseReduction")) null else obj.optDouble("chromaNoiseReduction")
                        .toFloat(),
                    captureNoiseReductionLevel = if (obj.isNull("captureNoiseReductionLevel")) null else obj.optInt("captureNoiseReductionLevel"),
                    rawDenoiseValue = if (obj.isNull("denoiseValue")) null else obj.optDouble("denoiseValue").toFloat(),
                    rawChromaDenoiseValue = if (obj.isNull("rawChromaDenoiseValue")) {
                        null
                    } else {
                        obj.optDouble("rawChromaDenoiseValue").toFloat()
                    },
                    rawExposureCompensation = if (obj.isNull("rawExposureCompensation")) null else obj.optDouble("rawExposureCompensation").toFloat(),
                    rawAutoExposure = obj.optCompatibleRawAutoExposure(),
                    rawHighlightsAdjustment = if (obj.isNull("rawHighlightsAdjustment")) null else obj.optDouble("rawHighlightsAdjustment").toFloat(),
                    rawShadowsAdjustment = if (obj.isNull("rawShadowsAdjustment")) null else obj.optDouble("rawShadowsAdjustment").toFloat(),
                    rawBlackPointCorrection = if (obj.isNull("rawBlackPointCorrection")) null else obj.optDouble("rawBlackPointCorrection").toFloat(),
                    rawWhitePointCorrection = if (obj.isNull("rawWhitePointCorrection")) null else obj.optDouble("rawWhitePointCorrection").toFloat(),
                    rawAutoWhiteBalanceEstimate = if (obj.isNull("rawAutoWhiteBalanceEstimate")) null else obj.optBoolean("rawAutoWhiteBalanceEstimate"),
                    rawLensShadingCorrectionEnabled = if (obj.isNull("rawLensShadingCorrectionEnabled")) null else obj.optBoolean("rawLensShadingCorrectionEnabled"),
                    rawDcpId = if (obj.isNull("rawDcpId")) null else obj.optString("rawDcpId"),
                    rawHncsProfileId = if (obj.isNull("rawHncsProfileId")) {
                        null
                    } else {
                        obj.optString("rawHncsProfileId")
                    },
                    rawHncsRenderIntent = HncsRenderIntent.fromPersistedValue(
                        if (obj.isNull("rawHncsRenderIntent")) {
                            null
                        } else {
                            obj.optString("rawHncsRenderIntent")
                        }
                    ),
                    rawHncsFilmCurveMode = HncsFilmCurveMode.fromPersistedValue(
                        if (obj.isNull("rawHncsFilmCurveMode")) {
                            null
                        } else {
                            obj.optString("rawHncsFilmCurveMode")
                        }
                    ),
                    rawRenderingEngine = RawRenderingEngine.fromPersistedName(
                        if (obj.isNull("rawColorEngine")) null else obj.optString("rawColorEngine"),
                        fallback = RawRenderingEngine.AdobeCurve
                    ),
                    rawToneMappingParameters = RawToneMappingParameters(
                        agxBlackRelativeExposure = if (obj.isNull("rawAgxBlackRelativeExposure")) {
                            RawToneMappingParameters.AGX_BLACK_RELATIVE_EXPOSURE_DEFAULT
                        } else {
                            obj.optDouble("rawAgxBlackRelativeExposure").toFloat()
                        },
                        agxWhiteRelativeExposure = if (obj.isNull("rawAgxWhiteRelativeExposure")) {
                            RawToneMappingParameters.AGX_WHITE_RELATIVE_EXPOSURE_DEFAULT
                        } else {
                            obj.optDouble("rawAgxWhiteRelativeExposure").toFloat()
                        },
                        agxToe = if (obj.isNull("rawAgxToe")) {
                            RawToneMappingParameters.AGX_TOE_DEFAULT
                        } else {
                            obj.optDouble("rawAgxToe").toFloat()
                        },
                        agxShoulder = if (obj.isNull("rawAgxShoulder")) {
                            RawToneMappingParameters.AGX_SHOULDER_DEFAULT
                        } else {
                            obj.optDouble("rawAgxShoulder").toFloat()
                        },
                        filmicBlackRelativeExposure = if (obj.isNull("rawFilmicBlackRelativeExposure")) {
                            RawToneMappingParameters.FILMIC_BLACK_RELATIVE_EXPOSURE_DEFAULT
                        } else {
                            obj.optDouble("rawFilmicBlackRelativeExposure").toFloat()
                        },
                        filmicWhiteRelativeExposure = if (obj.isNull("rawFilmicWhiteRelativeExposure")) {
                            RawToneMappingParameters.FILMIC_WHITE_RELATIVE_EXPOSURE_DEFAULT
                        } else {
                            obj.optDouble("rawFilmicWhiteRelativeExposure").toFloat()
                        },
                        useOppoMasterToneMap = obj.optBoolean("rawOppoMasterToneMap", false),
                        usePhotonHdr =
                            obj.optBoolean("rawPhotonHdr", false) ||
                                obj.optBoolean("rawPhotonPgtmToneMap", false) ||
                                obj.optBoolean("rawGooglePixelToneMap", false)
                    ).normalized(),
                    rawBlackLevelMode = if (obj.isNull("rawBlackLevelMode")) null else obj.optString("rawBlackLevelMode"),
                    rawCustomBlackLevel = if (obj.isNull("rawCustomBlackLevel")) null else obj.optDouble("rawCustomBlackLevel").toFloat(),
                    rawWhiteLevelMode = if (obj.isNull("rawWhiteLevelMode")) null else obj.optString("rawWhiteLevelMode"),
                    rawCustomWhiteLevel = if (obj.isNull("rawCustomWhiteLevel")) null else obj.optDouble("rawCustomWhiteLevel").toFloat(),
                    rawCfaCorrectionMode = if (obj.isNull("rawCfaCorrectionMode")) null else obj.optString("rawCfaCorrectionMode"),
                    cameraId = if (obj.isNull("cameraId")) null else obj.optString("cameraId"),
                    frameId = if (obj.isNull("frameId")) null else obj.optString("frameId"),
                    width = obj.optInt("width", 0),
                    height = obj.optInt("height", 0),
                    ratio = if (obj.isNull("ratio")) null else AspectRatio.fromString(obj.optString("ratio")),
                    cropRegion = if (obj.isNull("cropRegion")) null else {
                        val cropObj = obj.getJSONObject("cropRegion")
                        Rect(
                            cropObj.optInt("left", 0),
                            cropObj.optInt("top", 0),
                            cropObj.optInt("right", 0),
                            cropObj.optInt("bottom", 0)
                        )
                    },
                    postCropRegion = if (obj.isNull("postCropRegion")) null else {
                        val pcObj = obj.getJSONObject("postCropRegion")
                        Rect(
                            pcObj.getInt("left"),
                            pcObj.getInt("top"),
                            pcObj.getInt("right"),
                            pcObj.getInt("bottom")
                        )
                    },
                    postRotationDegrees = obj.optInt("postRotationDegrees", 0),
                    postStraightenDegrees = if (obj.isNull("postStraightenDegrees")) {
                        0f
                    } else {
                        obj.optDouble("postStraightenDegrees").toFloat()
                    },
                    postMirrorHorizontal = obj.optBoolean("postMirrorHorizontal", false),
                    rotation = obj.optInt("rotation", 0),
                    
                    deviceModel = if (obj.isNull("deviceModel")) null else obj.optString("deviceModel"),
                    brand = if (obj.isNull("brand")) null else obj.optString("brand"),
                    dateTaken = if (obj.isNull("dateTaken")) null else obj.optLong("dateTaken"),
                    location = if (obj.isNull("location")) null else obj.optString("location"),
                    latitude = if (obj.isNull("latitude")) null else obj.optDouble("latitude"),
                    longitude = if (obj.isNull("longitude")) null else obj.optDouble("longitude"),
                    altitude = if (obj.isNull("altitude")) null else obj.optDouble("altitude"),
                    iso = if (obj.isNull("iso")) null else obj.optInt("iso"),
                    shutterSpeed = if (obj.isNull("shutterSpeed")) null else obj.optString("shutterSpeed"),
                    focalLength = if (obj.isNull("focalLength")) null else obj.optString("focalLength"),
                    focalLength35mm = if (obj.isNull("focalLength35mm")) null else obj.optString("focalLength35mm"),
                    aperture = if (obj.isNull("aperture")) null else obj.optString("aperture"),
                    exposureBias = if (obj.isNull("exposureBias")) null else obj.optDouble("exposureBias").toFloat(),
                    isImported = obj.optBoolean("isImported", false),
                    sourceUri = if (obj.isNull("sourceUri")) null else obj.optString("sourceUri"),
                    mimeType = if (obj.isNull("mimeType")) null else obj.optString("mimeType"),
                    durationMs = if (obj.isNull("durationMs")) null else obj.optLong("durationMs"),
                    frameRate = if (obj.isNull("frameRate")) null else obj.optInt("frameRate"),
                    bitrate = if (obj.isNull("bitrate")) null else obj.optLong("bitrate"),
                    rotationDegrees = if (obj.isNull("rotationDegrees")) null else obj.optInt("rotationDegrees"),
                    hasAudio = if (obj.isNull("hasAudio")) null else obj.optBoolean("hasAudio"),
                    videoWidth = if (obj.isNull("videoWidth")) null else obj.optInt("videoWidth"),
                    videoHeight = if (obj.isNull("videoHeight")) null else obj.optInt("videoHeight"),
                    customProperties = mutableMapOf<String, String>().apply {
                        val customPropsObj = obj.optJSONObject("customProperties")
                        customPropsObj?.keys()?.forEach { key ->
                            put(key, customPropsObj.getString(key))
                        }
                    },
                    exportedUris = exportedUris,
                    computationalAperture = if (obj.isNull("computationalAperture")) null else obj.optDouble("computationalAperture")
                        .toFloat(),
                    focusPointX = if (obj.isNull("focusPointX")) null else obj.optDouble("focusPointX").toFloat(),
                    focusPointY = if (obj.isNull("focusPointY")) null else obj.optDouble("focusPointY").toFloat(),
                    presentationTimestampUs = if (obj.isNull("presentationTimestampUs")) null else obj.optLong("presentationTimestampUs"),
                    droMode = if (obj.isNull("droMode")) null else obj.optString("droMode"),
                    software = if (obj.isNull("software")) null else obj.optString("software"),
                    isMirrored = obj.optBoolean("isMirrored", false),
                    colorSpace = (if (obj.isNull("colorSpace")) null else obj.optString("colorSpace"))?.let {
                        ColorSpace.Named.valueOf(
                            it
                        )
                    } ?: ColorSpace.Named.SRGB,
                    manualHdrEffectEnabled = obj.optBoolean("manualHdrEffectEnabled", false),
                    hdrEffectStrength = HdrGainmapStrength.coerce(
                        if (obj.isNull("hdrEffectStrength")) null else obj.optDouble("hdrEffectStrength").toFloat()
                    ),
                    hasEmbeddedGainmap = obj.optBoolean("hasEmbeddedGainmap", false),
                    dynamicRangeProfile = if (obj.isNull("dynamicRangeProfile")) null else obj.optString("dynamicRangeProfile"),
                    captureMode = if (obj.isNull("captureMode")) null else obj.optString("captureMode"),
                    multipleExposureFrameCount = if (obj.isNull("multipleExposureFrameCount")) null else obj.optInt("multipleExposureFrameCount"),
                    hasAiDenoisedBase = obj.optBoolean("hasAiDenoisedBase", false),
                    aiDenoiseStrength = if (obj.isNull("aiDenoiseStrength")) null else obj.optDouble("aiDenoiseStrength").toFloat(),
                    applyEffectsToVideo = obj.optBoolean("applyEffectsToVideo", false),
                    spectralFilmStock = if (obj.isNull("spectralFilmStock")) null else obj.optString("spectralFilmStock"),
                    spectralFilmPrint = if (obj.isNull("spectralFilmPrint")) null else obj.optString("spectralFilmPrint"),
                    spectralFilmCDensityGain = if (obj.isNull("spectralFilmCDensityGain")) 1f else obj.optDouble("spectralFilmCDensityGain").toFloat(),
                    spectralFilmMDensityGain = if (obj.isNull("spectralFilmMDensityGain")) 1f else obj.optDouble("spectralFilmMDensityGain").toFloat(),
                    spectralFilmYDensityGain = if (obj.isNull("spectralFilmYDensityGain")) 1f else obj.optDouble("spectralFilmYDensityGain").toFloat(),
                )
            } catch (e: Exception) {
                PLog.e(TAG, "Failed to parse JSON", e)
                null
            }
        }

        
        fun createDefault(width: Int, height: Int): MediaMetadata {
            return MediaMetadata(
                deviceModel = DeviceUtil.model,
                brand = Build.MANUFACTURER.replaceFirstChar { it.uppercase() },
                dateTaken = System.currentTimeMillis(),
                width = width,
                height = height
            )
        }

        
        fun fromUri(context: Context, uri: Uri): MediaMetadata {
            var width = 0
            var height = 0
            var mimeType: String? = null
            var rotation = 0

            
            try {
                context.contentResolver.openInputStream(uri)?.use { stream ->
                    val options = BitmapFactory.Options().apply { inJustDecodeBounds = true }
                    BitmapFactory.decodeStream(stream, null, options)
                    width = options.outWidth
                    height = options.outHeight
                    mimeType = options.outMimeType
                }
            } catch (e: Exception) {
                PLog.w(TAG, "Failed to decode bounds from $uri", e)
            }

            if (mimeType == null) {
                mimeType = context.contentResolver.getType(uri)
            }

            return try {
                context.contentResolver.openInputStream(uri)?.use { inputStream ->
                    val exif = ExifInterface(inputStream)

                    val model = exif.getAttribute(ExifInterface.TAG_MODEL)
                    val make = exif.getAttribute(ExifInterface.TAG_MAKE)
                    val dateStr = exif.getAttribute(ExifInterface.TAG_DATETIME_ORIGINAL) ?: exif.getAttribute(
                        ExifInterface.TAG_DATETIME
                    )

                    val iso = exif.getAttributeInt(ExifInterface.TAG_ISO_SPEED_RATINGS, 0).takeIf { it > 0 }
                        ?: exif.getAttributeInt(ExifInterface.TAG_PHOTOGRAPHIC_SENSITIVITY, 0).takeIf { it > 0 }

                    val shutterSpeed = exif.getAttribute(ExifInterface.TAG_EXPOSURE_TIME)?.let {
                        try {
                            val time = it.toDouble()
                            if (time >= 1.0) {
                                "${time.toInt()}\""
                            } else {
                                "1/${(1.0 / time).toInt()}"
                            }
                        } catch (e: Exception) {
                            it
                        }
                    }

                    val aperture = exif.getAttributeDouble(ExifInterface.TAG_F_NUMBER, 0.0).takeIf { it > 0 }
                        ?.let { "f/${String.format("%.1f", it)}" }
                    val focalLength = exif.getAttributeDouble(ExifInterface.TAG_FOCAL_LENGTH, 0.0)
                        .takeIf { it > 0.0 }
                        ?.let { "${it.toInt()}mm" }
                    val focalLength35mm = exif.getAttributeInt(ExifInterface.TAG_FOCAL_LENGTH_IN_35MM_FILM, 0)
                        .takeIf { it > 0 }?.let { "${it}mm" }

                    val exifWidth = exif.getAttributeInt(ExifInterface.TAG_IMAGE_WIDTH, 0)
                        .let { if (it == 0) exif.getAttributeInt(ExifInterface.TAG_PIXEL_X_DIMENSION, 0) else it }
                    val exifHeight = exif.getAttributeInt(ExifInterface.TAG_IMAGE_LENGTH, 0)
                        .let { if (it == 0) exif.getAttributeInt(ExifInterface.TAG_PIXEL_Y_DIMENSION, 0) else it }

                    if (exifWidth > 0) width = exifWidth
                    if (exifHeight > 0) height = exifHeight

                    rotation = exif.getAttributeInt(ExifInterface.TAG_ORIENTATION, ExifInterface.ORIENTATION_NORMAL).let {
                        when (it) {
                            ExifInterface.ORIENTATION_ROTATE_90 -> 90
                            ExifInterface.ORIENTATION_ROTATE_180 -> 180
                            ExifInterface.ORIENTATION_ROTATE_270 -> 270
                            else -> 0
                        }
                    }

                    val dateTaken = dateStr?.let {
                        try {
                            SimpleDateFormat("yyyy:MM:dd HH:mm:ss", Locale.US).parse(it)?.time
                        } catch (e: Exception) {
                            null
                        }
                    }

                    val software = exif.getAttribute(ExifInterface.TAG_SOFTWARE)

                    val latLong = exif.latLong
                    val latitude = latLong?.get(0)
                    val longitude = latLong?.get(1)
                    val altitude = exif.getAltitude(0.0)
                        .takeIf { it != 0.0 || exif.getAttribute(ExifInterface.TAG_GPS_ALTITUDE) != null }

                    MediaMetadata(
                        deviceModel = model,
                        brand = make?.replaceFirstChar { it.uppercase() },
                        dateTaken = dateTaken,
                        iso = iso,
                        shutterSpeed = shutterSpeed,
                        focalLength = focalLength,
                        focalLength35mm = focalLength35mm,
                        aperture = aperture,
                        width = width,
                        height = height,
                        rotation = rotation,
                        latitude = latitude,
                        longitude = longitude,
                        altitude = altitude,
                        software = software,
                        mimeType = mimeType,
                        isImported = true
                    )
                } ?: createDefault(width, height).copy(mimeType = mimeType, isImported = true)
            } catch (e: Exception) {
                PLog.e(TAG, "Failed to load EXIF from $uri", e)
                createDefault(width, height).copy(mimeType = mimeType, isImported = true)
            }
        }
    }
}
