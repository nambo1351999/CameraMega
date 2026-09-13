package com.mega.superx.filter.camera.processor

import com.mega.superx.filter.camera.raw.MgcSpatialStrengthMap
import com.mega.superx.filter.camera.utils.PLog
import java.nio.ByteBuffer

internal object MgcSpatialStrengthMapGenerator {
    private const val TAG = "MgcSpatialStrength"

    
    private const val REJECTED_DENOISE_MULTIPLIER = 1f

    init {
        System.loadLibrary("my-native-lib")
    }

    
    data class Result(
        val strengthMap: MgcSpatialStrengthMap,
        val outputReadNoise: FloatArray,
        val outputShotNoise: FloatArray,
        val outputWeightsSumTotalDiag0: FloatArray,
        val outputWeightsSumTotalDiag1: FloatArray,
    )

    fun compute(
        outputMode: MgcSpatialOutputMode,
        fusedFixed16: ByteBuffer,
        width: Int,
        height: Int,
        cfaPattern: Int,
        alignment: ByteBuffer,
        alignmentWidth: Int,
        alignmentHeight: Int,
        rejection: ByteBuffer,
        rejectionWidth: Int,
        rejectionHeight: Int,
        frameCount: Int,
        inputReadNoise: FloatArray,
        inputShotNoise: FloatArray,
        frameWeights: FloatArray,
        kernelSigmas: FloatArray,
    ): Result? {
        val geometry = runCatching {
            mgcSpatialDiagnosticGeometry(outputMode, width, height)
        }.getOrNull() ?: return null
        val expectedAlignmentValues =
            alignmentWidth.toLong() * alignmentHeight.toLong() * frameCount * 2L
        val expectedRejectionValues =
            rejectionWidth.toLong() * rejectionHeight.toLong() * frameCount
        val valid = fusedFixed16.isDirect && alignment.isDirect && rejection.isDirect &&
            width > 0 && height > 0 && cfaPattern in 0..3 && frameCount > 1 &&
            fusedFixed16.capacity().toLong() >=
                geometry.fixed16SampleCount * Short.SIZE_BYTES &&
            alignmentWidth == geometry.alignmentWidth &&
            alignmentHeight == geometry.alignmentHeight &&
            rejectionWidth == geometry.rejectionWidth &&
            rejectionHeight == geometry.rejectionHeight &&
            alignment.capacity().toLong() >= expectedAlignmentValues * Float.SIZE_BYTES &&
            rejection.capacity().toLong() >= expectedRejectionValues &&
            inputReadNoise.size == frameCount * 3 &&
            inputShotNoise.size == frameCount * 3 &&
            frameWeights.size == frameCount &&
            kernelSigmas.size == frameCount
        if (!valid) {
            PLog.e(
                TAG,
                "Rejected malformed MGC strength input: mode=$outputMode " +
                    "image=${width}x$height frames=$frameCount " +
                    "alignment=${alignmentWidth}x$alignmentHeight " +
                    "rejection=${rejectionWidth}x$rejectionHeight",
            )
            return null
        }
        fusedFixed16.position(0)
        alignment.position(0)
        rejection.position(0)
        val output = ShortArray(rejectionWidth * rejectionHeight)
        val outputReadNoise = FloatArray(3)
        val outputShotNoise = FloatArray(3)
        val outputWeightsSumTotalDiag0 = FloatArray(3)
        val outputWeightsSumTotalDiag1 = FloatArray(3)
        val result = nativeCompute(
            layout = if (outputMode == MgcSpatialOutputMode.BAYER) 0 else 1,
            fusedFixed16 = fusedFixed16,
            width = width,
            height = height,
            cfaPattern = cfaPattern,
            alignment = alignment,
            alignmentWidth = alignmentWidth,
            alignmentHeight = alignmentHeight,
            rejection = rejection,
            rejectionWidth = rejectionWidth,
            rejectionHeight = rejectionHeight,
            frameCount = frameCount,
            inputReadNoise = inputReadNoise,
            inputShotNoise = inputShotNoise,
            frameWeights = frameWeights,
            kernelSigmas = kernelSigmas,
            rejectedDenoiseMultiplier = REJECTED_DENOISE_MULTIPLIER,
            outputStrengthQ8 = output,
            outputReadNoise = outputReadNoise,
            outputShotNoise = outputShotNoise,
            outputWeightsSumTotalDiag0 = outputWeightsSumTotalDiag0,
            outputWeightsSumTotalDiag1 = outputWeightsSumTotalDiag1,
        )
        fusedFixed16.position(0)
        alignment.position(0)
        rejection.position(0)
        if (result != 0) {
            PLog.e(
                TAG,
                "MGC strength AOT failed: result=$result mode=$outputMode " +
                    "image=${width}x$height frames=$frameCount",
            )
            return null
        }
        if (outputReadNoise.any { !it.isFinite() || it < 0f } ||
            outputShotNoise.any { !it.isFinite() || it < 0f } ||
            (0 until 3).any { channel ->
                outputReadNoise[channel] <= 0f && outputShotNoise[channel] <= 0f
            } ||
            outputWeightsSumTotalDiag0.any { !it.isFinite() || it < 0f } ||
            outputWeightsSumTotalDiag1.any { !it.isFinite() || it < 0f }
        ) {
            PLog.e(
                TAG,
                "MGC strength AOT returned malformed output noise: " +
                    "read=${outputReadNoise.contentToString()} " +
                    "shot=${outputShotNoise.contentToString()} " +
                    "diag0=${outputWeightsSumTotalDiag0.contentToString()} " +
                    "diag1=${outputWeightsSumTotalDiag1.contentToString()}",
            )
            return null
        }
        return Result(
            strengthMap = MgcSpatialStrengthMap(
                width = rejectionWidth,
                height = rejectionHeight,
                q8 = output,
            ),
            outputReadNoise = outputReadNoise,
            outputShotNoise = outputShotNoise,
            outputWeightsSumTotalDiag0 = outputWeightsSumTotalDiag0,
            outputWeightsSumTotalDiag1 = outputWeightsSumTotalDiag1,
        )
    }

    private external fun nativeCompute(
        layout: Int,
        fusedFixed16: ByteBuffer,
        width: Int,
        height: Int,
        cfaPattern: Int,
        alignment: ByteBuffer,
        alignmentWidth: Int,
        alignmentHeight: Int,
        rejection: ByteBuffer,
        rejectionWidth: Int,
        rejectionHeight: Int,
        frameCount: Int,
        inputReadNoise: FloatArray,
        inputShotNoise: FloatArray,
        frameWeights: FloatArray,
        kernelSigmas: FloatArray,
        rejectedDenoiseMultiplier: Float,
        outputStrengthQ8: ShortArray,
        outputReadNoise: FloatArray,
        outputShotNoise: FloatArray,
        outputWeightsSumTotalDiag0: FloatArray,
        outputWeightsSumTotalDiag1: FloatArray,
    ): Int
}
