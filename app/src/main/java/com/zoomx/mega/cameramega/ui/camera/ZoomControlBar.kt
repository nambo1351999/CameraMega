package com.zoomx.mega.cameramega.ui.camera

import androidx.compose.animation.*
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.gestures.detectHorizontalDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextDecoration
import androidx.compose.ui.text.rememberTextMeasurer
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.text.drawText
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.Constraints
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.zoomx.mega.cameramega.R
import com.zoomx.mega.cameramega.camera.CameraInfo
import com.zoomx.mega.cameramega.camera.LensType
import com.zoomx.mega.cameramega.viewmodel.CameraViewModel
import kotlinx.coroutines.delay
import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.roundToInt
import com.zoomx.mega.cameramega.ui.icons.AppIcons

enum class ZoomDisplayMode {
    ZOOM_RATIO,      
    FOCAL_LENGTH;    

    fun next(): ZoomDisplayMode {
        return if (this == ZOOM_RATIO) FOCAL_LENGTH else ZOOM_RATIO
    }

    companion object {
        fun fromPersistedName(name: String?): ZoomDisplayMode {
            return entries.firstOrNull { it.name == name } ?: FOCAL_LENGTH
        }
    }
}

@Composable
fun ZoomControlBar(
    viewModel: CameraViewModel,
    zoomRatio: Float,
    availableCameras: List<CameraInfo>,
    currentCameraId: String,
    onZoomChange: (Float) -> Unit,
    onZoomStopClick: (Float) -> Unit,
    onLensSwitch: (String) -> Unit,
    onFilterClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val displayMode by viewModel.zoomDisplayMode.collectAsState()

    val currentCameraIdState by rememberUpdatedState(currentCameraId)

    val currentCamera = availableCameras.find { it.cameraId == currentCameraId }
    val isCameraReady = currentCamera != null

    
    val mainCamera =
        availableCameras.find { it.lensType == if (currentCamera?.lensType == LensType.FRONT) LensType.FRONT else LensType.BACK_MAIN }

    
    val customFocalLengths by viewModel.customFocalLengths.collectAsState(initial = emptyList())
    val hiddenFocalLengths by viewModel.hiddenFocalLengths.collectAsState(initial = emptyList())
    val lensZoomStops = viewModel.calculateLensZoomStops(availableCameras, currentCamera)
    val zoomStops = viewModel.allZoomStops(lensZoomStops, mainCamera, currentCamera, customFocalLengths, hiddenFocalLengths)

    val macroCameras = remember(availableCameras) {
        availableCameras.filter { it.lensType == LensType.BACK_MACRO }
    }

    val cameraState by viewModel.state.collectAsState()
    val minZoom = remember(cameraState.availableCameras) {
        cameraState.availableCameras.filter { it.lensType != LensType.FRONT }.minOfOrNull { it.minZoom * it.displayIntrinsicZoomRatio } ?: 1f
    }
    val maxZoom = remember(cameraState.availableCameras) {
        cameraState.availableCameras.filter { it.lensType != LensType.FRONT }.maxOfOrNull { it.maxZoom * it.displayIntrinsicZoomRatio } ?: 20f
    }

    var isContinuousZooming by remember { mutableStateOf(false) }
    var lastInteractionTime by remember { mutableLongStateOf(0L) }
    
    var internalZoomRatio by remember { mutableFloatStateOf(zoomRatio) }
    var isDragging by remember { mutableStateOf(false) }
    
    
    var customZoomStop by remember { mutableStateOf<Float?>(null) }
    var replacedStopIndex by remember { mutableIntStateOf(-1) }
    var originalStopRatio by remember { mutableFloatStateOf(0f) }

    fun closeContinuousZoomBar() {
        if (!isContinuousZooming) {
            return
        }

        isContinuousZooming = false

        val settlement = settleContinuousZoomStop(
            zoomStops = zoomStops,
            zoomRatio = internalZoomRatio
        )
        customZoomStop = settlement.customZoomStop
        replacedStopIndex = settlement.replacedStopIndex
        originalStopRatio = settlement.originalStopRatio
        settlement.snapZoomStop?.let(onZoomChange)
    }

    
    LaunchedEffect(zoomRatio, isDragging, isContinuousZooming, viewModel.isZooming) {
        if (viewModel.isZooming || (!isDragging && !isContinuousZooming && customZoomStop == null)) {
            internalZoomRatio = zoomRatio
        }
    }

    
    LaunchedEffect(viewModel.isZooming) {
        if (viewModel.isZooming) {
            isContinuousZooming = true
            customZoomStop = null
            replacedStopIndex = -1
        }
        lastInteractionTime = System.currentTimeMillis()
    }

    LaunchedEffect(zoomStops, customZoomStop, replacedStopIndex) {
        if (customZoomStop != null && replacedStopIndex !in zoomStops.indices) {
            customZoomStop = null
            replacedStopIndex = -1
        }
    }

    
    LaunchedEffect(isContinuousZooming, lastInteractionTime, isDragging, viewModel.isZooming) {
        if (isContinuousZooming && !isDragging && !viewModel.isZooming) {
            delay(2000)
            if (isContinuousZooming && !isDragging && !viewModel.isZooming) {
                closeContinuousZoomBar()
            }
        }
    }

    Box(
        modifier = modifier
            .fillMaxWidth()
            .padding(8.dp)
            .height(32.dp)
            .pointerInput(isCameraReady, minZoom, maxZoom, zoomStops) {
                if (!isCameraReady) return@pointerInput
                var dragAccumulated = 0f
                detectHorizontalDragGestures(
                    onDragStart = {
                        dragAccumulated = 0f
                        isDragging = true
                        customZoomStop = null
                        replacedStopIndex = -1
                    },
                    onHorizontalDrag = { change, dragAmount ->
                        if (!isContinuousZooming) {
                            dragAccumulated += dragAmount
                            if (abs(dragAccumulated) > 10.dp.toPx()) {
                                isContinuousZooming = true
                            }
                        }

                        if (isContinuousZooming) {
                            change.consume()
                            lastInteractionTime = System.currentTimeMillis()
                            
                            val sensitivity = 0.002f
                            val newZoom = (internalZoomRatio * exp(-dragAmount.toDouble() * sensitivity).toFloat()).coerceIn(minZoom, maxZoom)
                            internalZoomRatio = newZoom
                            
                            onZoomChange(newZoom)
                        }
                    },
                    onDragEnd = {
                        val shouldCloseContinuousZoomBar = isContinuousZooming && !viewModel.isZooming
                        isDragging = false
                        dragAccumulated = 0f
                        lastInteractionTime = System.currentTimeMillis()
                        if (shouldCloseContinuousZoomBar) {
                            closeContinuousZoomBar()
                        }
                    },
                    onDragCancel = {
                        val shouldCloseContinuousZoomBar = isContinuousZooming && !viewModel.isZooming
                        isDragging = false
                        dragAccumulated = 0f
                        lastInteractionTime = System.currentTimeMillis()
                        if (shouldCloseContinuousZoomBar) {
                            closeContinuousZoomBar()
                        }
                    }
                )
            },
        contentAlignment = Alignment.Center
    ) {
        AnimatedVisibility(
            visible = !isContinuousZooming,
            enter = fadeIn() + expandHorizontally(),
            exit = fadeOut() + shrinkHorizontally(),
            modifier = Modifier.align(Alignment.CenterStart)
        ) {
            
            IconButton(
                onClick = {
                    viewModel.saveZoomDisplayMode(displayMode.next())
                },
                modifier = Modifier.size(32.dp)
            ) {
                Icon(
                    imageVector = AppIcons.SwapHoriz,
                    contentDescription = stringResource(R.string.toggle_display_mode),
                    modifier = Modifier
                        .size(32.dp)
                        .padding(8.dp),
                    tint = Color.White
                )
            }
        }

        AnimatedVisibility(
            visible = !isContinuousZooming,
            enter = fadeIn() + expandHorizontally(),
            exit = fadeOut() + shrinkHorizontally(),
            modifier = Modifier.align(Alignment.CenterEnd)
        ) {
            
            IconButton(
                onClick = onFilterClick,
                modifier = Modifier.size(32.dp)
            ) {
                Icon(
                    imageVector = AppIcons.AutoAwesome,
                    contentDescription = stringResource(R.string.filters_panel),
                    modifier = Modifier
                        .size(32.dp)
                        .padding(8.dp),
                    tint = Color.Yellow
                )
            }
        }

        
        Box(
            modifier = Modifier
                .fillMaxHeight()
                .padding(horizontal = if (isContinuousZooming) 0.dp else 40.dp)
                .fillMaxWidth(),
            contentAlignment = Alignment.Center
        ) {
            if (!isCameraReady) {
                CircularProgressIndicator(
                    modifier = Modifier.size(20.dp),
                    color = Color(0xFFFFD700),
                    strokeWidth = 2.dp
                )
            } else if (isContinuousZooming) {
                ZoomContinuousRuler(
                    zoomRatio = internalZoomRatio,
                    minZoom = minZoom,
                    maxZoom = maxZoom,
                    mainCamera = mainCamera,
                    displayMode = displayMode,
                    modifier = Modifier.fillMaxSize()
                )
            } else {
                val effectiveStops = remember(zoomStops, customZoomStop, replacedStopIndex) {
                    buildEffectiveZoomStops(
                        zoomStops = zoomStops,
                        customZoomStop = customZoomStop,
                        replacedStopIndex = replacedStopIndex
                    )
                }
                val stopItems = remember(effectiveStops, availableCameras, currentCamera) {
                    buildZoomStopItems(
                        stops = effectiveStops,
                        availableCameras = availableCameras,
                        currentCamera = currentCamera
                    )
                }
                ZoomRuler(
                    zoomRatio = internalZoomRatio,
                    stopItems = stopItems,
                    macroCameras = macroCameras,
                    currentCameraId = currentCameraIdState,
                    mainCamera = mainCamera,
                    displayMode = displayMode,
                    onZoomChange = { stop ->
                        val targetStop = if (customZoomStop != null && stop == customZoomStop) originalStopRatio else stop
                        onZoomStopClick(targetStop)
                        customZoomStop = null
                        replacedStopIndex = -1
                    },
                    onLensSwitch = onLensSwitch,
                    modifier = Modifier.fillMaxHeight()
                )
            }
        }
    }
}

