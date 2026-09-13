package com.mega.superx.filter.camera.utils

import android.graphics.*
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CaptureResult
import android.hardware.camera2.DngCreator
import android.media.ExifInterface
import android.media.Image
import android.util.Log
import android.util.Size
import com.mega.superx.filter.camera.camera.AspectRatio
import com.mega.superx.filter.camera.camera.CaptureInfo
import com.mega.superx.filter.camera.model.SafeImage
import com.mega.superx.filter.camera.processor.GpuLinearRgbSource
import com.mega.superx.filter.camera.raw.DngProfileGainTableMap
import com.mega.superx.filter.camera.raw.DngBaselineExposure
import com.mega.superx.filter.camera.raw.RawCfaCorrection
import com.mega.superx.filter.camera.raw.RawDngProfilePreparation
import com.mega.superx.filter.camera.raw.RawDngProfilePreparationOptions
import com.mega.superx.filter.camera.raw.RawMetadata
import com.mega.superx.filter.camera.raw.RawNoiseProfileLayout
import com.mega.superx.filter.camera.raw.RawDemosaicProcessor
import com.mega.superx.filter.camera.raw.RawRenderingEngine
import com.mega.superx.filter.camera.raw.RawSceneExposureDeviceLimits
import com.mega.superx.filter.camera.raw.RawWhiteLevelCorrection
import com.mega.superx.filter.camera.raw.toAdobeDefaultMeteringPlan
import java.io.File
import java.io.FileInputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.ShortBuffer

object RawProcessor {

    private const val TAG = "RawProcessor"
    private val BLACK_LEVEL_OVERRIDE_MODES = setOf("0", "16", "64", "256", "512", "Custom")

    enum class RawBufferValueDomain {
        SENSOR,
        NORMALIZED_SENSOR_RANGE,
    }

    fun resolveBlackLevelForMode(
        defaultBlackLevel: FloatArray,
        blackLevelMode: String?,
        customBlackLevel: Float?,
    ): FloatArray {
        val overrideBlackLevel = when (blackLevelMode) {
            "0" -> 0f
            "16" -> 16f
            "64" -> 64f
            "256" -> 256f
            "512" -> 512f
            "Custom" -> customBlackLevel ?: 0f
            else -> null
        }
        return overrideBlackLevel?.let { level ->
            FloatArray(defaultBlackLevel.size.coerceAtLeast(4)) { level }
        } ?: defaultBlackLevel.copyOf()
    }

    fun isBlackLevelOverrideMode(blackLevelMode: String?): Boolean =
        blackLevelMode in BLACK_LEVEL_OVERRIDE_MODES

    fun resolveCfaPatternForMode(defaultCfaPattern: Int, cfaCorrectionMode: String?): Int {
        return RawCfaCorrection.resolveCfaPattern(defaultCfaPattern, cfaCorrectionMode)
    }

