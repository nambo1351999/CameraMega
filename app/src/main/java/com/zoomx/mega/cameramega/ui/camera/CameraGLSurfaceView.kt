package com.zoomx.mega.cameramega.ui.camera

import android.content.Context
import android.graphics.Bitmap
import android.graphics.PointF
import android.graphics.SurfaceTexture
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.view.Surface
import com.zoomx.mega.cameramega.livephoto.LivePhotoRecorder
import com.zoomx.mega.cameramega.camera.MeteringMode
import com.zoomx.mega.cameramega.lut.LutConfig
import com.zoomx.mega.cameramega.lut.LutRenderer
import com.zoomx.mega.cameramega.lut.PreviewCaptureSource
import com.zoomx.mega.cameramega.model.ColorRecipeParams
import com.zoomx.mega.cameramega.model.ColorPaletteMapper
import com.zoomx.mega.cameramega.raw.HncsFilmCurveMode
import com.zoomx.mega.cameramega.raw.RawRenderingEngine
import com.zoomx.mega.cameramega.raw.RawToneMappingParameters
import com.zoomx.mega.cameramega.screencapture.PhantomPipCrop
import com.zoomx.mega.cameramega.utils.PLog
import com.zoomx.mega.cameramega.video.VideoLogProfile

class CameraGLSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null
) : GLSurfaceView(context, attrs) {

    companion object {
        private const val TAG = "CameraGLSurfaceView"
    }

    private val renderer: LutRenderer = LutRenderer(context.applicationContext)

    var onHistogramUpdated: ((IntArray) -> Unit)? = null
    var onMeteringUpdated: ((Double, Double) -> Unit)? = null
    var onHighlightPointUpdated: ((Float, Float) -> Unit)? = null
    var onAiFocusInputAvailable: ((Bitmap) -> Unit)? = null
        set(value) {
            field = value
            renderer.onAiFocusInputAvailable = value
        }

    var onSurfaceReady: ((Surface) -> Unit)? = null
    var onSurfaceDestroyed: (() -> Unit)? = null
    var onFirstPreviewFrame: (() -> Unit)? = null
    private var currentSurface: Surface? = null

    init {
        
        setEGLContextClientVersion(3)

        
        setRenderer(renderer)

        
        renderMode = RENDERMODE_WHEN_DIRTY

        
        renderer.onSurfaceTextureAvailable = { surfaceTexture ->
            
            currentSurface = Surface(surfaceTexture)

            
            post {
                val surface = currentSurface ?: return@post
                onSurfaceReady?.invoke(surface)
            }
        }

        
        renderer.onRequestRender = {
            requestRender()
        }

        renderer.onHistogramUpdated = { histogram ->
            onHistogramUpdated?.invoke(histogram)
        }

        renderer.onMeteringUpdated = { totalWeight, weightedSumLuminance ->
            onMeteringUpdated?.invoke(totalWeight, weightedSumLuminance)
        }

        renderer.onHighlightPointUpdated = { hx, hy ->
            onHighlightPointUpdated?.invoke(hx, hy)
        }

        
        preserveEGLContextOnPause = true

        PLog.d(TAG, "CameraGLSurfaceView initialized")
    }

    
    fun setPreviewSize(width: Int, height: Int) {
        queueEvent {
            renderer.setPreviewSize(width, height)
        }
    }

    fun setCaptureSize(width: Int, height: Int) {
        queueEvent {
            renderer.setCaptureSize(width, height)
        }
    }

    fun setSensorOrientation(orientation: Int) {
        queueEvent {
            renderer.setSensorOrientation(orientation)
        }
    }

    fun setCalibrationOffset(offset: Int) {
        queueEvent {
            renderer.setCalibrationOffset(offset)
        }
    }

    fun setDeviceRotation(degrees: Int) {
        queueEvent {
            renderer.setDeviceRotation(degrees)
        }
    }

    fun setLensFacing(facing: Int) {
        queueEvent {
            renderer.setLensFacing(facing)
        }
    }

    fun setCaptureAspectRatio(aspectRatio: Float) {
        queueEvent {
            renderer.setCaptureAspectRatio(aspectRatio)
        }
    }

    fun setFocusPoint(point: PointF?) {
        renderer.focusPoint = point
    }

    fun setMeteringMode(mode: MeteringMode) {
        renderer.meteringMode = mode
    }

    fun setMeteringEnabled(enabled: Boolean) {
        renderer.meteringEnabled = enabled
    }

    
    fun setLut(lutConfig: LutConfig?) {
        queueEvent {
            renderer.setLut(lutConfig)
            requestRender()
        }
    }

    fun setBaselineLut(lutConfig: LutConfig?) {
        queueEvent {
            renderer.setBaselineLut(lutConfig)
            requestRender()
        }
    }

    
    fun setLutEnabled(enabled: Boolean) {
        renderer.lutEnabled = enabled
        requestRender()
    }

    fun setBaselineLutEnabled(enabled: Boolean) {
        renderer.baselineLutEnabled = enabled
        requestRender()
    }

    fun setIsHlgInput(isHlg: Boolean) {
        renderer.isHlgInput = isHlg
        requestRender()
    }

    fun setRawPreviewSettings(
        enabled: Boolean,
        exposureCompensation: Float,
        blackPointCorrection: Float,
        whitePointCorrection: Float,
        renderingEngine: RawRenderingEngine,
        hncsFilmCurveMode: HncsFilmCurveMode,
        toneMappingParameters: RawToneMappingParameters
    ) {
        queueEvent {
            renderer.setRawPreviewSettings(
                enabled = enabled,
                exposureCompensation = exposureCompensation,
                blackPointCorrection = blackPointCorrection,
                whitePointCorrection = whitePointCorrection,
                renderingEngine = renderingEngine,
                hncsFilmCurveMode = hncsFilmCurveMode,
                toneMappingParameters = toneMappingParameters
            )
            requestRender()
        }
    }

    fun setAutoFocus(auto: Boolean) {
        renderer.isAutoFocus = auto
        requestRender()
    }

    fun setFocusPeakingEnabled(enabled: Boolean) {
        renderer.focusPeakingEnabled = enabled
        requestRender()
    }

    fun setAiFocusBusy(busy: Boolean) {
        renderer.isAiFocusBusy = busy
        requestRender()
    }

    
    fun getLutIntensity(): Float = renderer.lutIntensity

    
    fun isLutEnabled(): Boolean = renderer.lutEnabled

    
    fun setColorRecipeEnabled(enabled: Boolean) {
        renderer.colorRecipeEnabled = enabled
        requestRender()
    }

    fun setBaselineColorRecipeEnabled(enabled: Boolean) {
        renderer.baselineColorRecipeEnabled = enabled
        requestRender()
    }

    
    fun setParams(params: ColorRecipeParams) {
        val effectiveParams = ColorPaletteMapper.mergeIntoEffectiveParams(params)

        renderer.setRecipeParams(effectiveParams)
        requestRender()
    }

    fun setBaselineParams(params: ColorRecipeParams) {
        val effectiveParams = ColorPaletteMapper.mergeIntoEffectiveParams(params)
        renderer.updateBaselineRecipeParams(effectiveParams)
        requestRender()
    }

    
    fun requestRenderFrame() {
        requestRender()
    }

    
    fun getSurfaceTexture(): SurfaceTexture? = renderer.getSurfaceTexture()

    fun getRenderSurface(): Surface? = currentSurface

    fun setSourceCrop(crop: PhantomPipCrop) {
        queueEvent {
            renderer.setSourceCrop(crop)
            requestRender()
        }
    }

    
    fun capturePreviewFrame(callback: (Bitmap) -> Unit) {
        capturePreviewFrameInternal(
            maxLongEdge = null,
            source = PreviewCaptureSource.FinalDisplay,
            requestRenderImmediately = true
        ) { bitmap ->
            bitmap?.let(callback)
        }
    }

    fun capturePreviewFrame(maxLongEdge: Int, callback: (Bitmap) -> Unit) {
        capturePreviewFrameInternal(
            maxLongEdge = maxLongEdge,
            source = PreviewCaptureSource.FinalDisplay,
            requestRenderImmediately = true
        ) { bitmap ->
            bitmap?.let(callback)
        }
    }

    fun captureNextPreviewFrame(maxLongEdge: Int, callback: (Bitmap) -> Unit) {
        capturePreviewFrameInternal(
            maxLongEdge = maxLongEdge,
            source = PreviewCaptureSource.FinalDisplay,
            requestRenderImmediately = false
        ) { bitmap ->
            bitmap?.let(callback)
        }
    }

    fun captureOriginalPreviewFrame(callback: (Bitmap?) -> Unit) {
        capturePreviewFrameInternal(
            maxLongEdge = null,
            source = PreviewCaptureSource.Original,
            requestRenderImmediately = true,
            callback = callback
        )
    }

    private fun capturePreviewFrameInternal(
        maxLongEdge: Int?,
        source: PreviewCaptureSource,
        requestRenderImmediately: Boolean,
        callback: (Bitmap?) -> Unit
    ) {
        queueEvent {
            val onCaptured: (Bitmap) -> Unit = { bitmap ->
                
                post {
                    callback(bitmap)
                }
            }
            if (maxLongEdge != null) {
                renderer.capturePreviewFrame(maxLongEdge, source, onCaptured)
            } else {
                renderer.capturePreviewFrame(source = source, onCaptured = onCaptured)
            }
            if (requestRenderImmediately) {
                requestRender()
            }
        }
    }

    
    fun setLivePhotoRecorder(recorder: LivePhotoRecorder?) {
        renderer.livePhotoRecorder = recorder
    }

    fun setVideoLogProfile(profile: VideoLogProfile) {
        renderer.videoLogProfile = profile
        requestRender()
    }

    override fun onPause() {
        renderer.setRenderingPaused(true)
        super.onPause()
        PLog.d(TAG, "onPause")
    }

    override fun onResume() {
        renderer.setRenderingPaused(false)
        super.onResume()
        PLog.d(TAG, "onResume")
    }

    fun restoreRenderStateAfterResume() {
        queueEvent {
            renderer.restoreLutTexturesAfterResume()
            requestRender()
        }
    }

    override fun onDetachedFromWindow() {
        super.onDetachedFromWindow()
        PLog.d(TAG, "onDetachedFromWindow")

        
        onSurfaceDestroyed?.invoke()

        
        currentSurface?.release()
        currentSurface = null

        
        queueEvent {
            renderer.release()
        }
    }
}