private fun buildEffectiveZoomStops(
    zoomStops: List<Float>,
    customZoomStop: Float?,
    replacedStopIndex: Int
): List<Float> {
    if (customZoomStop == null || replacedStopIndex !in zoomStops.indices) {
        return zoomStops
    }

    return zoomStops.toMutableList().apply {
        this[replacedStopIndex] = customZoomStop
    }
}

@Composable
private fun ZoomRuler(
    zoomRatio: Float,
    stopItems: List<ZoomStopItem>,
    macroCameras: List<CameraInfo>,
    currentCameraId: String,
    mainCamera: CameraInfo?,
    displayMode: ZoomDisplayMode,
    onZoomChange: (Float) -> Unit,
    onLensSwitch: (String) -> Unit,
    modifier: Modifier = Modifier
) {
    val activeColor = Color(0xFFFFD700)
    val inactiveColor = Color.White

    val stopsState by rememberUpdatedState(stopItems)

    BoxWithConstraints(
        modifier = modifier,
        contentAlignment = Alignment.Center
    ) {
        val hasMacroSeparator = stopsState.isNotEmpty() && macroCameras.isNotEmpty()
        val adaptiveMetrics = calculateZoomRulerAdaptiveMetrics(
            availableSpace = maxWidth,
            itemCount = stopsState.size + macroCameras.size,
            separatorCount = if (hasMacroSeparator) 1 else 0
        )

        Row(
            modifier = Modifier
                .width(adaptiveMetrics.rulerLength)
                .fillMaxHeight(),
            horizontalArrangement = Arrangement.spacedBy(adaptiveMetrics.spacing, Alignment.CenterHorizontally),
            verticalAlignment = Alignment.CenterVertically
        ) {
            val isCurrentMacro = macroCameras.any { it.cameraId == currentCameraId }
            val selectedStopIndex = if (isCurrentMacro) -1 else stopsState.indices.minByOrNull {
                val item = stopsState[it]
                if (item.containsCamera(currentCameraId) && abs(item.zoomRatio - zoomRatio) <= 0.01f) {
                    -1f
                } else {
                    abs(item.zoomRatio - zoomRatio)
                }
            }

            stopsState.forEachIndexed { index, item ->
                val stop = item.zoomRatio
                val isSelected = index == selectedStopIndex && abs(stop - zoomRatio) <= 0.01f

                
                val text = when (displayMode) {
                    ZoomDisplayMode.ZOOM_RATIO -> {
                        formatZoomRatioLabel(stop)
                    }

                    ZoomDisplayMode.FOCAL_LENGTH -> {
                        zoomRatioToFocalLength(stop, mainCamera)
                    }
                } + if (item.isCustomLensStop) "*" else ""

                val style = TextStyle(
                    fontSize = if (isSelected) adaptiveMetrics.selectedFontSize else adaptiveMetrics.normalFontSize,
                    fontWeight = if (isSelected) FontWeight.Bold else FontWeight.Medium,
                    color = if (isSelected) activeColor else inactiveColor,
                    textAlign = TextAlign.Center,
                    textDecoration = if (item.isLensStop) TextDecoration.Underline else TextDecoration.None,
                    shadow = ViewfinderTextShadow
                )

                Box(
                    modifier = Modifier
                        .size(adaptiveMetrics.itemSize)
                        .autoRotate()
                        .pointerInput(item.cameraIds, stop, currentCameraId) {
                            detectTapGestures {
                                val cameraId = item.targetCameraId(currentCameraId)
                                if (cameraId != null && cameraId != currentCameraId) {
                                    onLensSwitch(cameraId)
                                } else {
                                    onZoomChange(stop)
                                }
                            }
                        },
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        text = text,
                        style = style,
                        maxLines = 1,
                        softWrap = false,
                        overflow = TextOverflow.Clip
                    )
                    if (item.hasVariantBadge) {
                        val badgeText = item.selectedVariantIndex(currentCameraId)
                            ?.takeIf { isSelected }
                            ?.toString()
                            ?: item.variantCount.toString()
                        ZoomStopVariantBadge(
                            text = badgeText,
                            isSelected = isSelected,
                            activeColor = activeColor,
                            inactiveColor = inactiveColor,
                            itemSize = adaptiveMetrics.itemSize,
                            modifier = Modifier
                                .align(Alignment.TopEnd)
                                .padding(top = 1.dp, end = 1.dp)
                        )
                    }
                }
            }

            if (macroCameras.isNotEmpty()) {
                if (hasMacroSeparator) {
                    Box(
                        modifier = Modifier
                            .width(1.dp)
                            .height(adaptiveMetrics.separatorLength)
                            .background(Color.White.copy(alpha = 0.2f))
                    )
                }

                macroCameras.forEach { macroCam ->
                    val isSelected = macroCam.cameraId == currentCameraId
                    Box(
                        modifier = Modifier
                            .size(adaptiveMetrics.itemSize)
                            .autoRotate()
                            .pointerInput(macroCam.cameraId) {
                                detectTapGestures { onLensSwitch(macroCam.cameraId) }
                            },
                        contentAlignment = Alignment.Center
                    ) {
                        Icon(
                            imageVector = AppIcons.FilterVintage,
                            contentDescription = "Macro",
                            tint = if (isSelected) activeColor else inactiveColor,
                            modifier = Modifier.size(
                                if (isSelected) adaptiveMetrics.selectedIconSize else adaptiveMetrics.normalIconSize
                            )
                        )
                    }
                }
            }
        }
    }
}