    fun resolveWhiteLevelForMode(
        defaultWhiteLevel: Float,
        whiteLevelMode: String?,
        customWhiteLevel: Float? = null
    ): Float {
        return RawWhiteLevelCorrection.resolveWhiteLevel(defaultWhiteLevel, whiteLevelMode, customWhiteLevel)
    }

    
    fun canRenderDngBufferDirectly(
        width: Int,
        height: Int,
        characteristics: CameraCharacteristics,
    ): Boolean {
        val pixelArray = characteristics.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE)
        val activeArray = characteristics.get(
            CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE
        )
        val matchesPixelArray = pixelArray?.width == width && pixelArray.height == height
        if (!matchesPixelArray || activeArray == null) return true
        return activeArray.left == 0 && activeArray.top == 0 &&
            activeArray.right == width && activeArray.bottom == height
    }

    
    fun buildLinearDngRenderMetadata(
        width: Int,
        height: Int,
        characteristics: CameraCharacteristics,
        captureResult: CaptureResult,
        baseMetadata: RawMetadata,
        defaultCrop: Rect,
        rotation: Int,
        profilePreparation: RawDngProfilePreparation,
    ): RawMetadata {
        val dngChannelNoise = resolveDngWriterNoiseProfile(captureResult)
        val metadata = buildAdobeDngColorMetadata(
            width = width,
            height = height,
            characteristics = characteristics,
            captureResult = captureResult,
            userExposureCompensation = baseMetadata.exposureBias,
        )
        return metadata.copy(
            width = width,
            height = height,
            blackLevel = floatArrayOf(0f, 0f, 0f, 0f),
            whiteLevel = 65535f,
            lensShadingMap = null,
            lensShadingMapWidth = 0,
            lensShadingMapHeight = 0,
            lensShadingMapGrid = null,
            postRawSensitivityBoost = 1f,
            baselineExposure = DngBaselineExposure.sanitize(profilePreparation.baselineExposureEv),
            shadowScale = 1f,
            channelNoiseProfile = dngChannelNoise,
            noiseProfileLayout = RawNoiseProfileLayout.DNG_RGB,
            afRegions = null,
            activeArray = Rect(0, 0, width, height),
            defaultCrop = Rect(defaultCrop),
            aeMode = CaptureResult.CONTROL_AE_MODE_ON,
            exposureCompensation = 0f,
            exposureBias = baseMetadata.exposureBias,
            frameCount = 1,
            mgcSharpenTuningSnr = baseMetadata.mgcSharpenTuningSnr,
            mgcSharpenAttenuationScale = baseMetadata.mgcSharpenAttenuationScale,
            rotation = rotation,
            profileGainTableMap = profilePreparation.profileGainTableMap,
        )
    }

    
    fun buildCfaDngRenderMetadata(
        width: Int,
        height: Int,
        characteristics: CameraCharacteristics,
        captureResult: CaptureResult,
        sourceMetadata: RawMetadata,
        defaultCrop: Rect,
        rotation: Int,
        profilePreparation: RawDngProfilePreparation,
        blackLevelMode: String?,
        customBlackLevel: Float?,
        whiteLevelMode: String?,
        customWhiteLevel: Float?,
        cfaCorrectionMode: String?,
    ): RawMetadata {
        val resolvedCfaPattern = resolveCfaPatternForMode(
            sourceMetadata.cfaPattern,
            cfaCorrectionMode,
        )
        val resolvedBlackLevel = resolveBlackLevelForMode(
            sourceMetadata.blackLevel,
            blackLevelMode,
            customBlackLevel,
        )
        val resolvedWhiteLevel = resolveWhiteLevelForMode(
            sourceMetadata.whiteLevel,
            whiteLevelMode,
            customWhiteLevel,
        )
        return buildAdobeDngColorMetadata(
            width = width,
            height = height,
            characteristics = characteristics,
            captureResult = captureResult,
            userExposureCompensation = sourceMetadata.exposureBias,
        ).copy(
            width = width,
            height = height,
            cfaPattern = resolvedCfaPattern,
            blackLevel = resolvedBlackLevel,
            whiteLevel = resolvedWhiteLevel,
            postRawSensitivityBoost = 1f,
            baselineExposure = DngBaselineExposure.sanitize(profilePreparation.baselineExposureEv),
            shadowScale = 1f,
            channelNoiseProfile = resolveDngWriterNoiseProfile(captureResult),
            noiseProfileLayout = RawNoiseProfileLayout.DNG_RGB,
            afRegions = null,
            activeArray = Rect(0, 0, width, height),
            defaultCrop = Rect(defaultCrop),
            aeMode = CaptureResult.CONTROL_AE_MODE_ON,
            exposureCompensation = 0f,
            frameCount = 1,
            rotation = rotation,
            profileGainTableMap = profilePreparation.profileGainTableMap,
        )
    }

    
    private fun buildAdobeDngColorMetadata(
        width: Int,
        height: Int,
        characteristics: CameraCharacteristics,
        captureResult: CaptureResult,
        userExposureCompensation: Float? = null,
    ): RawMetadata {
        val dngWhiteBalance = resolveDngWriterWhiteBalance(captureResult)
        return RawMetadata.create(
            width = width,
            height = height,
            characteristics = characteristics,
            captureResult = captureResult,
            userExposureCompensation = userExposureCompensation,
            colorSpace = RawRenderingEngine.AdobeCurve.workingColorSpace,
        ).copy(
            whiteBalanceGains = dngWhiteBalance,
            preMul = dngWhiteBalance.copyOf(),
        )
    }

    private fun resolveDngWriterWhiteBalance(captureResult: CaptureResult): FloatArray {
        val neutral = captureResult.get(CaptureResult.SENSOR_NEUTRAL_COLOR_POINT)
            ?.takeIf { it.size >= 3 }
            ?.take(3)
            ?.map { value -> value.numerator.toDouble() / value.denominator.toDouble() }
            ?.takeIf { values -> values.all { it.isFinite() && it > 0.0 } }
        if (neutral != null) {
            val green = neutral[1]
            return floatArrayOf(
                (green / neutral[0]).toFloat(),
                1f,
                1f,
                (green / neutral[2]).toFloat(),
            )
        }

        val gains = captureResult.get(CaptureResult.COLOR_CORRECTION_GAINS)
            ?: return floatArrayOf(1f, 1f, 1f, 1f)
        val green = ((gains.greenEven + gains.greenOdd) * 0.5f)
            .takeIf { it.isFinite() && it > 0f }
            ?: 1f
        return floatArrayOf(
            gains.red.takeIf { it.isFinite() && it > 0f }?.div(green) ?: 1f,
            1f,
            1f,
            gains.blue.takeIf { it.isFinite() && it > 0f }?.div(green) ?: 1f,
        )
    }

    private fun resolveDngWriterNoiseProfile(captureResult: CaptureResult): FloatArray {
        val source = captureResult.get(CaptureResult.SENSOR_NOISE_PROFILE)
            ?.takeIf { it.size >= 3 }
            ?: return floatArrayOf(0f, 0f)

        fun safePair(index: Int): Pair<Float, Float>? {
            val pair = source.getOrNull(index) ?: return null
            val slope = pair.first.takeIf { it.isFinite() && it >= 0.0 }?.toFloat() ?: return null
            val offset = pair.second.takeIf { it.isFinite() && it >= 0.0 }?.toFloat() ?: return null
            return slope to offset
        }

        val red = safePair(0) ?: return floatArrayOf(0f, 0f)
        val green = if (source.size >= 4) {
            val even = safePair(1) ?: return floatArrayOf(0f, 0f)
            val odd = safePair(2) ?: return floatArrayOf(0f, 0f)
            (even.first + odd.first) * 0.5f to (even.second + odd.second) * 0.5f
        } else {
            safePair(1) ?: return floatArrayOf(0f, 0f)
        }
        val blue = safePair(if (source.size >= 4) 3 else 2)
            ?: return floatArrayOf(0f, 0f)
        return floatArrayOf(
            red.first,
            red.second,
            green.first,
            green.second,
            blue.first,
            blue.second,
            0f,
            0f,
        )
    }

    
    fun isRawImage(image: SafeImage): Boolean {
        return image.format == ImageFormat.RAW_SENSOR ||
                image.format == ImageFormat.RAW_PRIVATE ||
                image.format == ImageFormat.RAW10 ||
                image.format == ImageFormat.RAW12
    }

    fun processAndToBitmap(
        file: File,
        aspectRatio: AspectRatio?,
        cropRegion: Rect?,
        rotation: Int
    ): Bitmap? {
        val source = ImageDecoder.createSource(file)
        return processAndToBitmap(source, aspectRatio, cropRegion, rotation)
    }

    fun processAndToBitmap(
        source: ImageDecoder.Source,
        aspectRatio: AspectRatio?,
        cropRegion: Rect?,
        rotation: Int
    ): Bitmap? {
        return try {
            var decodedBitmap = ImageDecoder.decodeBitmap(source) { decoder, info, _ ->
                decoder.setTargetColorSpace(ColorSpace.get(ColorSpace.Named.EXTENDED_SRGB))
                decoder.allocator = ImageDecoder.ALLOCATOR_SOFTWARE
            }

            PLog.d(TAG, "DNG decoded: ${decodedBitmap.width}x${decodedBitmap.height} ${decodedBitmap.config}")

            
            if (rotation != 0) {
                val matrix = Matrix()
                matrix.postRotate(rotation.toFloat())
                val rotatedBitmap = Bitmap.createBitmap(
                    decodedBitmap, 0, 0,
                    decodedBitmap.width, decodedBitmap.height,
                    matrix, true
                )
                if (rotatedBitmap != decodedBitmap) {
                    decodedBitmap.recycle()
                }
                decodedBitmap = rotatedBitmap
            }

            
            val rect =
                BitmapUtils.calculateProcessedRect(decodedBitmap.width, decodedBitmap.height, aspectRatio, cropRegion)
            Log.d(TAG, "processAndToBitmap: $rect")
            val croppedBitmap = Bitmap.createBitmap(decodedBitmap, rect.left, rect.top, rect.width(), rect.height())
            if (croppedBitmap != decodedBitmap) {
                decodedBitmap.recycle()
            }

            croppedBitmap
        } catch (e: Exception) {
            PLog.e(TAG, "Fallback RAW processing also failed", e)
            null
        }
    }

    
    suspend fun saveToDng(
        image: SafeImage,
        characteristics: CameraCharacteristics,
        captureResult: CaptureResult,
        outputStream: java.io.OutputStream,
        rotation: Int = 0,
        thumbnail: Bitmap? = null,
        blackLevelMode: String? = null,
        customBlackLevel: Float? = null,
        whiteLevelMode: String? = null,
        customWhiteLevel: Float? = null,
        cfaCorrectionMode: String? = null,
        effectiveFocalLengthMm: Float? = null,
        effectiveFocalLength35mm: Int? = null,
        captureInfo: CaptureInfo,
        dngProfilePreparationOptions: RawDngProfilePreparationOptions? = null,
        defaultCropOverride: Rect? = null,
    ): Boolean {
        if (!isRawImage(image)) {
            throw IllegalArgumentException("Image is not RAW format: ${image.format}")
        }

        return saveRawImageToDngWithCustomWriter(
            image = image,
            characteristics = characteristics,
            captureResult = captureResult,
            outputStream = outputStream,
            rotation = rotation,
            thumbnail = thumbnail,
            blackLevelMode = blackLevelMode,
            customBlackLevel = customBlackLevel,
            whiteLevelMode = whiteLevelMode,
            customWhiteLevel = customWhiteLevel,
            cfaCorrectionMode = cfaCorrectionMode,
            effectiveFocalLengthMm = effectiveFocalLengthMm,
            effectiveFocalLength35mm = effectiveFocalLength35mm,
            captureInfo = captureInfo,
            dngProfilePreparationOptions = dngProfilePreparationOptions,
            defaultCrop = defaultCropOverride ?: resolveCameraRawDefaultCrop(
                width = image.width,
                height = image.height,
                characteristics = characteristics,
                captureResult = captureResult,
            ),
        )
    }

    internal fun resolveCameraRawDefaultCrop(
        width: Int,
        height: Int,
        characteristics: CameraCharacteristics,
        captureResult: CaptureResult,
    ): Rect {
        val pixelArray = characteristics.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE)
        val preCorrection = characteristics.get(
            CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE
        ) ?: Rect(0, 0, width, height)
        val postCorrection = characteristics.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE)
        val bufferIncludesPixelArray = pixelArray?.width == width && pixelArray.height == height
        val targetWidth = if (bufferIncludesPixelArray) preCorrection.width() else width
        val targetHeight = if (bufferIncludesPixelArray) preCorrection.height() else height
        val scalerCropRegion = captureResult.get(CaptureResult.SCALER_CROP_REGION)
        val zoomRatio = captureResult.get(CaptureResult.CONTROL_ZOOM_RATIO) ?: 1f
        val distortionMode = captureResult.get(CaptureResult.DISTORTION_CORRECTION_MODE)
        val hasDistortionCorrectionControl = characteristics.get(
            CameraCharacteristics.DISTORTION_CORRECTION_AVAILABLE_MODES
        ) != null
        val mapped = RawDngCropMapper.mapToDefaultCrop(
            preCorrectionActiveArray = preCorrection.toRawCropRect(),
            postCorrectionActiveArray = postCorrection?.toRawCropRect(),
            scalerCropRegion = scalerCropRegion?.toRawCropRect(),
            zoomRatio = zoomRatio,
            usePreCorrectionCoordinateSystem = hasDistortionCorrectionControl &&
                distortionMode == CaptureResult.DISTORTION_CORRECTION_MODE_OFF,
            targetWidth = targetWidth,
            targetHeight = targetHeight,
        )
        val defaultCrop = Rect(mapped.left, mapped.top, mapped.right, mapped.bottom)
        PLog.i(
            TAG,
            "RAW_CROP_TRACE stage=CAMERA2_RESULT buffer=${width}x$height " +
                "pixelArray=$pixelArray activePre=$preCorrection activePost=$postCorrection " +
                "scalerCrop=$scalerCropRegion scalerReturned=${scalerCropRegion != null} " +
                "zoomRatio=$zoomRatio distortionMode=$distortionMode " +
                "distortionControl=$hasDistortionCorrectionControl " +
                "coordinateSpace=${if (hasDistortionCorrectionControl && distortionMode == CaptureResult.DISTORTION_CORRECTION_MODE_OFF) "PRE_CORRECTION" else "POST_CORRECTION"} " +
                "dngTarget=${targetWidth}x$targetHeight mappedDefaultCrop=$defaultCrop"
        )
        return defaultCrop
    }

    private fun Rect.toRawCropRect(): RawCropRect = RawCropRect(left, top, right, bottom)

    private suspend fun saveRawImageToDngWithCustomWriter(
        image: SafeImage,
        characteristics: CameraCharacteristics,
        captureResult: CaptureResult,
        outputStream: java.io.OutputStream,
        rotation: Int,
        thumbnail: Bitmap?,
        blackLevelMode: String?,
        customBlackLevel: Float?,
        whiteLevelMode: String?,
        customWhiteLevel: Float?,
        cfaCorrectionMode: String?,
        effectiveFocalLengthMm: Float?,
        effectiveFocalLength35mm: Int?,
        captureInfo: CaptureInfo,
        dngProfilePreparationOptions: RawDngProfilePreparationOptions?,
        defaultCrop: Rect,
    ): Boolean {
        if (image.format != ImageFormat.RAW_SENSOR) {
            PLog.w(TAG, "Custom DNG writer requires RAW_SENSOR input, got format=${image.format}")
            return false
        }

        val rawBuffer = copyRawSensorImageToContiguousBuffer(image) ?: return false
        val rawMetadata = RawMetadata.create(
            width = image.width,
            height = image.height,
            characteristics = characteristics,
            captureResult = captureResult
        )

        PLog.i(TAG, "Writing RAW_SENSOR DNG with custom writer")
        return try {
            saveRawBufferToDng(
                rawBuffer = rawBuffer,
                width = image.width,
                height = image.height,
                characteristics = characteristics,
                captureResult = captureResult,
                outputStream = outputStream,
                rotation = rotation,
                thumbnail = thumbnail,
                cfaPattern = rawMetadata.cfaPattern,
                blackLevel = rawMetadata.blackLevel,
                whiteLevel = rawMetadata.whiteLevel.toInt(),
                valueDomain = RawBufferValueDomain.SENSOR,
                customWriter = true,
                blackLevelMode = blackLevelMode,
                customBlackLevel = customBlackLevel,
                whiteLevelMode = whiteLevelMode,
                customWhiteLevel = customWhiteLevel,
                cfaCorrectionMode = cfaCorrectionMode,
                effectiveFocalLengthMm = effectiveFocalLengthMm,
                effectiveFocalLength35mm = effectiveFocalLength35mm,
                captureInfo = captureInfo,
                dngProfilePreparationOptions = dngProfilePreparationOptions,
                defaultCrop = defaultCrop,
            )
        } finally {
            LargeDirectBuffer.free(rawBuffer)
        }
    }

    internal fun copyRawSensorImageToContiguousBuffer(image: SafeImage): ByteBuffer? {
        val plane = image.planes.firstOrNull() ?: return null
        val rowStride = plane.rowStride
        val pixelStride = runCatching { plane.pixelStride }.getOrDefault(2).takeIf { it > 0 } ?: 2
        val width = image.width
        val height = image.height
        val rowBytes = width * 2

        if (pixelStride != 2 || rowStride < rowBytes) {
            PLog.w(TAG, "Unsupported RAW_SENSOR plane stride row=$rowStride pixel=$pixelStride size=${width}x${height}")
            return null
        }

        val source = plane.buffer.duplicate()
        val output = LargeDirectBuffer.allocate(
            rowBytes.toLong() * height.toLong(),
            "RAW_SENSOR contiguous copy",
        )?.order(ByteOrder.nativeOrder()) ?: return null
        for (row in 0 until height) {
            val rowOffset = row * rowStride
            val rowEnd = rowOffset + rowBytes
            if (rowEnd > source.capacity()) {
                PLog.w(TAG, "RAW_SENSOR plane too small row=$row rowEnd=$rowEnd capacity=${source.capacity()}")
                LargeDirectBuffer.free(output)
                return null
            }
            source.limit(rowEnd)
            source.position(rowOffset)
            output.put(source)
        }
        output.rewind()
        return output
    }

    suspend fun prepareRawDngProfile(
        rawBuffer: ByteBuffer?,
        gpuLinearRgbSource: GpuLinearRgbSource? = null,
        width: Int,
        height: Int,
        characteristics: CameraCharacteristics,
        captureResult: CaptureResult,
        cfaPattern: Int = RawMetadata.CFA_RGGB,
        blackLevel: FloatArray = floatArrayOf(0f, 0f, 0f, 0f),
        whiteLevel: Int = 65535,
        valueDomain: RawBufferValueDomain = RawBufferValueDomain.SENSOR,
        blackLevelMode: String? = null,
        customBlackLevel: Float? = null,
        whiteLevelMode: String? = null,
        customWhiteLevel: Float? = null,
        cfaCorrectionMode: String? = null,
        baselineExposureEv: Float? = null,
        imageLayout: SuperResolutionDngWriter.ImageLayout = SuperResolutionDngWriter.ImageLayout.CFA,
        inputRowStepSamples: Int? = null,
        inputColStepSamples: Int? = null,
        pixelsIncludeLensShadingCorrection: Boolean = false,
        options: RawDngProfilePreparationOptions,
        defaultCrop: Rect,
    ): RawDngProfilePreparation? {
        val resolvedCfaPattern = resolveCfaPatternForMode(cfaPattern, cfaCorrectionMode)
        val resolvedBlackLevel = resolveBlackLevelForMode(blackLevel, blackLevelMode, customBlackLevel)
        val resolvedWhiteLevel = resolveWhiteLevelForMode(
            whiteLevel.toFloat(),
            whiteLevelMode,
            customWhiteLevel,
        ).toInt()
        val sourceBaselineExposureEv = DngBaselineExposure.sanitize(
            baselineExposureEv ?: captureResult
                .get(CaptureResult.CONTROL_POST_RAW_SENSITIVITY_BOOST)
                ?.takeIf { it > 0 }
                ?.let { kotlin.math.log2(it / 100.0).toFloat() }
                ?: 0f
        )
        val inputSamplesPerPixel = inputColStepSamples ?: imageLayout.samplesPerPixel
        val inputRowStrideBytes = (inputRowStepSamples
            ?: width * inputSamplesPerPixel) * Short.SIZE_BYTES
        val statsBlackLevel = if (valueDomain == RawBufferValueDomain.NORMALIZED_SENSOR_RANGE) {
            FloatArray(if (inputSamplesPerPixel >= 3) 3 else 4)
        } else {
            resolvedBlackLevel
        }
        val statsWhiteLevel = if (valueDomain == RawBufferValueDomain.NORMALIZED_SENSOR_RANGE) {
            65535f
        } else {
            resolvedWhiteLevel.toFloat()
        }
        val cameraStatsMetadata = buildAdobeDngColorMetadata(
            width = width,
            height = height,
            characteristics = characteristics,
            captureResult = captureResult,
        ).copy(
            width = width,
            height = height,
            cfaPattern = resolvedCfaPattern,
            blackLevel = statsBlackLevel,
            whiteLevel = statsWhiteLevel,
            baselineExposure = sourceBaselineExposureEv,
            defaultCrop = defaultCrop,
        )
        val statsMetadata = if (pixelsIncludeLensShadingCorrection) {
            cameraStatsMetadata.copy(
                lensShadingMap = null,
                lensShadingMapWidth = 0,
                lensShadingMapHeight = 0,
                lensShadingMapGrid = null,
            ).also {
                PLog.i(
                    TAG,
                    "RAW LSC ownership: fused pixels already corrected; " +
                        "capture profile will not apply Camera2/DNG gain map",
                )
            }
        } else {
            cameraStatsMetadata
        }
        val embeddedDngColorPlan = SuperResolutionDngWriter.resolveEmbeddedRenderPlan(
            characteristics = characteristics,
            metadata = statsMetadata,
            imageLayout = imageLayout,
            profileGainTableMap = null,
            profileToneCurve = null,
        )
        if (embeddedDngColorPlan == null) {
            PLog.e(TAG, "Unable to resolve the DNG color plan for RAW capture-profile preparation")
            return null
        }
        
        
        val meteringRenderPlan = embeddedDngColorPlan.toAdobeDefaultMeteringPlan()
        val captureProfileMetadata = statsMetadata.copy(
            colorCorrectionMatrix = meteringRenderPlan.colorCorrectionMatrix.copyOf(),
            cameraWhite = meteringRenderPlan.cameraWhite.copyOf(),
        )
        val captureProfile = options.captureProfilePreparer?.prepare(
            com.mega.superx.filter.camera.raw.RawDngCaptureProfileInput(
                rawData = rawBuffer?.duplicate()?.order(ByteOrder.nativeOrder()),
                width = width,
                height = height,
                rowStride = inputRowStrideBytes,
                samplesPerPixel = inputSamplesPerPixel,
                metadata = captureProfileMetadata.copy(profileGainTableMap = null),
                meteringRenderPlan = meteringRenderPlan,
                gpuLinearRgbSource = gpuLinearRgbSource,
                sceneExposureDeviceLimits = RawSceneExposureDeviceLimits
                    .fromCameraCharacteristics(characteristics),
            )
        )
        val exposureOffsetEv = captureProfile?.exposureOffsetEv
            ?.takeIf { it.isFinite() }
            ?.coerceIn(
                com.mega.superx.filter.camera.raw.MeteringSystem.RAW_EXPOSURE_MIN_EV,
                com.mega.superx.filter.camera.raw.MeteringSystem.RAW_EXPOSURE_MAX_EV,
            )
        val finalBaselineExposureEv = DngBaselineExposure.resolveCaptureBaseline(
            sourceBaselineEv = sourceBaselineExposureEv,
            sceneBaselineEv = exposureOffsetEv,
        )
        val profileRequired = options.generatePhotonPgtm
        if (profileRequired && captureProfile?.profileGainTableMap == null) {
            PLog.e(
                TAG,
                "GPU RAW profile preparation failed: photonPgtm=${options.generatePhotonPgtm} " +
                    "size=${width}x$height samplesPerPixel=$inputSamplesPerPixel"
            )
            return null
        }
        val preparedProfileGainTableMap = captureProfile?.profileGainTableMap?.let { map ->
            if (map.sourceTag == DngProfileGainTableMap.TAG_PROFILE_GAIN_TABLE_MAP2) {
                map
            } else {
                map.copy(sourceTag = DngProfileGainTableMap.TAG_PROFILE_GAIN_TABLE_MAP2)
            }
        }
        return RawDngProfilePreparation(
            baselineExposureEv = finalBaselineExposureEv,
            profileGainTableMap = preparedProfileGainTableMap,
        ).also { finalProfile ->
            PLog.i(
                TAG,
                "RAW_SCENE_EXPOSURE stage=SHARED_PROFILE_READY " +
                    "enabled=${options.captureProfilePreparer != null} " +
                    "estimated=${exposureOffsetEv != null} " +
                    "sourceBaselineEv=$sourceBaselineExposureEv " +
                    "sourceBaselineGain=${DngBaselineExposure.exactGain(sourceBaselineExposureEv)} " +
                    "sceneBaselineEv=$exposureOffsetEv " +
                    "sceneBaselineGain=${exposureOffsetEv?.let(DngBaselineExposure::exactGain)} " +
                    "sourceBaselineApplied=${exposureOffsetEv == null} " +
                    "finalBaselineEv=${finalProfile.baselineExposureEv} " +
                    "finalBaselineGain=${DngBaselineExposure.exactGain(finalProfile.baselineExposureEv)} " +
                    "pgtm=${finalProfile.profileGainTableMap != null} " +
                    "pgtmSource=GPU photon=${options.generatePhotonPgtm} statsBounds=${options.statsBounds}"
            )
        }
    }

    suspend fun saveRawBufferToDng(
        rawBuffer: ByteBuffer,
        width: Int,
        height: Int,
        characteristics: CameraCharacteristics,
        captureResult: CaptureResult,
        captureMetadataResult: CaptureResult? = null,
        effectiveFocalLengthMm: Float? = null,
        effectiveFocalLength35mm: Int? = null,
        captureInfo: CaptureInfo,
        outputStream: java.io.OutputStream,
        rotation: Int = 0,
        thumbnail: Bitmap? = null,
        cfaPattern: Int = RawMetadata.CFA_RGGB,
        blackLevel: FloatArray = floatArrayOf(0f, 0f, 0f, 0f),
        whiteLevel: Int = 65535,
        valueDomain: RawBufferValueDomain = RawBufferValueDomain.SENSOR,
        customWriter: Boolean = false,
        blackLevelMode: String? = null,
        customBlackLevel: Float? = null,
        whiteLevelMode: String? = null,
        customWhiteLevel: Float? = null,
        cfaCorrectionMode: String? = null,
        baselineExposureEv: Float? = null,
        profileGainTableMap: DngProfileGainTableMap? = null,
        profileName: String? = null,
        profileToneCurve: FloatArray? = null,
        imageLayout: SuperResolutionDngWriter.ImageLayout = SuperResolutionDngWriter.ImageLayout.CFA,
        compression: SuperResolutionDngWriter.Compression = SuperResolutionDngWriter.Compression.UNCOMPRESSED,
        inputRowStepSamples: Int? = null,
        inputColStepSamples: Int? = null,
        pixelsIncludeLensShadingCorrection: Boolean = false,
        dngProfilePreparationOptions: RawDngProfilePreparationOptions? = null,
        defaultCrop: Rect,
        preparedDngProfile: RawDngProfilePreparation? = null,
    ): Boolean {
        val resolvedCfaPattern = resolveCfaPatternForMode(cfaPattern, cfaCorrectionMode)
        val resolvedBlackLevel = resolveBlackLevelForMode(blackLevel, blackLevelMode, customBlackLevel)
        val resolvedWhiteLevel = resolveWhiteLevelForMode(whiteLevel.toFloat(), whiteLevelMode, customWhiteLevel).toInt()
        val hasCfaOverride = RawCfaCorrection.isOverrideMode(cfaCorrectionMode)
        val hasWhiteLevelOverride = RawWhiteLevelCorrection.isOverrideMode(whiteLevelMode)
        val requiresCustomWriter = imageLayout != SuperResolutionDngWriter.ImageLayout.CFA ||
                compression != SuperResolutionDngWriter.Compression.UNCOMPRESSED ||
                dngProfilePreparationOptions != null ||
                profileGainTableMap != null ||
                profileToneCurve != null ||
                pixelsIncludeLensShadingCorrection
        if (hasCfaOverride && resolvedCfaPattern != cfaPattern) {
            PLog.d(TAG, "RAW DNG CFA override mode=$cfaCorrectionMode cfa=$cfaPattern->$resolvedCfaPattern")
        }
        if (resolvedWhiteLevel != whiteLevel) {
            PLog.d(TAG, "RAW DNG white level override mode=$whiteLevelMode white=$whiteLevel->$resolvedWhiteLevel")
        }

        val preparedProfile = preparedDngProfile ?: dngProfilePreparationOptions?.let { options ->
            prepareRawDngProfile(
                rawBuffer = rawBuffer,
                width = width,
                height = height,
                characteristics = characteristics,
                captureResult = captureResult,
                cfaPattern = cfaPattern,
                blackLevel = blackLevel,
                whiteLevel = whiteLevel,
                valueDomain = valueDomain,
                blackLevelMode = blackLevelMode,
                customBlackLevel = customBlackLevel,
                whiteLevelMode = whiteLevelMode,
                customWhiteLevel = customWhiteLevel,
                cfaCorrectionMode = cfaCorrectionMode,
                baselineExposureEv = baselineExposureEv,
                imageLayout = imageLayout,
                inputRowStepSamples = inputRowStepSamples,
                inputColStepSamples = inputColStepSamples,
                pixelsIncludeLensShadingCorrection = pixelsIncludeLensShadingCorrection,
                options = options,
                defaultCrop = defaultCrop,
            )
        }
        if (dngProfilePreparationOptions != null && preparedProfile == null) return false
        val writtenBaselineExposureEv = preparedProfile?.baselineExposureEv ?: baselineExposureEv
        val writtenProfileGainTableMap = if (dngProfilePreparationOptions != null) {
            preparedProfile?.profileGainTableMap
        } else {
            profileGainTableMap
        }
        val writtenProfileName = profileName
        val orientation = when (rotation) {
            90 -> ExifInterface.ORIENTATION_ROTATE_90
            180 -> ExifInterface.ORIENTATION_ROTATE_180
            270 -> ExifInterface.ORIENTATION_ROTATE_270
            else -> ExifInterface.ORIENTATION_NORMAL
        }
        if (customWriter || requiresCustomWriter || hasCfaOverride || hasWhiteLevelOverride || !canDngCreatorWriteBuffer(width, height, characteristics)) {
            PLog.i(TAG, "Writing stacked RAW DNG with custom writer: ${width}x${height} layout=$imageLayout compression=$compression")
            return SuperResolutionDngWriter.write(
                outputStream = outputStream,
                rawBuffer = rawBuffer,
                width = width,
                height = height,
                characteristics = characteristics,
                captureResult = captureResult,
                captureMetadataResult = captureMetadataResult ?: captureResult,
                effectiveFocalLengthMm = effectiveFocalLengthMm,
                effectiveFocalLength35mm = effectiveFocalLength35mm,
                captureInfo = captureInfo,
                orientation = orientation,
                cfaPattern = resolvedCfaPattern,
                blackLevel = blackLevel,
                whiteLevel = resolvedWhiteLevel,
                valueDomain = valueDomain,
                blackLevelMode = blackLevelMode,
                customBlackLevel = customBlackLevel,
                whiteLevelMode = whiteLevelMode,
                customWhiteLevel = customWhiteLevel,
                baselineExposureEv = writtenBaselineExposureEv,
                profileGainTableMap = writtenProfileGainTableMap,
                profileName = writtenProfileName,
                profileToneCurve = profileToneCurve,
                imageLayout = imageLayout,
                compression = compression,
                inputRowStepSamples = inputRowStepSamples,
                inputColStepSamples = inputColStepSamples,
                pixelsIncludeLensShadingCorrection = pixelsIncludeLensShadingCorrection,
                defaultCrop = defaultCrop,
            )
        }

        val dngCreator = DngCreator(characteristics, captureResult)
        return try {
            dngCreator.setOrientation(orientation)

            val dngInputBuffer = rawBuffer.duplicate().order(ByteOrder.nativeOrder())
            if (valueDomain == RawBufferValueDomain.NORMALIZED_SENSOR_RANGE) {
                denormalizeNormalizedRawBufferInPlace(
                    rawBuffer = dngInputBuffer,
                    width = width,
                    height = height,
                    cfaPattern = resolvedCfaPattern,
                    blackLevel = blackLevel,
                    whiteLevel = resolvedWhiteLevel
                )
            }
            dngInputBuffer.rewind()
            dngCreator.writeByteBuffer(outputStream, Size(width, height), dngInputBuffer, 0)
            true
        } catch (e: Exception) {
            PLog.w(TAG, "Failed to save stacked RAW buffer as DNG, ignoring", e)
            false
        } finally {
            dngCreator.close()
        }
    }

    internal fun denormalizeNormalizedRawBufferInPlace(
        rawBuffer: ByteBuffer,
        width: Int,
        height: Int,
        cfaPattern: Int,
        blackLevel: FloatArray,
        whiteLevel: Int,
    ) {
        val output = rawBuffer.order(ByteOrder.nativeOrder()).asShortBuffer()
        var index = 0
        for (y in 0 until height) {
            for (x in 0 until width) {
                val channelIndex = RawCfaCorrection.channelIndexForPixel(cfaPattern, x, y)
                val encoded = output.get(index).toInt() and 0xFFFF
                val channelBlackLevel = blackLevel.getOrElse(channelIndex) { 0f }
                val channelWhiteLevel = whiteLevel.coerceAtLeast(channelBlackLevel.toInt() + 1)
                val sensorValue = ((encoded / 65535f) * (channelWhiteLevel - channelBlackLevel) + channelBlackLevel)
                    .toInt()
                    .coerceIn(0, channelWhiteLevel)
                output.put(index, sensorValue.toShort())
                index++
            }
        }
    }

    private fun canDngCreatorWriteBuffer(
        width: Int,
        height: Int,
        characteristics: CameraCharacteristics,
    ): Boolean {
        val pixelArraySize = characteristics.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE)
        if (pixelArraySize?.width == width && pixelArraySize.height == height) {
            return true
        }
        val preCorrectionSize = characteristics.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE)
        if (preCorrectionSize?.width() == width && preCorrectionSize.height() == height) {
            return true
        }
        return false
    }

    private fun buildDngThumbnail(source: Bitmap?): Bitmap? {
        if (source == null || source.isRecycled) {
            return null
        }

        val maxEdge = 256
        val width = source.width.coerceAtLeast(1)
        val height = source.height.coerceAtLeast(1)
        val scale = minOf(maxEdge.toFloat() / width, maxEdge.toFloat() / height, 1f)
        val targetWidth = (width * scale).toInt().coerceAtLeast(1)
        val targetHeight = (height * scale).toInt().coerceAtLeast(1)

        return if (targetWidth == width && targetHeight == height) {
            source
        } else {
            Bitmap.createScaledBitmap(source, targetWidth, targetHeight, true)
        }
    }
}
