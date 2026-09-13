package com.mega.superx.filter.camera.ui.camera

import android.graphics.SurfaceTexture
import android.util.Size
import androidx.compose.foundation.background
import com.mega.superx.filter.camera.utils.PLog
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clipToBounds
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.viewinterop.AndroidView
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import com.mega.superx.filter.camera.livephoto.LivePhotoRecorder
import com.mega.superx.filter.camera.camera.FocusPointSource
import com.mega.superx.filter.camera.camera.MeteringMode
import com.mega.superx.filter.camera.lut.LutConfig
import com.mega.superx.filter.camera.model.ColorRecipeParams
import com.mega.superx.filter.camera.raw.HncsFilmCurveMode
import com.mega.superx.filter.camera.raw.RawRenderingEngine
import com.mega.superx.filter.camera.raw.RawToneMappingParameters
import com.mega.superx.filter.camera.ui.components.FocusIndicator
import com.mega.superx.filter.camera.utils.OrientationObserver
import com.mega.superx.filter.camera.video.CaptureMode
import com.mega.superx.filter.camera.video.VideoLogProfile

@Composable
fun CameraPreviewGL(
    aspectRatio: Float,
    previewSize: Size,
    captureSize: Size,
    captureMode: CaptureMode,
    sensorOrientation: Int,
    lensFacing: Int,
    calibrationOffset: Int,
    baselineLut: LutConfig?,
    currentLut: LutConfig?,
    baselineColorRecipeParams: ColorRecipeParams,
    colorRecipeParams: ColorRecipeParams,
    focusPoint: Pair<Float, Float>?,
    focusPointSource: FocusPointSource = FocusPointSource.MANUAL,
    isFocusLocked: Boolean = false,
    isFocusing: Boolean,
    focusSuccess: Boolean?,
    meteringMode: MeteringMode = MeteringMode.SYSTEM_DEFAULT,
    onSurfaceTextureReady: (SurfaceTexture) -> Unit,
    onSurfaceDestroyed: (SurfaceTexture?) -> Unit,
    onTap: (Float, Float, Int, Int) -> Unit,
    onLongPress: (Float, Float, Int, Int) -> Unit,
    onHistogramUpdated: ((IntArray) -> Unit)? = null,
    onMeteringUpdated: ((Double, Double) -> Unit)? = null,
    onHighlightPointUpdated: ((Float, Float) -> Unit)? = null,
    onAiFocusInputAvailable: ((android.graphics.Bitmap) -> Unit)? = null,
    livePhotoRecorder: LivePhotoRecorder? = null,
    videoLogProfile: VideoLogProfile = VideoLogProfile.OFF,
    isHlgInput: Boolean = false,
    naturalLightEnabled: Boolean = false,
    rawExposureCompensation: Float = 0f,
    rawBlackPointCorrection: Float = 0f,
    rawWhitePointCorrection: Float = 0f,
    rawRenderingEngine: RawRenderingEngine = RawRenderingEngine.AdobeCurve,
    rawHncsFilmCurveMode: HncsFilmCurveMode = HncsFilmCurveMode.Standard,
    rawToneMappingParameters: RawToneMappingParameters = RawToneMappingParameters.DEFAULT,
    isAiFocusBusy: Boolean = false,
    onGLSurfaceViewReady: ((CameraGLSurfaceView) -> Unit)? = null,
    isAutoFocus: Boolean = true,
    focusPeakingEnabled: Boolean = true,
    modifier: Modifier = Modifier
) {
    val rotationDegrees = OrientationObserver.rotationDegrees
    val lifecycleOwner = LocalLifecycleOwner.current
    var glSurfaceViewRef by remember { mutableStateOf<CameraGLSurfaceView?>(null) }
    var resumeGeneration by remember { mutableIntStateOf(0) }
    val currentOnTap by rememberUpdatedState(onTap)
    val currentOnLongPress by rememberUpdatedState(onLongPress)

    DisposableEffect(lifecycleOwner) {
        val observer = LifecycleEventObserver { _, event ->
            when (event) {
                Lifecycle.Event.ON_PAUSE -> glSurfaceViewRef?.onPause()
                Lifecycle.Event.ON_RESUME -> {
                    glSurfaceViewRef?.onResume()
                    glSurfaceViewRef?.restoreRenderStateAfterResume()
                    resumeGeneration++
                }
                else -> Unit
            }
        }
        lifecycleOwner.lifecycle.addObserver(observer)
        onDispose {
            lifecycleOwner.lifecycle.removeObserver(observer)
        }
    }

    
    BoxWithConstraints(
        modifier = modifier
            .fillMaxSize()
            .background(Color.Black),
        contentAlignment = Alignment.Center
    ) {
        val containerWidth = constraints.maxWidth.toFloat()
        val containerHeight = constraints.maxHeight.toFloat()

        
        val targetRatio = aspectRatio

        
        val displayWidth: Float
        val displayHeight: Float

        if (containerWidth / containerHeight > targetRatio) {
            
            displayHeight = containerHeight
            displayWidth = displayHeight * targetRatio
        } else {
            
            displayWidth = containerWidth
            displayHeight = displayWidth / targetRatio
        }

        var viewWidth by remember { mutableIntStateOf(0) }
        var viewHeight by remember { mutableIntStateOf(0) }

        
        var surfaceTextureNotified by remember { mutableStateOf(false) }
        var notifiedPreviewSize by remember { mutableStateOf<Size?>(null) }
        var notifiedResumeGeneration by remember { mutableIntStateOf(-1) }
        var notifiedSurfaceTexture by remember { mutableStateOf<SurfaceTexture?>(null) }
        
        var surfaceAvailable by remember { mutableStateOf(false) }

        Box(
            modifier = Modifier
                .width(with(LocalDensity.current) { displayWidth.toDp() })
                .height(with(LocalDensity.current) { displayHeight.toDp() })
                .clipToBounds()
                .onSizeChanged { size ->
                    viewWidth = size.width
                    viewHeight = size.height
                }
                .pointerInput(Unit) {
                    detectTapGestures(
                        onLongPress = { offset ->
                            currentOnLongPress(offset.x, offset.y, viewWidth, viewHeight)
                        },
                        onTap = { offset ->
                            currentOnTap(offset.x, offset.y, viewWidth, viewHeight)
                        }
                    )
                }
        ) {
            
            key(previewSize.width, previewSize.height, captureMode) {
                AndroidView(
                    factory = { ctx ->
                        CameraGLSurfaceView(ctx).apply {
                            glSurfaceViewRef = this
                            
                            onGLSurfaceViewReady?.invoke(this)
                        }
                    },
                    update = { glSurfaceView ->
                        
                        glSurfaceView.onSurfaceReady = { _ ->
                            if (glSurfaceViewRef !== glSurfaceView) {
                                PLog.d("CameraPreviewGL", "Ignoring stale onSurfaceReady")
                            } else {
                                PLog.d("CameraPreviewGL", "onSurfaceReady called")
                                
                                surfaceAvailable = true
                                glSurfaceView.getSurfaceTexture()?.let { surfaceTexture ->
                                    glSurfaceView.setPreviewSize(previewSize.width, previewSize.height)
                                    PLog.d("CameraPreviewGL", "onSurfaceReady: notifiedST=$notifiedSurfaceTexture, newST=$surfaceTexture")
                                    
                                    if (notifiedSurfaceTexture != null && notifiedSurfaceTexture != surfaceTexture) {
                                        PLog.d("CameraPreviewGL", "onSurfaceReady: Forcing surfaceTextureNotified = false")
                                        surfaceTextureNotified = false
                                    }
                                    
                                }
                            }
                        }

                        glSurfaceView.onSurfaceDestroyed = {
                            val destroyedSurfaceTexture = glSurfaceView.getSurfaceTexture()
                            if (glSurfaceViewRef === glSurfaceView) {
                                surfaceAvailable = false
                                surfaceTextureNotified = false
                                notifiedPreviewSize = null
                                notifiedResumeGeneration = -1
                                notifiedSurfaceTexture = null
                                glSurfaceViewRef = null
                                onSurfaceDestroyed(destroyedSurfaceTexture)
                            } else if (destroyedSurfaceTexture != null) {
                                onSurfaceDestroyed(destroyedSurfaceTexture)
                            }
                        }

                        glSurfaceView.onHistogramUpdated = { onHistogramUpdated?.invoke(it) }
                        glSurfaceView.onMeteringUpdated = { w, l -> onMeteringUpdated?.invoke(w, l) }
                        glSurfaceView.onHighlightPointUpdated = { hx, hy -> onHighlightPointUpdated?.invoke(hx, hy) }
                        glSurfaceView.onAiFocusInputAvailable = { onAiFocusInputAvailable?.invoke(it) }

                        viewWidth = glSurfaceView.width
                        viewHeight = glSurfaceView.height
                        glSurfaceView.setSensorOrientation(sensorOrientation)
                        glSurfaceView.setLensFacing(lensFacing)
                        glSurfaceView.setDeviceRotation(rotationDegrees.toInt())
                        glSurfaceView.setCalibrationOffset(calibrationOffset)
                        glSurfaceView.setCaptureAspectRatio(aspectRatio)

                        
                        
                        
                        
                        
                        val currentSurfaceNotified = surfaceTextureNotified
                        val currentNotifiedST = notifiedSurfaceTexture
                        val currentNotifiedResumeGen = notifiedResumeGeneration
                        val currentNotifiedPreviewSize = notifiedPreviewSize
                        
                        if (viewWidth > 0 && viewHeight > 0 && surfaceAvailable) {
                            glSurfaceView.getSurfaceTexture()?.let { surfaceTexture ->
                                glSurfaceView.setPreviewSize(previewSize.width, previewSize.height)
                                glSurfaceView.setCaptureSize(captureSize.width, captureSize.height)
                                val shouldNotifySurfaceTexture =
                                    !currentSurfaceNotified ||
                                        currentNotifiedPreviewSize != previewSize ||
                                        currentNotifiedResumeGen != resumeGeneration ||
                                        currentNotifiedST != surfaceTexture
                                if (shouldNotifySurfaceTexture) {
                                    PLog.d("CameraPreviewGL", "shouldNotifySurfaceTexture is true, calling onSurfaceTextureReady")
                                    surfaceTextureNotified = true
                                    notifiedPreviewSize = previewSize
                                    notifiedResumeGeneration = resumeGeneration
                                    notifiedSurfaceTexture = surfaceTexture
                                    onSurfaceTextureReady(surfaceTexture)
                                }
                            }
                        }
                        val colorRecipeEnabled = !colorRecipeParams.isDefault()
                        val baselineColorRecipeEnabled = !baselineColorRecipeParams.isDefault()
                        
                        glSurfaceView.setBaselineLut(baselineLut)
                        glSurfaceView.setBaselineLutEnabled(baselineLut != null)
                        glSurfaceView.setLut(currentLut)
                        glSurfaceView.setLutEnabled(currentLut != null)
                        glSurfaceView.setBaselineColorRecipeEnabled(baselineColorRecipeEnabled)
                        glSurfaceView.setColorRecipeEnabled(colorRecipeEnabled)

                        glSurfaceView.setBaselineParams(baselineColorRecipeParams)
                        glSurfaceView.setParams(colorRecipeParams)

                        glSurfaceView.setFocusPoint(focusPoint?.let {
                            android.graphics.PointF(
                                it.first,
                                it.second
                            )
                        })
                        glSurfaceView.setMeteringMode(meteringMode)
                        glSurfaceView.setLivePhotoRecorder(livePhotoRecorder)
                        glSurfaceView.setVideoLogProfile(videoLogProfile)
                        glSurfaceView.setIsHlgInput(isHlgInput)
                        glSurfaceView.setRawPreviewSettings(
                            enabled = naturalLightEnabled,
                            exposureCompensation = rawExposureCompensation,
                            blackPointCorrection = rawBlackPointCorrection,
                            whitePointCorrection = rawWhitePointCorrection,
                            renderingEngine = rawRenderingEngine,
                            hncsFilmCurveMode = rawHncsFilmCurveMode,
                            toneMappingParameters = rawToneMappingParameters
                        )
                        glSurfaceView.setAutoFocus(isAutoFocus)
                        glSurfaceView.setFocusPeakingEnabled(focusPeakingEnabled)
                        glSurfaceView.setAiFocusBusy(isAiFocusBusy)
                    },
                    modifier = Modifier.fillMaxSize()
                )
            }

            
            FocusIndicator(
                position = focusPoint,
                source = focusPointSource,
                isFocusLocked = isFocusLocked,
                isFocusing = isFocusing,
                focusSuccess = focusSuccess,
                modifier = Modifier.fillMaxSize()
            )
        }
    }
}