@Composable
fun ZoomContinuousRuler(
    zoomRatio: Float,
    minZoom: Float,
    maxZoom: Float,
    mainCamera: CameraInfo?,
    displayMode: ZoomDisplayMode,
    modifier: Modifier = Modifier
) {
    val yellow = Color(0xFFFFD700)
    val textMeasurer = rememberTextMeasurer()

    Box(modifier = modifier) {
        Canvas(modifier = Modifier.fillMaxSize().padding(horizontal = 24.dp, vertical = 4.dp)) {
            val width = size.width
            val height = size.height
            val yellowPx = yellow
            
            
            
            val zoomStep = 1.1f
            val visibleRatioRange = 1.5f 
            
            
            
            
            val logRangePerPixel = ln(visibleRatioRange.toDouble()) / width
            
            fun zoomToX(z: Float): Float {
                return (width / 2 + (ln(z.toDouble() / zoomRatio.toDouble()) / logRangePerPixel)).toFloat()
            }
            
            fun xToZoom(x: Float): Float {
                return (zoomRatio * exp((x - width / 2) * logRangePerPixel)).toFloat()
            }

            
            
            val minVisibleZoom = xToZoom(0f)
            val maxVisibleZoom = xToZoom(width)
            
            val majorSteps = listOf(0.5f, 1f, 2f, 3f, 4f, 5f, 6f, 7f, 8f, 9f, 10f, 12f, 15f, 20f, 30f, 50f)
            majorSteps.forEach { stepValue ->
                if (stepValue in minVisibleZoom..maxVisibleZoom) {
                    val x = zoomToX(stepValue)
                    val tickHeight = 12.dp.toPx()
                    val tickWidth = 1.5.dp.toPx()
                    
                    drawRect(
                        color = Color.White.copy(alpha = 0.8f),
                        topLeft = Offset(x - tickWidth / 2f, height - tickHeight),
                        size = Size(tickWidth, tickHeight)
                    )
                    
                    val text = when (displayMode) {
                        ZoomDisplayMode.ZOOM_RATIO -> formatZoomRatioLabel(stepValue)
                        ZoomDisplayMode.FOCAL_LENGTH -> zoomRatioToFocalLength(stepValue, mainCamera)
                    }
                    
                    val textLayoutResult = textMeasurer.measure(
                        text = text,
                        style = androidx.compose.ui.text.TextStyle(
                            fontSize = 9.sp,
                            fontWeight = FontWeight.Medium,
                            color = Color.White,
                            textAlign = TextAlign.Center,
                            shadow = ViewfinderTextShadow
                        )
                    )
                    
                    drawText(
                        textLayoutResult = textLayoutResult,
                        topLeft = Offset(
                            x - textLayoutResult.size.width / 2f,
                            y = 0f
                        )
                    )
                }
            }

            
            var minor = (Math.floor(minVisibleZoom * 10.0) / 10.0).toFloat()
            while (minor <= maxVisibleZoom) {
                if (minor >= minZoom && minor <= maxZoom) {
                    val x = zoomToX(minor)
                    if (x in 0f..width) {
                        val isMajor = majorSteps.any { abs(it - minor) < 0.01f }
                        if (!isMajor) {
                            val tickHeight = 6.dp.toPx()
                            val tickWidth = 1.dp.toPx()
                            drawRect(
                                color = Color.White.copy(alpha = 0.4f),
                                topLeft = Offset(x - tickWidth / 2f, height - tickHeight),
                                size = Size(tickWidth, tickHeight)
                            )
                        }
                    }
                }
                minor += 0.1f
            }
            
            
            val centerX = width / 2f
            val indicatorWidth = 2.dp.toPx()
            
            
            drawCircle(
                color = yellowPx.copy(alpha = 0.2f),
                center = Offset(centerX, height - 6.dp.toPx()),
                radius = 8.dp.toPx()
            )
            
            drawRect(
                color = yellowPx,
                topLeft = Offset(centerX - indicatorWidth / 2f, height - 15.dp.toPx()),
                size = Size(indicatorWidth, 15.dp.toPx())
            )

            
            val currentText = when (displayMode) {
                ZoomDisplayMode.ZOOM_RATIO -> formatZoomRatioLabel(zoomRatio)
                ZoomDisplayMode.FOCAL_LENGTH -> zoomRatioToFocalLength(zoomRatio, mainCamera) + "mm"
            }
            val currentTextLayout = textMeasurer.measure(
                text = currentText,
                style = androidx.compose.ui.text.TextStyle(
                    fontSize = 14.sp,
                    fontWeight = androidx.compose.ui.text.font.FontWeight.Bold,
                    color = yellowPx,
                    shadow = ViewfinderTextShadow
                )
            )
            drawText(
                currentTextLayout,
                topLeft = Offset(centerX - currentTextLayout.size.width / 2f, -18.dp.toPx())
            )
        }
    }
}

private fun zoomRatioToFocalLength(zoomRatio: Float, mainCamera: CameraInfo?): String {
    if (mainCamera == null || mainCamera.focalLength35mmEquivalent <= 0) {
        
        return (23f * zoomRatio).toInt().toString()
    }
    return (mainCamera.focalLength35mmEquivalent * zoomRatio).roundToInt().toString()
}
