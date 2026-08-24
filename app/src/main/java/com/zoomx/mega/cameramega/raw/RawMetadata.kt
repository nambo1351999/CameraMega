package com.zoomx.mega.cameramega.raw

import android.R
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CaptureResult
import android.hardware.camera2.params.ColorSpaceTransform
import android.hardware.camera2.params.MeteringRectangle
import android.hardware.camera2.params.RggbChannelVector
import android.util.Log
import android.util.Rational
import com.zoomx.mega.cameramega.processor.CalibratedRawNoiseProfile
import com.zoomx.mega.cameramega.processor.PhotonCoreImagingTuning
import com.zoomx.mega.cameramega.processor.RawNoiseModel
import com.zoomx.mega.cameramega.processor.RawNoiseProfileSelection
import com.zoomx.mega.cameramega.utils.DeviceUtil
import com.zoomx.mega.cameramega.utils.PLog
import kotlin.collections.contentToString

enum class RawNoiseProfileLayout {
    NONE,
    
    CAMERA2_CFA,
    
    DNG_RGB,
    
    CANONICAL_BAYER,
}

data class RawMetadata(
    val width: Int,
    val height: Int,

    
    val cfaPattern: Int,

    
    val blackLevel: FloatArray,

    
    val whiteLevel: Float,

    
    val whiteBalanceGains: FloatArray,

    
    val preMul: FloatArray = floatArrayOf(1f, 1f, 1f, 1f),

    
    val colorCorrectionMatrix: FloatArray,

    
    val cameraWhite: FloatArray = floatArrayOf(1f, 1f, 1f),

    
    val whitePointXy: FloatArray? = null,

    
    val colorTemperature: Float? = null,

    
    val cameraMake: String? = null,
    val cameraModel: String? = null,

    
    val lensShadingMap: FloatArray? = null,
    val lensShadingMapWidth: Int = 0,
    val lensShadingMapHeight: Int = 0,
    
    val lensShadingMapGrid: FloatArray? = null,

    
    val postRawSensitivityBoost: Float = 1.0f,
    val baselineExposure: Float = 0.0f,
    val shadowScale: Float = 1.0f,
    
    val channelNoiseProfile: FloatArray = FloatArray(0),
    val noiseProfileLayout: RawNoiseProfileLayout = RawNoiseProfileLayout.NONE,
    val afRegions: Array<MeteringRectangle>? = null,
    val activeArray: android.graphics.Rect? = null,
    val defaultCrop: android.graphics.Rect? = null,
    val aeMode: Int = CaptureResult.CONTROL_AE_MODE_ON,
    val exposureCompensation: Float = 0f,
    val exposureBias: Float = 0f,
    val iso: Int = 100,
    
    val maxAnalogSensitivity: Int = 0,
    val shutterSpeed: Long = 0L,
    val aperture: Float = 0f,
    val frameCount: Int = 1,
    
    val mgcDenoiseCorrelation: FloatArray? = null,
    
    val mgcDenoiseReadNoise: FloatArray? = null,
    
    val mgcDenoiseShotNoise: FloatArray? = null,
    
    val mgcSpatialStrengthMap: MgcSpatialStrengthMap? = null,
    
    val mgcSabreNoiseModelScale: Float? = null,
    
    val mgcDenoiseTuningSnr: Float? = null,
    
    val mgcSharpenTuningSnr: Float? = null,
    
    val mgcSharpenAttenuationScale: Float? = null,
    
    val coreImagingTuning: PhotonCoreImagingTuning = PhotonCoreImagingTuning.DEFAULT,
    val rotation: Int? = null,
    val profileGainTableMap: DngProfileGainTableMap? = null,
    
    val sceneHasFace: Boolean = false,
) {
    fun withNoiseProfileSelection(selection: RawNoiseProfileSelection): RawMetadata {
        val profile = when (selection) {
            is RawNoiseProfileSelection.Calibrated -> selection.profile
            RawNoiseProfileSelection.Camera2 -> {
                val hasUsableSourceProfile = when (noiseProfileLayout) {
                    RawNoiseProfileLayout.CAMERA2_CFA,
                    RawNoiseProfileLayout.CANONICAL_BAYER ->
                        RawNoiseModel.fromCamera2NoiseProfile(channelNoiseProfile)
                            .hasValidCamera2Profile
                    RawNoiseProfileLayout.DNG_RGB ->
                        RawNoiseModel.fromDngNoiseProfile(channelNoiseProfile).let { model ->
                            model.shotNoise.all { it > 0f } &&
                                model.readNoise.any { it > 0f }
                        }
                    RawNoiseProfileLayout.NONE -> false
                }
                if (hasUsableSourceProfile) return this
                CalibratedRawNoiseProfile.MGC_GOOGLE_BLUELINE_REAR
            }
        }
        val model = profile.evaluate(iso)
        return if (model != null) {
            copy(
                channelNoiseProfile = model.canonicalChannelPairs(),
                noiseProfileLayout = RawNoiseProfileLayout.CANONICAL_BAYER,
            )
        } else {
            copy(
                channelNoiseProfile = FloatArray(0),
                noiseProfileLayout = RawNoiseProfileLayout.NONE,
            )
        }
    }

    companion object {
        private const val TAG = "RawMetadata"

        
        const val CFA_RGGB = 0
        const val CFA_GRBG = 1
        const val CFA_GBRG = 2
        const val CFA_BGGR = 3
        const val CFA_QUAD_RGGB = 4
        const val CFA_QUAD_GRBG = 5
        const val CFA_QUAD_GBRG = 6
        const val CFA_QUAD_BGGR = 7
        const val CFA_QUAD_8X8_RGGB = 8
        const val CFA_QUAD_8X8_GRBG = 9
        const val CFA_QUAD_8X8_GBRG = 10
        const val CFA_QUAD_8X8_BGGR = 11

        fun isQuadBayer(cfaPattern: Int): Boolean {
            return cfaPattern in CFA_QUAD_RGGB..CFA_QUAD_8X8_BGGR
        }

        fun isQuadBayer8x8(cfaPattern: Int): Boolean {
            return cfaPattern in CFA_QUAD_8X8_RGGB..CFA_QUAD_8X8_BGGR
        }

        
        fun create(
            width: Int,
            height: Int,
            characteristics: CameraCharacteristics,
            captureResult: CaptureResult,
            userExposureCompensation: Float? = null,
            colorSpace: ColorSpace = ColorSpace.SRGB
        ): RawMetadata {
            
            val cfaId = characteristics.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT)
                ?: CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB

            
            val activeArray = characteristics.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE)

            
            val xOffset = activeArray?.left ?: 0
            val yOffset = activeArray?.top ?: 0

            
            
            
            var correctedCfa = cfaId

            if (xOffset % 2 == 1) {
                correctedCfa = when (correctedCfa) {
                    CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB -> CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG
                    CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG -> CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB
                    CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG -> CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR
                    CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR -> CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG
                    else -> correctedCfa
                }
            }

            if (yOffset % 2 == 1) {
                correctedCfa = when (correctedCfa) {
                    CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB -> CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG
                    CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG -> CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR
                    CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG -> CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB
                    CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR -> CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG
                    else -> correctedCfa
                }
            }

            val cfaPattern = when (correctedCfa) {
                CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB -> CFA_RGGB
                CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG -> CFA_GRBG
                CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG -> CFA_GBRG
                CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR -> CFA_BGGR
                else -> CFA_RGGB 
            }

            
            val whiteLevel = characteristics.get(CameraCharacteristics.SENSOR_INFO_WHITE_LEVEL)?.toFloat()
                ?: 1023f 

            
            
            
            val dynamicBlackLevel = captureResult.get(CaptureResult.SENSOR_DYNAMIC_BLACK_LEVEL)
            val staticBlackLevelPattern = characteristics.get(CameraCharacteristics.SENSOR_BLACK_LEVEL_PATTERN)

            
            val positionBlackLevel = if (dynamicBlackLevel != null) {
                floatArrayOf(
                    dynamicBlackLevel[0],  
                    dynamicBlackLevel[1],  
                    dynamicBlackLevel[2],  
                    dynamicBlackLevel[3]   
                )
            } else if (staticBlackLevelPattern != null) {
                floatArrayOf(
                    staticBlackLevelPattern.getOffsetForIndex(0, 0).toFloat(),  
                    staticBlackLevelPattern.getOffsetForIndex(1, 0).toFloat(),  
                    staticBlackLevelPattern.getOffsetForIndex(0, 1).toFloat(),  
                    staticBlackLevelPattern.getOffsetForIndex(1, 1).toFloat()   
                )
            } else {
                
                floatArrayOf(64f, 64f, 64f, 64f)
            }

            
            
            
            
            
            
            val blackLevel = when (cfaPattern) {
                CFA_RGGB -> floatArrayOf(
                    positionBlackLevel[0],  
                    positionBlackLevel[1],  
                    positionBlackLevel[2],  
                    positionBlackLevel[3]   
                )

                CFA_GRBG -> floatArrayOf(
                    positionBlackLevel[1],  
                    positionBlackLevel[0],  
                    positionBlackLevel[3],  
                    positionBlackLevel[2]   
                )

                CFA_GBRG -> floatArrayOf(
                    positionBlackLevel[2],  
                    positionBlackLevel[3],  
                    positionBlackLevel[0],  
                    positionBlackLevel[1]   
                )

                CFA_BGGR -> floatArrayOf(
                    positionBlackLevel[3],  
                    positionBlackLevel[2],  
                    positionBlackLevel[1],  
                    positionBlackLevel[0]   
                )

                else -> positionBlackLevel
            }

            
            val wbGains = captureResult.get(CaptureResult.COLOR_CORRECTION_GAINS)
            val whiteBalanceGains = if (wbGains != null) {
                
                
                
                floatArrayOf(
                    wbGains.red,
                    wbGains.greenEven, 
                    wbGains.greenOdd,  
                    wbGains.blue
                )
            } else {
                
                floatArrayOf(1f, 1f, 1f, 1f)
            }

            
            
            val colorCorrectionMatrix = computeCCMFromCharacteristics(characteristics, captureResult, colorSpace)
            val cameraWhite = computeCameraWhiteFromCharacteristics(characteristics, captureResult)
            val whitePointXy = computeWhiteXyFromCharacteristics(characteristics, captureResult)
            val colorTemperature = whitePointXy?.let(DngSdkColorSpec::colorTemperatureForXy)

            
            val shadingMap = captureResult.get(CaptureResult.STATISTICS_LENS_SHADING_CORRECTION_MAP)
            var lensShadingMap: FloatArray? = null
            var shadingWidth = 0
            var shadingHeight = 0
            if (shadingMap != null) {
                shadingWidth = shadingMap.columnCount
                shadingHeight = shadingMap.rowCount
                lensShadingMap = FloatArray(shadingWidth * shadingHeight * 4)
                shadingMap.copyGainFactors(lensShadingMap, 0)
            }

            
            val boost = captureResult.get(CaptureResult.CONTROL_POST_RAW_SENSITIVITY_BOOST) ?: 100
            
            val postRawSensitivityBoost = boost / 100.0f

            
            val channelNoiseProfile = extractChannelNoiseProfile(captureResult)

            
            val aeMode = captureResult.get(CaptureResult.CONTROL_AE_MODE) ?: CaptureResult.CONTROL_AE_MODE_ON
            val evComp = captureResult.get(CaptureResult.CONTROL_AE_EXPOSURE_COMPENSATION) ?: 0
            val evStep = characteristics.get(CameraCharacteristics.CONTROL_AE_COMPENSATION_STEP) ?: Rational(1, 3)
            val exposureCompensation = evComp * evStep.toFloat()

            
            val iso = captureResult.get(CaptureResult.SENSOR_SENSITIVITY) ?: 100
            val maxAnalogSensitivity = characteristics.get(
                CameraCharacteristics.SENSOR_MAX_ANALOG_SENSITIVITY,
            ) ?: 0
            val shutterSpeed = captureResult.get(CaptureResult.SENSOR_EXPOSURE_TIME) ?: 0L
            val aperture = captureResult.get(CaptureResult.LENS_APERTURE) ?: 0f

            return RawMetadata(
                width = width,
                height = height,
                cfaPattern = cfaPattern,
                blackLevel = blackLevel,
                whiteLevel = whiteLevel,
                whiteBalanceGains = whiteBalanceGains,
                preMul = whiteBalanceGains.copyOf(),
                colorCorrectionMatrix = colorCorrectionMatrix,
                cameraWhite = cameraWhite,
                whitePointXy = whitePointXy,
                colorTemperature = colorTemperature,
                lensShadingMap = lensShadingMap,
                lensShadingMapWidth = shadingWidth,
                lensShadingMapHeight = shadingHeight,
                postRawSensitivityBoost = postRawSensitivityBoost,
                baselineExposure = 0f,
                channelNoiseProfile = channelNoiseProfile,
                noiseProfileLayout = if (channelNoiseProfile.size >= 8) {
                    RawNoiseProfileLayout.CAMERA2_CFA
                } else {
                    RawNoiseProfileLayout.NONE
                },
                afRegions = captureResult.get(CaptureResult.CONTROL_AF_REGIONS),
                activeArray = activeArray,
                aeMode = aeMode,
                exposureCompensation = exposureCompensation,
                exposureBias = userExposureCompensation ?: exposureCompensation,
                iso = iso,
                maxAnalogSensitivity = maxAnalogSensitivity,
                shutterSpeed = shutterSpeed,
                aperture = aperture,
                sceneHasFace = captureResult.get(CaptureResult.STATISTICS_FACES)
                    ?.isNotEmpty() == true,
            )
        }

        private fun extractChannelNoiseProfile(captureResult: CaptureResult): FloatArray {
            val noiseProfile = captureResult.get(CaptureResult.SENSOR_NOISE_PROFILE)
            return if (noiseProfile != null && noiseProfile.isNotEmpty()) {
                FloatArray(noiseProfile.size * 2) { index ->
                    val pair = noiseProfile[index / 2]
                    if (index % 2 == 0) {
                        sanitizeNoiseCoefficient(pair.first.toFloat())
                    } else {
                        sanitizeNoiseCoefficient(pair.second.toFloat())
                    }
                }
            } else {
                FloatArray(0)
            }
        }

        
        internal fun greenNoiseProfile(
            channelNoiseProfile: FloatArray,
            cfaPattern: Int,
            layout: RawNoiseProfileLayout,
        ): FloatArray {
            fun pairAt(pairIndex: Int): FloatArray? {
                val offset = pairIndex * 2
                if (offset + 1 >= channelNoiseProfile.size) return null
                val slope = sanitizeNoiseCoefficient(channelNoiseProfile[offset])
                val intercept = sanitizeNoiseCoefficient(channelNoiseProfile[offset + 1])
                if (slope <= 0f && intercept <= 0f) return null
                return floatArrayOf(slope, intercept)
            }

            if (layout == RawNoiseProfileLayout.DNG_RGB) {
                return pairAt(1) ?: floatArrayOf(0f, 0f)
            }
            if (layout == RawNoiseProfileLayout.CANONICAL_BAYER) {
                val firstGreen = pairAt(1)
                val secondGreen = pairAt(2)
                return if (firstGreen != null && secondGreen != null) {
                    floatArrayOf(
                        (firstGreen[0] + secondGreen[0]) * 0.5f,
                        (firstGreen[1] + secondGreen[1]) * 0.5f,
                    )
                } else {
                    firstGreen ?: secondGreen ?: floatArrayOf(0f, 0f)
                }
            }
            if (layout != RawNoiseProfileLayout.CAMERA2_CFA) return floatArrayOf(0f, 0f)

            val basePattern = cfaPattern.mod(4)
            val greenIndices = when (basePattern) {
                CFA_GRBG, CFA_GBRG -> 0 to 3
                else -> 1 to 2
            }
            val firstGreen = pairAt(greenIndices.first)
            val secondGreen = pairAt(greenIndices.second)
            return if (firstGreen != null && secondGreen != null) {
                floatArrayOf(
                    (firstGreen[0] + secondGreen[0]) * 0.5f,
                    (firstGreen[1] + secondGreen[1]) * 0.5f
                )
            } else {
                firstGreen ?: secondGreen ?: floatArrayOf(0f, 0f)
            }
        }

        
        internal fun redBlueNoiseProfile(
            channelNoiseProfile: FloatArray,
            cfaPattern: Int,
            layout: RawNoiseProfileLayout,
        ): FloatArray {
            fun pairAt(pairIndex: Int): FloatArray? {
                val offset = pairIndex * 2
                if (offset + 1 >= channelNoiseProfile.size) return null
                val slope = sanitizeNoiseCoefficient(channelNoiseProfile[offset])
                val intercept = sanitizeNoiseCoefficient(channelNoiseProfile[offset + 1])
                if (slope <= 0f && intercept <= 0f) return null
                return floatArrayOf(slope, intercept)
            }

            val (red, blue) = if (layout == RawNoiseProfileLayout.DNG_RGB) {
                pairAt(0) to pairAt(2)
            } else if (layout == RawNoiseProfileLayout.CANONICAL_BAYER) {
                pairAt(0) to pairAt(3)
            } else if (layout == RawNoiseProfileLayout.CAMERA2_CFA) {
                val indices = when (cfaPattern.mod(4)) {
                    CFA_GRBG -> 1 to 2
                    CFA_GBRG -> 2 to 1
                    CFA_BGGR -> 3 to 0
                    else -> 0 to 3
                }
                pairAt(indices.first) to pairAt(indices.second)
            } else {
                null to null
            }

            return floatArrayOf(
                red?.get(0) ?: 0f,
                red?.get(1) ?: 0f,
                blue?.get(0) ?: 0f,
                blue?.get(1) ?: 0f
            )
        }

        private fun sanitizeNoiseCoefficient(value: Float): Float {
            return value.takeIf { it.isFinite() }?.coerceAtLeast(0f) ?: 0f
        }

        
        private fun computeCCMFromCharacteristics(
            characteristics: CameraCharacteristics,
            captureResult: CaptureResult,
            colorSpace: ColorSpace = ColorSpace.SRGB
        ): FloatArray {
            val wbGains = captureResult.get(CaptureResult.COLOR_CORRECTION_GAINS)
            val whiteBalanceGains = if (wbGains != null) {
                floatArrayOf(wbGains.red, wbGains.greenEven, wbGains.greenOdd, wbGains.blue)
            } else {
                floatArrayOf(1f, 1f, 1f, 1f)
            }
            val forwardMatrix1 = if (DeviceUtil.isOppo) {
                null
            } else {
                characteristics.get(CameraCharacteristics.SENSOR_FORWARD_MATRIX1)?.let(::extractCCM)
            }
            val forwardMatrix2 = if (DeviceUtil.isOppo) {
                null
            } else {
                characteristics.get(CameraCharacteristics.SENSOR_FORWARD_MATRIX2)?.let(::extractCCM)
            }
            return DngSdkColorSpec.computeCameraToWorkingMatrix(
                colorMatrix1 = characteristics.get(CameraCharacteristics.SENSOR_COLOR_TRANSFORM1)?.let(::extractCCM),
                colorMatrix2 = characteristics.get(CameraCharacteristics.SENSOR_COLOR_TRANSFORM2)?.let(::extractCCM),
                forwardMatrix1 = forwardMatrix1,
                forwardMatrix2 = forwardMatrix2,
                calibrationIlluminant1 = characteristics.get(CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT1) ?: 0,
                calibrationIlluminant2 = characteristics.get(CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT2)?.toInt() ?: 0,
                whiteBalanceGains = whiteBalanceGains,
                workingColorSpace = colorSpace
            ) ?: run {
                Log.d(TAG, "No ForwardMatrix/ColorMatrix available, using identity matrix")
                floatArrayOf(1f, 0f, 0f, 0f, 1f, 0f, 0f, 0f, 1f)
            }
        }

        private fun computeCameraWhiteFromCharacteristics(
            characteristics: CameraCharacteristics,
            captureResult: CaptureResult
        ): FloatArray {
            val wbGains = captureResult.get(CaptureResult.COLOR_CORRECTION_GAINS)
            val whiteBalanceGains = if (wbGains != null) {
                floatArrayOf(wbGains.red, wbGains.greenEven, wbGains.greenOdd, wbGains.blue)
            } else {
                floatArrayOf(1f, 1f, 1f, 1f)
            }
            return DngSdkColorSpec.computeCameraWhite(
                colorMatrix1 = characteristics.get(CameraCharacteristics.SENSOR_COLOR_TRANSFORM1)?.let(::extractCCM),
                colorMatrix2 = characteristics.get(CameraCharacteristics.SENSOR_COLOR_TRANSFORM2)?.let(::extractCCM),
                forwardMatrix1 = characteristics.get(CameraCharacteristics.SENSOR_FORWARD_MATRIX1)?.let(::extractCCM),
                forwardMatrix2 = characteristics.get(CameraCharacteristics.SENSOR_FORWARD_MATRIX2)?.let(::extractCCM),
                calibrationIlluminant1 = characteristics.get(CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT1) ?: 0,
                calibrationIlluminant2 = characteristics.get(CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT2)?.toInt() ?: 0,
                whiteBalanceGains = whiteBalanceGains
            ) ?: floatArrayOf(1f, 1f, 1f)
        }

        private fun computeWhiteXyFromCharacteristics(
            characteristics: CameraCharacteristics,
            captureResult: CaptureResult
        ): FloatArray? {
            val wbGains = captureResult.get(CaptureResult.COLOR_CORRECTION_GAINS)
                ?: return null
            return DngSdkColorSpec.computeWhiteXy(
                colorMatrix1 = characteristics.get(
                    CameraCharacteristics.SENSOR_COLOR_TRANSFORM1
                )?.let(::extractCCM),
                colorMatrix2 = characteristics.get(
                    CameraCharacteristics.SENSOR_COLOR_TRANSFORM2
                )?.let(::extractCCM),
                forwardMatrix1 = characteristics.get(
                    CameraCharacteristics.SENSOR_FORWARD_MATRIX1
                )?.let(::extractCCM),
                forwardMatrix2 = characteristics.get(
                    CameraCharacteristics.SENSOR_FORWARD_MATRIX2
                )?.let(::extractCCM),
                calibrationIlluminant1 = characteristics.get(
                    CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT1
                ) ?: 0,
                calibrationIlluminant2 = characteristics.get(
                    CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT2
                )?.toInt() ?: 0,
                whiteBalanceGains = floatArrayOf(
                    wbGains.red,
                    wbGains.greenEven,
                    wbGains.greenOdd,
                    wbGains.blue
                )
            )
        }

        
        private fun calculateInterpolationWeight(
            illuminant1: Int?,
            illuminant2: Int?,
            wbGains: RggbChannelVector?,
            colorMatrix1: ColorSpaceTransform?,
            colorMatrix2: ColorSpaceTransform?
        ): Float {
            if (illuminant1 == null || illuminant2 == null || illuminant1 == 0 || illuminant2 == 0 || wbGains == null) {
                return 0.0f
            }

            val dngReferenceWeight = calculateDngReferenceInterpolationWeight(
                illuminant1 = illuminant1,
                illuminant2 = illuminant2,
                wbGains = wbGains,
                colorMatrix1 = colorMatrix1,
                colorMatrix2 = colorMatrix2
            )
            if (dngReferenceWeight != null) {
                return dngReferenceWeight
            }

            return calculateRatioInterpolationWeight(illuminant1, illuminant2, wbGains)
        }

        private fun calculateRatioInterpolationWeight(
            illuminant1: Int,
            illuminant2: Int,
            wbGains: RggbChannelVector
        ): Float {
            val t1 = illuminantToTemp(illuminant1)
            val t2 = illuminantToTemp(illuminant2)
            if (kotlin.math.abs(t1 - t2) < 100f) return 1.0f

            
            
            
            val currentRatio = wbGains.red / wbGains.blue

            
            val ratioWarm = 0.5f   
            val ratioCool = 1.6f   

            
            fun getTargetRatio(temp: Float): Float {
                return when {
                    temp <= 2856f -> ratioWarm
                    temp >= 6504f -> ratioCool
                    else -> ratioWarm + (ratioCool - ratioWarm) * (temp - 2856f) / (6504f - 2856f)
                }
            }

            val r1 = getTargetRatio(t1)
            val r2 = getTargetRatio(t2)

            
            val diff = r1 - r2
            if (kotlin.math.abs(diff) < 0.01f) return 0.5f

            val weight = (currentRatio - r2) / diff
            return weight.coerceIn(0.0f, 1.0f)
        }

        private fun calculateDngReferenceInterpolationWeight(
            illuminant1: Int,
            illuminant2: Int,
            wbGains: RggbChannelVector,
            colorMatrix1: ColorSpaceTransform?,
            colorMatrix2: ColorSpaceTransform?
        ): Float? {
            val matrix1 = colorMatrix1?.let { extractCCM(it) }
            val matrix2 = colorMatrix2?.let { extractCCM(it) }
            if (matrix1 == null && matrix2 == null) return null

            val green = ((wbGains.greenEven + wbGains.greenOdd) * 0.5f).takeIf { it > 1e-6f } ?: 1f
            val neutral = floatArrayOf(
                green / wbGains.red.coerceAtLeast(1e-6f),
                1f,
                green / wbGains.blue.coerceAtLeast(1e-6f)
            )
            val whiteXy = neutralToXy(neutral, matrix1, matrix2, illuminant1, illuminant2) ?: return null
            return calculateTemperatureInterpolationWeight(illuminant1, illuminant2, whiteXy)
        }

        private fun neutralToXy(
            neutral: FloatArray,
            colorMatrix1: FloatArray?,
            colorMatrix2: FloatArray?,
            illuminant1: Int,
            illuminant2: Int
        ): FloatArray? {
            var lastXy = floatArrayOf(0.3457f, 0.3585f)
            repeat(30) { pass ->
                val xyzToCamera = findXyzToCamera(lastXy, colorMatrix1, colorMatrix2, illuminant1, illuminant2)
                    ?: return null
                val cameraToXyz = invertMatrix3x3(xyzToCamera) ?: return null
                val nextXyz = multiplyMatrixVector(cameraToXyz, neutral)
                val nextXy = xyzToXy(nextXyz) ?: return null

                if (kotlin.math.abs(nextXy[0] - lastXy[0]) + kotlin.math.abs(nextXy[1] - lastXy[1]) < 1e-7f) {
                    return nextXy
                }
                if (pass == 29) {
                    nextXy[0] = (lastXy[0] + nextXy[0]) * 0.5f
                    nextXy[1] = (lastXy[1] + nextXy[1]) * 0.5f
                    return nextXy
                }
                lastXy = nextXy
            }
            return lastXy
        }

        private fun findXyzToCamera(
            whiteXy: FloatArray,
            colorMatrix1: FloatArray?,
            colorMatrix2: FloatArray?,
            illuminant1: Int,
            illuminant2: Int
        ): FloatArray? {
            if (colorMatrix1 != null && colorMatrix2 != null) {
                val weight = calculateTemperatureInterpolationWeight(illuminant1, illuminant2, whiteXy)
                return FloatArray(9) { index -> colorMatrix1[index] * weight + colorMatrix2[index] * (1f - weight) }
            }
            return colorMatrix1 ?: colorMatrix2
        }

        private fun calculateTemperatureInterpolationWeight(
            illuminant1: Int,
            illuminant2: Int,
            whiteXy: FloatArray
        ): Float {
            val t1 = illuminantToTemp(illuminant1)
            val t2 = illuminantToTemp(illuminant2)
            if (t1 <= 0f || t2 <= 0f || kotlin.math.abs(t1 - t2) < 1f) return 1f

            val whiteTemp = xyCoordToTemperature(whiteXy)
            val low = kotlin.math.min(t1, t2)
            val high = kotlin.math.max(t1, t2)
            val mix = when {
                whiteTemp <= low -> 1f
                whiteTemp >= high -> 0f
                else -> {
                    val invT = 1f / whiteTemp
                    (invT - (1f / high)) / ((1f / low) - (1f / high))
                }
            }.coerceIn(0f, 1f)
            return if (t1 > t2) 1f - mix else mix
        }

        private fun xyCoordToTemperature(xy: FloatArray): Float {
            val denominator = xy[1] - 0.1858f
            val safeDenominator = if (kotlin.math.abs(denominator) < 1e-6f) {
                if (denominator < 0f) -1e-6f else 1e-6f
            } else {
                denominator
            }
            val n = (xy[0] - 0.3320f) / safeDenominator
            return (-449f * n * n * n + 3525f * n * n - 6823.3f * n + 5520.33f)
                .coerceIn(2000f, 50000f)
        }

        private fun xyzToXy(xyz: FloatArray): FloatArray? {
            val sum = xyz[0] + xyz[1] + xyz[2]
            if (sum <= 1e-6f || xyz.any { !it.isFinite() }) return null
            return floatArrayOf(xyz[0] / sum, xyz[1] / sum)
        }

        private fun multiplyMatrixVector(matrix: FloatArray, vector: FloatArray): FloatArray {
            return floatArrayOf(
                matrix[0] * vector[0] + matrix[1] * vector[1] + matrix[2] * vector[2],
                matrix[3] * vector[0] + matrix[4] * vector[1] + matrix[5] * vector[2],
                matrix[6] * vector[0] + matrix[7] * vector[1] + matrix[8] * vector[2]
            )
        }

        
        private fun computeCamToXYZ(
            forwardMatrix: ColorSpaceTransform?,
            colorMatrix: ColorSpaceTransform?,
            illuminant: Int?
        ): FloatArray? {
            if (forwardMatrix != null) {
                return extractCCM(forwardMatrix)
            }
            if (colorMatrix != null) {
                val xyzToCam = extractCCM(colorMatrix)

                
                val ill = illuminant ?: 21 
                val (lx, ly, lz) = getIlluminantWhitePoint(ill)

                
                val cameraNeutral = FloatArray(3)
                for (i in 0 until 3) {
                    cameraNeutral[i] = xyzToCam[i * 3 + 0] * lx +
                            xyzToCam[i * 3 + 1] * ly +
                            xyzToCam[i * 3 + 2] * lz
                }

                
                val referenceMatrix = xyzToCam.copyOf()
                for (row in 0 until 3) {
                    val cn = if (kotlin.math.abs(cameraNeutral[row]) > 0.001f) cameraNeutral[row] else 1.0f
                    referenceMatrix[row * 3 + 0] /= cn
                    referenceMatrix[row * 3 + 1] /= cn
                    referenceMatrix[row * 3 + 2] /= cn
                }

                
                val m = invertMatrix3x3(referenceMatrix) ?: return null

                
                val adapt = getChromaticAdaptationMatrix(ill)
                return multiplyMatrix3x3(adapt, m)
            }
            return null
        }

        private fun getIlluminantWhitePoint(ill: Int): Triple<Float, Float, Float> {
            return if (ill == 17) { 
                Triple(1.0985f, 1.0000f, 0.3558f)
            } else { 
                Triple(0.9504f, 1.0000f, 1.0888f)
            }
        }

        private fun getChromaticAdaptationMatrix(ill: Int): FloatArray {
            return if (ill == 17) { 
                floatArrayOf(
                    0.8924f, -0.0157f, 0.0529f,
                    -0.1111f, 1.0505f, -0.0151f,
                    0.0522f, -0.0077f, 2.2396f
                )
            } else { 
                floatArrayOf(
                    1.0478f, 0.0229f, -0.0501f,
                    0.0295f, 0.9905f, -0.0170f,
                    -0.0092f, 0.0150f, 0.7521f
                )
            }
        }

        
        private fun illuminantToTemp(illuminant: Int): Float {
            return when (illuminant) {
                1 -> 5500f      
                2 -> 4000f      
                3 -> 3200f      
                4 -> 3400f      
                9 -> 6500f      
                10 -> 7500f     
                11 -> 8000f     
                12 -> 6500f     
                13 -> 5000f     
                14 -> 4200f     
                15 -> 3500f     
                17 -> 2856f     
                18 -> 4874f     
                19 -> 6774f     
                20 -> 5500f     
                21 -> 6504f     
                22 -> 7505f     
                23 -> 5000f     
                24 -> 3200f     
                else -> 5000f
            }
        }

        
        private fun computeXYZD50ToGamut(primaries: FloatArray, whitePoint: FloatArray): FloatArray? {
            if (primaries.size != 6 || whitePoint.size != 2) return null

            val xr = primaries[0];
            val yr = primaries[1]
            val xg = primaries[2];
            val yg = primaries[3]
            val xb = primaries[4];
            val yb = primaries[5]
            val xw = whitePoint[0];
            val yw = whitePoint[1]

            
            
            val mS = floatArrayOf(
                xr / yr, xg / yg, xb / yb,
                1f, 1f, 1f,
                (1 - xr - yr) / yr, (1 - xg - yg) / yg, (1 - xb - yb) / yb
            )
            val invS = invertMatrix3x3(mS) ?: return null

            
            val Xw = xw / yw
            val Yw = 1f
            val Zw = (1 - xw - yw) / yw

            
            val sR = invS[0] * Xw + invS[1] * Yw + invS[2] * Zw
            val sG = invS[3] * Xw + invS[4] * Yw + invS[5] * Zw
            val sB = invS[6] * Xw + invS[7] * Yw + invS[8] * Zw

            
            val gamutToXYZD65 = floatArrayOf(
                mS[0] * sR, mS[1] * sG, mS[2] * sB,
                mS[3] * sR, mS[4] * sG, mS[5] * sB,
                mS[6] * sR, mS[7] * sG, mS[8] * sB
            )

            val isD50WhitePoint = kotlin.math.abs(xw - 0.3457f) < 0.002f &&
                    kotlin.math.abs(yw - 0.3585f) < 0.002f

            val gamutToXYZD50 = if (isD50WhitePoint) {
                gamutToXYZD65
            } else {
                val bradfordD65ToD50 = floatArrayOf(
                    1.0478112f, 0.0228866f, -0.0501270f,
                    0.0295424f, 0.9904844f, -0.0170491f,
                    -0.0092345f, 0.0150436f, 0.7521316f
                )
                multiplyMatrix3x3(bradfordD65ToD50, gamutToXYZD65)
            }

            
            return invertMatrix3x3(gamutToXYZD50)
        }

        
        private fun invertMatrix3x3(m: FloatArray): FloatArray? {
            if (m.size != 9) return null

            
            val det = m[0] * (m[4] * m[8] - m[5] * m[7]) -
                    m[1] * (m[3] * m[8] - m[5] * m[6]) +
                    m[2] * (m[3] * m[7] - m[4] * m[6])

            if (kotlin.math.abs(det) < 1e-12f) {
                Log.e(TAG, "Matrix is singular, cannot invert")
                return null
            }

            val invDet = 1.0f / det
            return floatArrayOf(
                
                (m[4] * m[8] - m[5] * m[7]) * invDet,
                (m[2] * m[7] - m[1] * m[8]) * invDet,
                (m[1] * m[5] - m[2] * m[4]) * invDet,
                
                (m[5] * m[6] - m[3] * m[8]) * invDet,
                (m[0] * m[8] - m[2] * m[6]) * invDet,
                (m[2] * m[3] - m[0] * m[5]) * invDet,
                
                (m[3] * m[7] - m[4] * m[6]) * invDet,
                (m[1] * m[6] - m[0] * m[7]) * invDet,
                (m[0] * m[4] - m[1] * m[3]) * invDet
            )
        }

        
        private fun extractCCM(transform: ColorSpaceTransform): FloatArray {
            val matrix = FloatArray(9)
            for (row in 0 until 3) {
                for (col in 0 until 3) {
                    val rational: Rational = transform.getElement(col, row)
                    matrix[row * 3 + col] = rational.toFloat()
                }
            }
            return matrix
        }

        
        fun multiplyMatrix3x3(a: FloatArray, b: FloatArray): FloatArray {
            require(a.size == 9 && b.size == 9) { "Both matrices must be 3x3" }

            val result = FloatArray(9)
            for (i in 0 until 3) {
                for (j in 0 until 3) {
                    result[i * 3 + j] =
                        a[i * 3 + 0] * b[0 * 3 + j] +
                                a[i * 3 + 1] * b[1 * 3 + j] +
                                a[i * 3 + 2] * b[2 * 3 + j]
                }
            }
            return result
        }
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as RawMetadata

        if (width != other.width) return false
        if (height != other.height) return false
        if (cfaPattern != other.cfaPattern) return false
        if (!blackLevel.contentEquals(other.blackLevel)) return false
        if (whiteLevel != other.whiteLevel) return false
        if (!whiteBalanceGains.contentEquals(other.whiteBalanceGains)) return false
        if (!colorCorrectionMatrix.contentEquals(other.colorCorrectionMatrix)) return false
        if (!cameraWhite.contentEquals(other.cameraWhite)) return false
        if (baselineExposure != other.baselineExposure) return false
        if (shadowScale != other.shadowScale) return false
        if (iso != other.iso) return false
        if (maxAnalogSensitivity != other.maxAnalogSensitivity) return false
        if (shutterSpeed != other.shutterSpeed) return false
        if (frameCount != other.frameCount) return false
        if (!channelNoiseProfile.contentEquals(other.channelNoiseProfile)) return false
        if (noiseProfileLayout != other.noiseProfileLayout) return false
        if (mgcDenoiseCorrelation != null) {
            if (other.mgcDenoiseCorrelation == null) return false
            if (!mgcDenoiseCorrelation.contentEquals(other.mgcDenoiseCorrelation)) return false
        } else if (other.mgcDenoiseCorrelation != null) return false
        if (mgcDenoiseReadNoise != null) {
            if (other.mgcDenoiseReadNoise == null) return false
            if (!mgcDenoiseReadNoise.contentEquals(other.mgcDenoiseReadNoise)) return false
        } else if (other.mgcDenoiseReadNoise != null) return false
        if (mgcDenoiseShotNoise != null) {
            if (other.mgcDenoiseShotNoise == null) return false
            if (!mgcDenoiseShotNoise.contentEquals(other.mgcDenoiseShotNoise)) return false
        } else if (other.mgcDenoiseShotNoise != null) return false
        if (mgcSpatialStrengthMap != other.mgcSpatialStrengthMap) return false
        if (mgcSabreNoiseModelScale != other.mgcSabreNoiseModelScale) return false
        if (mgcDenoiseTuningSnr != other.mgcDenoiseTuningSnr) return false
        if (mgcSharpenTuningSnr != other.mgcSharpenTuningSnr) return false
        if (mgcSharpenAttenuationScale != other.mgcSharpenAttenuationScale) return false
        if (coreImagingTuning != other.coreImagingTuning) return false
        if (rotation != other.rotation) return false

        return true
    }

    override fun hashCode(): Int {
        var result = width
        result = 31 * result + height
        result = 31 * result + cfaPattern
        result = 31 * result + blackLevel.contentHashCode()
        result = 31 * result + whiteLevel.hashCode()
        result = 31 * result + whiteBalanceGains.contentHashCode()
        result = 31 * result + colorCorrectionMatrix.contentHashCode()
        result = 31 * result + cameraWhite.contentHashCode()
        result = 31 * result + (lensShadingMap?.contentHashCode() ?: 0)
        result = 31 * result + lensShadingMapWidth
        result = 31 * result + lensShadingMapHeight
        result = 31 * result + (lensShadingMapGrid?.contentHashCode() ?: 0)
        result = 31 * result + postRawSensitivityBoost.hashCode()
        result = 31 * result + baselineExposure.hashCode()
        result = 31 * result + shadowScale.hashCode()
        result = 31 * result + iso
        result = 31 * result + maxAnalogSensitivity
        result = 31 * result + shutterSpeed.hashCode()
        result = 31 * result + frameCount
        result = 31 * result + channelNoiseProfile.contentHashCode()
        result = 31 * result + noiseProfileLayout.hashCode()
        result = 31 * result + (mgcDenoiseCorrelation?.contentHashCode() ?: 0)
        result = 31 * result + (mgcDenoiseReadNoise?.contentHashCode() ?: 0)
        result = 31 * result + (mgcDenoiseShotNoise?.contentHashCode() ?: 0)
        result = 31 * result + (mgcSpatialStrengthMap?.hashCode() ?: 0)
        result = 31 * result + (mgcSabreNoiseModelScale?.hashCode() ?: 0)
        result = 31 * result + (mgcDenoiseTuningSnr?.hashCode() ?: 0)
        result = 31 * result + (mgcSharpenTuningSnr?.hashCode() ?: 0)
        result = 31 * result + (mgcSharpenAttenuationScale?.hashCode() ?: 0)
        result = 31 * result + coreImagingTuning.hashCode()
        result = 31 * result + (rotation ?: 0)
        return result
    }
}
