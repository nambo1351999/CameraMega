package com.mega.superx.filter.camera

import android.Manifest
import android.content.Intent
import android.content.pm.ActivityInfo
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.view.KeyEvent
import android.view.WindowManager
import androidx.activity.compose.BackHandler
import androidx.activity.ComponentActivity
import androidx.activity.SystemBarStyle
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.slideInHorizontally
import androidx.compose.animation.slideOutHorizontally
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import kotlinx.coroutines.launch
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalWindowInfo
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import androidx.core.splashscreen.SplashScreen.Companion.installSplashScreen
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.lifecycle.repeatOnLifecycle
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.launch
import kotlinx.coroutines.flow.combine
import androidx.navigation.NavType
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import androidx.navigation.navArgument
import com.mega.superx.filter.camera.camera.AspectRatio
import com.mega.superx.filter.camera.data.CustomImportManager
import com.mega.superx.filter.camera.gallery.GalleryManager
import com.mega.superx.filter.camera.lut.creator.LutCreatorScreen
import com.mega.superx.filter.camera.lut.creator.LutCreatorViewModel
import com.mega.superx.filter.camera.ml.StartupMlPreloader
import com.mega.superx.filter.camera.screencapture.ScreenCaptureRenderConfigStore
import com.mega.superx.filter.camera.ui.camera.CameraScreen
import com.mega.superx.filter.camera.data.FilmData
import com.mega.superx.filter.camera.ui.camera.ColorWalkScreen
import com.mega.superx.filter.camera.ui.camera.FilmDetailScreen
import com.mega.superx.filter.camera.ui.camera.FilmLibraryScreen
import com.mega.superx.filter.camera.ui.camera.ToolboxScreen
import com.mega.superx.filter.camera.ui.camera.PuzzleScreen
import com.mega.superx.filter.camera.ui.gallery.BurstDetailScreen
import com.mega.superx.filter.camera.ui.gallery.GalleryDetailScreen
import com.mega.superx.filter.camera.ui.gallery.GalleryScreen
import com.mega.superx.filter.camera.ui.gallery.GalleryEditScreen
import com.mega.superx.filter.camera.ui.settings.FilterManagementScreen
import com.mega.superx.filter.camera.ui.settings.FrameEditorScreen
import com.mega.superx.filter.camera.ui.settings.FrameManagementScreen
import com.mega.superx.filter.camera.ui.settings.PhantomPipCropScreen
import com.mega.superx.filter.camera.ui.settings.SettingsScreen
import com.mega.superx.filter.camera.ui.settings.PresetEditorScreen
import com.mega.superx.filter.camera.ui.settings.PresetManagementScreen
import com.mega.superx.filter.camera.data.UserPreferencesRepository
import com.mega.superx.filter.camera.ui.flow.HomeMode
import com.mega.superx.filter.camera.ui.flow.HomeScreen
import com.mega.superx.filter.camera.ui.flow.LanguageOnboardingScreen
import com.mega.superx.filter.camera.ui.flow.OnboardingScreen
import com.mega.superx.filter.camera.ui.flow.SplashScreen
import com.mega.superx.filter.camera.ui.theme.CameraMegaTheme
import androidx.compose.runtime.CompositionLocalProvider
import com.mega.superx.filter.camera.ui.flow.CmFlowColors
import com.mega.superx.filter.camera.ui.flow.CmFlowTypography
import com.mega.superx.filter.camera.ui.flow.LocalCmFlowColors
import com.mega.superx.filter.camera.ui.flow.LocalCmFlowTypography
import com.mega.superx.filter.camera.update.AppUpdateManager
import com.mega.superx.filter.camera.utils.BuglyHelper
import com.mega.superx.filter.camera.utils.DeviceUtil
import com.mega.superx.filter.camera.utils.OrientationObserver
import com.mega.superx.filter.camera.utils.PLog
import com.mega.superx.filter.camera.utils.StartupTrace
import com.mega.superx.filter.camera.video.CaptureMode
import com.mega.superx.filter.camera.viewmodel.CameraViewModel
import com.mega.superx.filter.camera.viewmodel.GalleryTab
import com.mega.superx.filter.camera.viewmodel.GalleryViewModel
import java.util.Locale

object Routes {
    const val SPLASH = "splash"
    const val LANGUAGE = "language"
    const val ONBOARDING = "onboarding"
    const val HOME = "home"
    const val CAMERA = "camera"
    const val GALLERY = "gallery"
    const val PHOTO_DETAIL = "photo_detail/{tab}/{index}?photoId={photoId}"
    const val BURST_DETAIL = "burst_detail/{photoId}"
    const val PHOTO_EDIT = "photo_edit"
    const val SETTINGS = "settings"
    const val FILTER_MANAGEMENT = "filter_management?locateLutId={locateLutId}&videoLut={videoLut}"
    const val FRAME_MANAGEMENT = "frame_management"
    const val FRAME_EDITOR = "frame_editor?frameId={frameId}&imageFrame={imageFrame}"
    const val LUT_CREATOR = "lut_creator"
    const val LUT_SYNTHESIS = "lut_synthesis"
    const val TOOLBOX = "toolbox"
    const val PUZZLE = "puzzle"
    const val PRESET_EDITOR = "preset_editor?presetId={presetId}"
    const val PRESET_MANAGEMENT = "preset_management"

    fun presetEditor(presetId: String? = null): String {
        return if (presetId != null) "preset_editor?presetId=$presetId" else "preset_editor"
    }
    const val PHANTOM_PIP_CROP = "phantom_pip_crop"
    const val COLOR_WALK = "color_walk"
    const val FILM_LIBRARY = "film_library"
    const val FILM_DETAIL = "film_detail/{filmId}"

    fun filmDetail(filmId: String) = "film_detail/$filmId"

    fun photoDetail(tab: GalleryTab = GalleryTab.PHOTON, index: Int = 0, photoId: String? = null) =
        "photo_detail/$tab/$index" + (if (photoId != null) "?photoId=$photoId" else "")

    fun filterManagement(locateLutId: String? = null, videoLut: Boolean = false): String {
        val arguments = buildList {
            locateLutId?.let { add("locateLutId=$it") }
            if (videoLut) add("videoLut=true")
        }
        return "filter_management" + arguments.takeIf { it.isNotEmpty() }
            ?.joinToString(prefix = "?", separator = "&")
            .orEmpty()
    }

    fun burstDetail(photoId: String) = "burst_detail/$photoId"

    fun frameEditor(frameId: String? = null, imageFrame: Boolean = false): String {
        return buildString {
            append("frame_editor?imageFrame=$imageFrame")
            if (frameId != null) {
                append("&frameId=$frameId")
            }
        }
    }
}

private val ZIP_IMPORT_MIME_TYPES = setOf(
    "application/zip",
    "application/x-zip-compressed"
)

private val CUBE_IMPORT_MIME_TYPES = setOf(
    "application/cube",
    "application/x-cube",
    "application/vnd.adobe.cube",
    "application/vnd.iridas.cube"
)

private const val CAMERA_REVIEW_ACTION = "com.android.camera.action.REVIEW"
private const val PROVIDER_REVIEW_ACTION = "android.provider.action.REVIEW"
private const val PROVIDER_REVIEW_SECURE_ACTION = "android.provider.action.REVIEW_SECURE"
private const val PHOTOS_SECURE_REVIEW_ACTION = "com.google.android.apps.photos.action.SECURE_REVIEW"
private const val PHOTOS_MARS_REVIEW_ACTION = "com.google.android.apps.photos.mars.api.ACTION_REVIEW"
private const val PHOTOS_MARS_REVIEW_SECURE_ACTION = "com.google.android.apps.photos.mars.api.ACTION_REVIEW_SECURE"
private const val MEDIASTORE_AUTHORITY = "media"

private val EXTERNAL_GALLERY_REVIEW_ACTIONS = setOf(
    CAMERA_REVIEW_ACTION,
    PROVIDER_REVIEW_ACTION,
    PROVIDER_REVIEW_SECURE_ACTION,
    PHOTOS_SECURE_REVIEW_ACTION,
    PHOTOS_MARS_REVIEW_ACTION,
    PHOTOS_MARS_REVIEW_SECURE_ACTION
)

class MainActivity : ComponentActivity() {

    private val cameraViewModel: CameraViewModel by viewModels()
    private val galleryViewModel: GalleryViewModel by viewModels()

    private val permissions =
        arrayOf(Manifest.permission.CAMERA, Manifest.permission.RECORD_AUDIO)

    private var hasPermissions by mutableStateOf(false)
    private var pendingRoute by mutableStateOf<String?>(null)
    private var pendingLutImportUris by mutableStateOf<List<Uri>>(emptyList())
    private var externalGalleryReviewReturnToCaller by mutableStateOf(false)

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { result ->
        hasPermissions = result.values.all { it }
    }

    private fun applyPreferredWindowColorMode(useHdrScreenMode: Boolean, useP3ColorSpace: Boolean) {
        val configuration = resources.configuration
        when {
            useHdrScreenMode && Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE && !DeviceUtil.isHarmonyOS && configuration.isScreenHdr -> {
                window.colorMode = ActivityInfo.COLOR_MODE_HDR
            }
            useP3ColorSpace && configuration.isScreenWideColorGamut -> {
                window.colorMode = ActivityInfo.COLOR_MODE_WIDE_COLOR_GAMUT
            }
            else -> {
                window.colorMode = ActivityInfo.COLOR_MODE_DEFAULT
            }
        }
    }

    private fun applyWindowScreenBrightness(brightness: Float?) {
        val updatedAttributes = window.attributes.apply {
            screenBrightness = brightness?.coerceIn(0f, 1f)
                ?: WindowManager.LayoutParams.BRIGHTNESS_OVERRIDE_NONE
        }
        window.attributes = updatedAttributes
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        val splashScreen = installSplashScreen()
        super.onCreate(savedInstanceState)
        StartupTrace.mark("MainActivity.onCreate start")

        
        StartupTrace.measure("MainActivity.enableEdgeToEdge") {
            enableEdgeToEdge(
                statusBarStyle = SystemBarStyle.dark(android.graphics.Color.TRANSPARENT),
                navigationBarStyle = SystemBarStyle.dark(android.graphics.Color.TRANSPARENT)
            )
        }
        hideSystemUI()
        StartupTrace.mark("MainActivity.hideSystemUI applied")
        
        lifecycleScope.launch {
            repeatOnLifecycle(Lifecycle.State.STARTED) {
                combine(
                    cameraViewModel.useHdrScreenMode,
                    cameraViewModel.useP3ColorSpace
                ) { useHdrScreen, useP3 ->
                    useHdrScreen to useP3
                }.collect { (useHdrScreen, useP3) ->
                    applyPreferredWindowColorMode(useHdrScreen, useP3)
                    StartupTrace.mark("MainActivity.applyPreferredWindowColorMode applied: useHdrScreen=$useHdrScreen, useP3=$useP3")
                }
            }
        }

        lifecycleScope.launch {
            repeatOnLifecycle(Lifecycle.State.STARTED) {
                cameraViewModel.windowScreenBrightness.collect { brightness ->
                    applyWindowScreenBrightness(brightness)
                }
            }
        }

        OrientationObserver.observe(this)
        StartupTrace.mark("MainActivity.OrientationObserver.observe applied")
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            GalleryManager.hdrSdrRatio = display?.hdrSdrRatio ?: 0f
        }
        PLog.d("MainActivity", "hdrSdrRatio=${GalleryManager.hdrSdrRatio}")

        
        hasPermissions = permissions.all {
            ContextCompat.checkSelfPermission(this, it) == PackageManager.PERMISSION_GRANTED
        }
        StartupTrace.mark("MainActivity.permissions checked", "hasPermissions=$hasPermissions")

        handleIntent(intent)
        StartupTrace.mark("MainActivity.intent handled", "pendingRoute=$pendingRoute")

        lifecycleScope.launch {
            FilmData.init(this@MainActivity)
        }

        lifecycleScope.launch {
            try {
                StartupMlPreloader.preloadForStartup(
                    context = this@MainActivity,
                    preferences = cameraViewModel.userPreferences.value
                )
            } finally {
                StartupTrace.mark("MainActivity.mlPreloadFinished")
            }
        }

        splashScreen.setKeepOnScreenCondition {
            val cameraInitialized = cameraViewModel.isInitialized.value
            val galleryInitialized = galleryViewModel.isInitialized.value
            !(cameraInitialized && galleryInitialized)
        }

        StartupTrace.mark("MainActivity.setContent start")
        setContent {
            StartupComposeReadyEffect()
            val currentRecipeParams by cameraViewModel.currentRecipeParams.collectAsState()
            val phantomPipCrop by cameraViewModel.phantomPipCrop.collectAsState()
            ScreenCaptureRenderConfigStore.save(
                baselineLutConfig = cameraViewModel.currentBaselineLutConfig,
                baselineColorRecipeParams = cameraViewModel.currentBaselineRecipeParams.value,
                creativeLutConfig = cameraViewModel.currentLutConfig,
                creativeColorRecipeParams = currentRecipeParams,
                crop = phantomPipCrop
            )

            CameraMegaTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = Color.Black
                ) {
                    Box(modifier = Modifier.fillMaxSize()) {
                        val cameraInitialized by cameraViewModel.isInitialized.collectAsState()
                        val galleryInitialized by galleryViewModel.isInitialized.collectAsState()

                        LaunchedEffect(cameraInitialized, galleryInitialized) {
                            if (cameraInitialized && galleryInitialized) {
                                cameraViewModel.onShutterAnimationTriggered()
                            }
                        }

                        if (hasPermissions) {
                            NavigationHost(
                                cameraViewModel = cameraViewModel,
                                galleryViewModel = galleryViewModel,
                                pendingRoute = pendingRoute,
                                onRouteHandled = { pendingRoute = null },
                                pendingLutImportUris = pendingLutImportUris,
                                onLutImportHandled = { pendingLutImportUris = emptyList() },
                                externalGalleryReviewReturnToCaller = externalGalleryReviewReturnToCaller,
                                onExternalGalleryReviewBack = {
                                    externalGalleryReviewReturnToCaller = false
                                    finish()
                                }
                            )
                        } else {
                            PermissionScreen(
                                onRequestPermission = {
                                    permissionLauncher.launch(permissions)
                                }
                            )
                        }

                        AppUpdateInstallPrompt()
                    }
                }
            }
        }
        StartupTrace.mark("MainActivity.setContent end")
        window.decorView.post {
            StartupTrace.mark("MainActivity.decorView.post")
            reportFullyDrawn()
            StartupTrace.reportFullyDrawn("MainActivity.reportFullyDrawn")
        }
    }

    override fun onResume() {
        super.onResume()
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        if (keyCode == KeyEvent.KEYCODE_VOLUME_UP || keyCode == KeyEvent.KEYCODE_VOLUME_DOWN || keyCode == 767 || keyCode == 781) {
            if (cameraViewModel.handleVolumeKey(keyCode == KeyEvent.KEYCODE_VOLUME_UP)) {
                return true
            }
        }
        PLog.d("MainAct", "onKeyDown: $keyCode")
        return super.onKeyDown(keyCode, event)
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        handleIntent(intent)
    }

    private fun handleIntent(intent: Intent?) {
        intent ?: return
        val lutImportUris = getExternalLutImportUris(intent)
        if (lutImportUris.isNotEmpty()) {
            PLog.d("MainActivity", "External LUT import intent: action=${intent.action}, type=${intent.type}, count=${lutImportUris.size}")
            pendingLutImportUris = lutImportUris
            pendingRoute = Routes.filterManagement()
        } else if (intent.action == Intent.ACTION_SEND && intent.type?.startsWith("image/") == true) {
            val uri = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                intent.getParcelableExtra(Intent.EXTRA_STREAM, Uri::class.java)
            } else {
                @Suppress("DEPRECATION")
                intent.getParcelableExtra<Uri>(Intent.EXTRA_STREAM)
            }
            if (uri != null) {
                galleryViewModel.importSharedImage(uri) { photoId ->
                    pendingRoute = Routes.photoDetail(photoId = photoId)
                }
            }
        } else if (intent.action == Intent.ACTION_SEND_MULTIPLE && intent.type?.startsWith("image/") == true) {
            val uris = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                intent.getParcelableArrayListExtra(Intent.EXTRA_STREAM, Uri::class.java)
            } else {
                @Suppress("DEPRECATION")
                intent.getParcelableArrayListExtra<Uri>(Intent.EXTRA_STREAM)
            }
            if (!uris.isNullOrEmpty()) {
                galleryViewModel.importSharedImages(uris)
                pendingRoute = Routes.GALLERY
            }
        } else if (isExternalGalleryLaunchIntent(intent)) {
            handleExternalGalleryLaunchIntent(intent)
        } else {
            externalGalleryReviewReturnToCaller = false
        }
        intent.getStringExtra("route")?.let {
            pendingRoute = it
        }
        intent.getBooleanExtra("show_ghost_permissions", false).let {
            cameraViewModel.showGhostPermissions = it
        }
    }

    private fun handleExternalGalleryLaunchIntent(intent: Intent) {
        val data = intent.data
        externalGalleryReviewReturnToCaller = intent.action in EXTERNAL_GALLERY_REVIEW_ACTIONS
        pendingRoute = Routes.GALLERY
        PLog.d(
            "MainActivity",
            "External gallery launch intent: action=${intent.action}, type=${intent.type}, data=$data, returnToCaller=$externalGalleryReviewReturnToCaller; opening gallery while resolving content"
        )
        if (data == null) {
            return
        }

        galleryViewModel.openExternalGalleryContent(
            uri = data,
            onSuccess = { tab, photoId ->
                val route = Routes.photoDetail(tab = tab, photoId = photoId)
                PLog.d("MainActivity", "External gallery content resolved: tab=$tab, photoId=$photoId, route=$route")
                pendingRoute = route
            },
            onFallback = {
                PLog.w("MainActivity", "External gallery content unavailable; remaining in gallery: data=$data")
            }
        )
    }

    private fun getExternalLutImportUris(intent: Intent): List<Uri> {
        return when (intent.action) {
            Intent.ACTION_VIEW -> {
                listOfNotNull(intent.data)
                    .filter { shouldHandleExternalLutImportUri(it, intent.type) }
            }
            Intent.ACTION_SEND -> {
                listOfNotNull(getStreamUri(intent))
                    .filter { shouldHandleExternalLutImportUri(it, intent.type) }
            }
            Intent.ACTION_SEND_MULTIPLE -> {
                getStreamUris(intent)
                    .filter { shouldHandleExternalLutImportUri(it, intent.type) }
            }
            else -> emptyList()
        }
    }

    private fun isExternalGalleryLaunchIntent(intent: Intent): Boolean {
        val action = intent.action ?: return false
        return when {
            action == Intent.ACTION_PICK && intent.type?.startsWith("image/") == true -> true
            action == Intent.ACTION_VIEW && isMediaStoreImageViewIntent(intent) -> true
            action in EXTERNAL_GALLERY_REVIEW_ACTIONS -> true
            else -> false
        }
    }

    private fun isMediaStoreImageViewIntent(intent: Intent): Boolean {
        val data = intent.data ?: return false
        val normalizedMimeType = intent.type
            ?.substringBefore(';')
            ?.trim()
            ?.lowercase(Locale.US)
        return data.scheme == "content" &&
                data.authority == MEDIASTORE_AUTHORITY &&
                (normalizedMimeType == null || normalizedMimeType.startsWith("image/"))
    }

    private fun shouldHandleExternalLutImportUri(uri: Uri, mimeType: String?): Boolean {
        val fileName = CustomImportManager.resolveDisplayFileName(this, uri)
        if (CustomImportManager.isExternalLutImportFileName(fileName)) {
            return true
        }

        val normalizedMimeType = mimeType
            ?.substringBefore(';')
            ?.trim()
            ?.lowercase(Locale.US)

        return when {
            normalizedMimeType == null -> false
            normalizedMimeType in ZIP_IMPORT_MIME_TYPES -> true
            normalizedMimeType in CUBE_IMPORT_MIME_TYPES -> true
            normalizedMimeType == "text/plain" -> CustomImportManager.isCubeImportFileName(fileName)
            normalizedMimeType == "application/octet-stream" -> CustomImportManager.isExternalLutImportFileName(fileName)
            else -> false
        }
    }

    private fun getStreamUri(intent: Intent): Uri? {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            intent.getParcelableExtra(Intent.EXTRA_STREAM, Uri::class.java)
        } else {
            @Suppress("DEPRECATION")
            intent.getParcelableExtra<Uri>(Intent.EXTRA_STREAM)
        }
    }

    private fun getStreamUris(intent: Intent): List<Uri> {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            intent.getParcelableArrayListExtra(Intent.EXTRA_STREAM, Uri::class.java)?.filterNotNull().orEmpty()
        } else {
            @Suppress("DEPRECATION")
            intent.getParcelableArrayListExtra<Uri>(Intent.EXTRA_STREAM).orEmpty()
        }
    }

    private fun hideSystemUI() {
        WindowCompat.setDecorFitsSystemWindows(window, false)
        WindowInsetsControllerCompat(window, window.decorView).let { controller ->
            controller.hide(WindowInsetsCompat.Type.statusBars())
            controller.systemBarsBehavior =
                WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }
    }
}

@Composable
private fun StartupComposeReadyEffect() {
    LaunchedEffect(Unit) {
        StartupTrace.mark("MainActivity.first composition")
    }
}

@Composable
private fun AppUpdateInstallPrompt() {
    val context = LocalContext.current
    val lifecycleOwner = LocalLifecycleOwner.current
    var readyApk by remember { mutableStateOf<java.io.File?>(null) }

    LaunchedEffect(lifecycleOwner) {
        lifecycleOwner.lifecycle.repeatOnLifecycle(Lifecycle.State.STARTED) {
            AppUpdateManager.readyApk.collect { apkFile ->
                readyApk = apkFile
            }
        }
    }

    val apkFile = readyApk ?: return
    AlertDialog(
        onDismissRequest = {
            AppUpdateManager.consumeReadyApk(apkFile)
            readyApk = null
        },
        title = {
            Text(
                text = stringResource(R.string.update_ready_title),
                style = MaterialTheme.typography.titleLarge
            )
        },
        text = {
            Text(
                text = stringResource(R.string.update_ready_message),
                style = MaterialTheme.typography.bodyMedium
            )
        },
        confirmButton = {
            TextButton(
                onClick = {
                    val installStarted = AppUpdateManager.startInstall(context, apkFile)
                    if (installStarted) {
                        AppUpdateManager.consumeReadyApk(apkFile)
                        readyApk = null
                    } else {
                        android.widget.Toast.makeText(
                            context,
                            R.string.update_install_permission_hint,
                            android.widget.Toast.LENGTH_LONG
                        ).show()
                    }
                }
            ) {
                Text(stringResource(R.string.update_install_now))
            }
        },
        dismissButton = {
            TextButton(
                onClick = {
                    AppUpdateManager.consumeReadyApk(apkFile)
                    readyApk = null
                }
            ) {
                Text(stringResource(R.string.cancel))
            }
        }
    )
}

@Composable
private fun FlowTheme(content: @Composable () -> Unit) {
    CompositionLocalProvider(
        LocalCmFlowColors provides CmFlowColors(),
        LocalCmFlowTypography provides CmFlowTypography(),
        content = content,
    )
}

@Composable
fun NavigationHost(
    cameraViewModel: CameraViewModel,
    galleryViewModel: GalleryViewModel,
    pendingRoute: String? = null,
    onRouteHandled: () -> Unit = {},
    pendingLutImportUris: List<Uri> = emptyList(),
    onLutImportHandled: () -> Unit = {},
    externalGalleryReviewReturnToCaller: Boolean = false,
    onExternalGalleryReviewBack: () -> Unit = {}
) {
    val context = LocalContext.current
    val navController = rememberNavController()
    val navBackStackEntry by navController.currentBackStackEntryAsState()
    val currentRoute = navBackStackEntry?.destination?.route
    val userPreferences by cameraViewModel.userPreferences.collectAsState()
    val cameraInitialized by cameraViewModel.isInitialized.collectAsState()
    val galleryInitialized by galleryViewModel.isInitialized.collectAsState()
    val isAppReady = cameraInitialized && galleryInitialized
    val userPreferencesRepository = remember { UserPreferencesRepository(context) }
    val scope = rememberCoroutineScope()
    var isFirstLaunch by remember { mutableStateOf(userPreferences.isFirstLaunch) }
    var languagePromptComplete by remember { mutableStateOf(userPreferences.languagePromptComplete) }

    LaunchedEffect(userPreferences.isFirstLaunch, userPreferences.languagePromptComplete) {
        isFirstLaunch = userPreferences.isFirstLaunch
        languagePromptComplete = userPreferences.languagePromptComplete
    }

    fun navigateFromSplash() {
        val destination = when {
            !languagePromptComplete -> Routes.LANGUAGE
            isFirstLaunch -> Routes.ONBOARDING
            else -> Routes.HOME
        }
        navController.navigate(destination) {
            popUpTo(Routes.SPLASH) { inclusive = true }
        }
    }

    fun handleHomeMode(mode: HomeMode) {
        when (mode) {
            HomeMode.PRO_PHOTO -> {
                cameraViewModel.setUseRawMax(false)
                cameraViewModel.setUseJpgMax(false)
                cameraViewModel.setCaptureMode(CaptureMode.PHOTO)
            }
            HomeMode.RAW_MAX -> {
                cameraViewModel.setUseJpgMax(false)
                cameraViewModel.setUseRawMax(true)
                cameraViewModel.setCaptureMode(CaptureMode.PHOTO)
            }
            HomeMode.JPG_MAX -> {
                cameraViewModel.setUseRawMax(false)
                cameraViewModel.setUseJpgMax(true)
                cameraViewModel.setCaptureMode(CaptureMode.PHOTO)
            }
            HomeMode.VIDEO_PRO -> cameraViewModel.setCaptureMode(CaptureMode.VIDEO)
            HomeMode.PHANTOM -> {
                if (!cameraViewModel.phantomMode.value) {
                    cameraViewModel.togglePhantomMode()
                }
                cameraViewModel.setCaptureMode(CaptureMode.PHOTO)
            }
            HomeMode.FILM_LUT -> {
                navController.navigate(Routes.FILM_LIBRARY)
                return
            }
        }
        navController.navigate(Routes.CAMERA)
    }
    val handleGalleryBack: () -> Unit = {
        if (externalGalleryReviewReturnToCaller) {
            onExternalGalleryReviewBack()
        } else {
            navController.popBackStack()
        }
    }

    androidx.compose.runtime.LaunchedEffect(pendingRoute) {
        pendingRoute?.let {
            navController.navigate(it)
            onRouteHandled()
        }
    }
    BackHandler(
        enabled = externalGalleryReviewReturnToCaller &&
            (currentRoute == Routes.GALLERY || currentRoute == Routes.PHOTO_DETAIL)
    ) {
        onExternalGalleryReviewBack()
    }
    navController.addOnDestinationChangedListener { _, destination, _ ->
        BuglyHelper.setUserScene(context, destination.route.hashCode())
    }
    val containerSize = LocalWindowInfo.current.containerSize
    cameraViewModel.isExpanded =
        (containerSize.width * 1f / containerSize.height) > AspectRatio.RATIO_4_3.getValue(false)

    Box(modifier = Modifier.fillMaxSize()) {
        NavHost(
            navController = navController,
            startDestination = Routes.SPLASH,
            enterTransition = {
                slideInHorizontally(initialOffsetX = { it }) + fadeIn()
            },
            exitTransition = {
                slideOutHorizontally(targetOffsetX = { -it }) + fadeOut()
            },
            popEnterTransition = {
                slideInHorizontally(initialOffsetX = { -it }) + fadeIn()
            },
            popExitTransition = {
                slideOutHorizontally(targetOffsetX = { it }) + fadeOut()
            }
        ) {
            composable(Routes.SPLASH) {
                FlowTheme {
                    SplashScreen(
                        isAppReady = isAppReady,
                        isFirstLaunch = isFirstLaunch,
                        languagePromptComplete = languagePromptComplete,
                        onNavigate = { firstLaunch, languageComplete ->
                            isFirstLaunch = firstLaunch
                            languagePromptComplete = languageComplete
                            navigateFromSplash()
                        },
                    )
                }
            }

            composable(Routes.LANGUAGE) {
                FlowTheme {
                    LanguageOnboardingScreen(
                        onComplete = {
                            val destination = if (isFirstLaunch) Routes.ONBOARDING else Routes.HOME
                            navController.navigate(destination) {
                                popUpTo(Routes.LANGUAGE) { inclusive = true }
                            }
                        },
                        onLanguageApplied = {
                            userPreferencesRepository.setLanguagePromptComplete()
                            languagePromptComplete = true
                        },
                    )
                }
            }

            composable(Routes.ONBOARDING) {
                FlowTheme {
                    OnboardingScreen(
                        onComplete = { _ ->
                            scope.launch {
                                userPreferencesRepository.setFirstLaunchComplete()
                                isFirstLaunch = false
                            }
                            navController.navigate(Routes.HOME) {
                                popUpTo(Routes.ONBOARDING) { inclusive = true }
                            }
                        },
                    )
                }
            }

            composable(Routes.HOME) {
                FlowTheme {
                    HomeScreen(
                        isLoading = !isAppReady,
                        onModeSelected = ::handleHomeMode,
                        onQuickCapture = {
                            cameraViewModel.setCaptureMode(CaptureMode.PHOTO)
                            navController.navigate(Routes.CAMERA)
                        },
                        onOpenGallery = { navController.navigate(Routes.GALLERY) },
                        onOpenFilmLibrary = { navController.navigate(Routes.FILM_LIBRARY) },
                        onSettingsClick = { navController.navigate(Routes.SETTINGS) },
                    )
                }
            }

            composable(Routes.CAMERA) {
                if (cameraViewModel.isExpanded) {
                    Row {
                        CameraScreen(
                            viewModel = cameraViewModel,
                            galleryViewModel = galleryViewModel,
                            onGalleryClick = {
                                navController.navigate(Routes.GALLERY)
                                val latestPhoto = galleryViewModel.latestPhoto.value
                                
                            },
                            onSettingsClick = {
                                navController.navigate(Routes.SETTINGS)
                            },
                            onFilterManagementClick = { lutId ->
                                navController.navigate(
                                    Routes.filterManagement(
                                        locateLutId = lutId,
                                        videoLut = cameraViewModel.state.value.captureMode ==
                                            CaptureMode.VIDEO &&
                                            cameraViewModel.separateVideoLutEnabled.value
                                    )
                                )
                            },
                            onFrameManagementClick = {
                                navController.navigate(Routes.FRAME_MANAGEMENT)
                            },
                            onToolboxClick = {
                                navController.navigate(Routes.TOOLBOX)
                            },
                            onPresetEditClick = { id ->
                                navController.navigate(Routes.presetEditor(id))
                            },
                            onPresetManagementClick = {
                                navController.navigate(Routes.PRESET_MANAGEMENT)
                            },
                            modifier = Modifier.weight(1f)
                        )
                        GalleryDetailScreen(
                            viewModel = galleryViewModel,
                            isExpanded = true,
                            onEdit = {
                                navController.navigate(Routes.PHOTO_EDIT)
                            },
                            onViewBurst = { photoId ->
                                navController.navigate(Routes.burstDetail(photoId))
                            },
                            modifier = Modifier.weight(1f)
                        )
                    }
                } else {
                    CameraScreen(
                        viewModel = cameraViewModel,
                        galleryViewModel = galleryViewModel,
                        onGalleryClick = {
                            val latestPhoto = galleryViewModel.latestPhoto.value
                            if (latestPhoto != null && System.currentTimeMillis() - latestPhoto.dateAdded < 3 * 60 * 1000) {
                                galleryViewModel.setCurrentPhotoById(latestPhoto.id)
                                navController.navigate(Routes.photoDetail(photoId = latestPhoto.id))
                            } else {
                                navController.navigate(Routes.GALLERY)
                            }
                        },
                        onSettingsClick = {
                            navController.navigate(Routes.SETTINGS)
                        },
                        onFilterManagementClick = { lutId ->
                            navController.navigate(
                                Routes.filterManagement(
                                    locateLutId = lutId,
                                    videoLut = cameraViewModel.state.value.captureMode ==
                                        CaptureMode.VIDEO &&
                                        cameraViewModel.separateVideoLutEnabled.value
                                )
                            )
                        },
                        onFrameManagementClick = {
                            navController.navigate(Routes.FRAME_MANAGEMENT)
                        },
                        onToolboxClick = {
                            navController.navigate(Routes.TOOLBOX)
                        },
                        onPresetEditClick = { id ->
                            navController.navigate(Routes.presetEditor(id))
                        },
                        onPresetManagementClick = {
                            navController.navigate(Routes.PRESET_MANAGEMENT)
                        },
                    )
                }
            }

            composable(Routes.GALLERY) {
                GalleryScreen(
                    viewModel = galleryViewModel,
                    onBack = handleGalleryBack,
                    onPhotoClick = { tab, index ->
                        navController.navigate(Routes.photoDetail(tab, index))
                    },
                    onNavigateToEdit = { photoId ->
                        if (photoId != null) {
                            navController.navigate(Routes.photoDetail(galleryViewModel.selectedTab, 0, photoId))
                        }
                        navController.navigate(Routes.PHOTO_EDIT)
                    }
                )
            }

            composable(
                route = Routes.PHOTO_DETAIL,
                arguments = listOf(
                    navArgument("tab") { type = NavType.StringType },
                    navArgument("index") { type = NavType.IntType },
                    navArgument("photoId") {
                        type = NavType.StringType
                        nullable = true
                        defaultValue = null
                    }
                )
            ) { backStackEntry ->
                val index = backStackEntry.arguments?.getInt("index") ?: 0
                val tab = backStackEntry.arguments?.getString("tab") ?: GalleryTab.PHOTON.name
                val photoId = backStackEntry.arguments?.getString("photoId")
                GalleryDetailScreen(
                    viewModel = galleryViewModel,
                    initialIndex = index,
                    selectedTab = GalleryTab.valueOf(tab),
                    photoId = photoId,
                    onBack = handleGalleryBack,
                    onGoToGallery = {
                        navController.navigate(Routes.GALLERY) {
                            popUpTo(Routes.CAMERA)
                        }
                    },
                    onEdit = {
                        navController.navigate(Routes.PHOTO_EDIT)
                    },
                    onViewBurst = { id ->
                        navController.navigate(Routes.burstDetail(id))
                    }
                )
            }

            composable(
                route = Routes.BURST_DETAIL,
                arguments = listOf(navArgument("photoId") { type = NavType.StringType })
            ) { backStackEntry ->
                val photoId = backStackEntry.arguments?.getString("photoId") ?: ""
                BurstDetailScreen(
                    viewModel = galleryViewModel,
                    photoId = photoId,
                    onEdit = {
                        navController.navigate(Routes.PHOTO_EDIT)
                    },
                    onBack = {
                        navController.popBackStack()
                    }
                )
            }

            composable(Routes.PHOTO_EDIT) {
                GalleryEditScreen(
                    viewModel = galleryViewModel,
                    cameraViewModel = cameraViewModel,
                    onBack = {
                        navController.popBackStack()
                    },
                    onOpenFrameEditor = { frameId ->
                        navController.navigate(Routes.frameEditor(frameId = frameId))
                    },
                    onFilterManagementClick = { lutId ->
                        navController.navigate(Routes.filterManagement(lutId))
                    }
                )
            }

            composable(Routes.SETTINGS) {
                SettingsScreen(
                    viewModel = cameraViewModel,
                    onBack = {
                        navController.popBackStack()
                    },
                    onFilterManagementClick = {
                        navController.navigate(Routes.filterManagement())
                    },
                    onVideoFilterManagementClick = {
                        navController.navigate(Routes.filterManagement(videoLut = true))
                    },
                    onFrameManagementClick = {
                        navController.navigate(Routes.FRAME_MANAGEMENT)
                    },
                    onPhantomPipCropClick = {
                        navController.navigate(Routes.PHANTOM_PIP_CROP)
                    },
                    onPresetManagementClick = {
                        navController.navigate(Routes.PRESET_MANAGEMENT)
                    }
                )
            }

            composable(
                route = Routes.PRESET_EDITOR,
                arguments = listOf(
                    navArgument("presetId") {
                        type = NavType.StringType
                        nullable = true
                        defaultValue = null
                    }
                )
            ) { backStackEntry ->
                val presetId = backStackEntry.arguments?.getString("presetId")
                PresetEditorScreen(
                    viewModel = cameraViewModel,
                    presetId = presetId,
                    onBack = {
                        navController.popBackStack()
                    }
                )
            }

            composable(Routes.PRESET_MANAGEMENT) {
                PresetManagementScreen(
                    viewModel = cameraViewModel,
                    onBack = {
                        navController.popBackStack()
                    },
                    onPresetEditClick = { id ->
                        navController.navigate(Routes.presetEditor(id))
                    }
                )
            }

            composable(Routes.PHANTOM_PIP_CROP) {
                val crop by cameraViewModel.phantomPipCrop.collectAsState()
                PhantomPipCropScreen(
                    initialCrop = crop,
                    onBack = { navController.popBackStack() },
                    onSave = {
                        cameraViewModel.setPhantomPipCrop(it)
                        navController.popBackStack()
                    }
                )
            }

            composable(
                route = Routes.FILTER_MANAGEMENT,
                arguments = listOf(
                    navArgument("locateLutId") {
                        type = NavType.StringType
                        nullable = true
                        defaultValue = null
                    },
                    navArgument("videoLut") {
                        type = NavType.BoolType
                        defaultValue = false
                    }
                )
            ) { backStackEntry ->
                val locateLutId = backStackEntry.arguments?.getString("locateLutId")
                val videoLut = backStackEntry.arguments?.getBoolean("videoLut") ?: false
                FilterManagementScreen(
                    viewModel = cameraViewModel,
                    onBack = {
                        navController.popBackStack()
                    },
                    pendingLutImportUris = pendingLutImportUris,
                    onLutImportHandled = onLutImportHandled,
                    locateLutId = locateLutId,
                    manageVideoLut = videoLut
                )
            }

            composable(Routes.LUT_CREATOR) {
                val lutCreatorViewModel: LutCreatorViewModel = androidx.lifecycle.viewmodel.compose.viewModel()
                LutCreatorScreen(
                    viewModel = lutCreatorViewModel,
                    onNavigateBack = { navController.popBackStack() },
                    onSuccess = { lutId ->
                        cameraViewModel.refreshCustomContent()
                        navController.popBackStack()
                    }
                )
            }

            composable(Routes.LUT_SYNTHESIS) {
                com.mega.superx.filter.camera.lut.synthesis.LutSynthesisScreen(
                    onNavigateBack = {
                        cameraViewModel.refreshCustomContent()
                        navController.popBackStack()
                    }
                )
            }

            composable(Routes.TOOLBOX) {
                ToolboxScreen(
                    onBack = { navController.popBackStack() },
                    onLutCreatorClick = { navController.navigate(Routes.LUT_CREATOR) },
                    onLutSynthesisClick = { navController.navigate(Routes.LUT_SYNTHESIS) },
                    onColorWalkClick = { navController.navigate(Routes.COLOR_WALK) },
                    onFilmLibraryClick = { navController.navigate(Routes.FILM_LIBRARY) },
                    onPuzzleClick = { navController.navigate(Routes.PUZZLE) }
                )
            }

            composable(Routes.PUZZLE) {
                PuzzleScreen(
                    onBack = { navController.popBackStack() }
                )
            }

            composable(Routes.FILM_LIBRARY) {
                FilmLibraryScreen(
                    onBack = { navController.popBackStack() },
                    onFilmClick = { film ->
                        navController.navigate(Routes.filmDetail(film.id ?: ""))
                    }
                )
            }

            composable(
                route = Routes.FILM_DETAIL,
                arguments = listOf(navArgument("filmId") { type = NavType.StringType })
            ) { backStackEntry ->
                val filmId = backStackEntry.arguments?.getString("filmId") ?: ""
                val film = FilmData.getFilmById(filmId)
                if (film != null) {
                    FilmDetailScreen(
                        film = film,
                        onBack = { navController.popBackStack() }
                    )
                }
            }

            composable(Routes.COLOR_WALK) {
                ColorWalkScreen(
                    viewModel = cameraViewModel,
                    onBack = { navController.popBackStack() }
                )
            }

            composable(Routes.FRAME_MANAGEMENT) {
                FrameManagementScreen(
                    viewModel = cameraViewModel,
                    onBack = {
                        navController.popBackStack()
                    },
                    onCreateFrameClick = {
                        navController.navigate(Routes.frameEditor())
                    },
                    onEditFrameStyle = { frameId ->
                        navController.navigate(Routes.frameEditor(frameId = frameId))
                    }
                )
            }

            composable(
                route = Routes.FRAME_EDITOR,
                arguments = listOf(
                    navArgument("frameId") {
                        type = NavType.StringType
                        nullable = true
                        defaultValue = null
                    },
                    navArgument("imageFrame") {
                        type = NavType.BoolType
                        defaultValue = false
                    }
                )
            ) { backStackEntry ->
                FrameEditorScreen(
                    viewModel = cameraViewModel,
                    frameId = backStackEntry.arguments?.getString("frameId"),
                    imageFrame = backStackEntry.arguments?.getBoolean("imageFrame") ?: false,
                    onBack = { navController.popBackStack() },
                    onSaved = { savedId ->
                        cameraViewModel.setFrame(savedId)
                        navController.popBackStack()
                    }
                )
            }
        }
    }
}

@Composable
fun PermissionScreen(
    onRequestPermission: () -> Unit,
    modifier: Modifier = Modifier
) {
    Box(
        modifier = modifier
            .fillMaxSize()
            .background(Color.Black)
            .navigationBarsPadding(),
        contentAlignment = Alignment.Center
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(24.dp)
        ) {
            Text(
                text = stringResource(R.string.camera_permission_required),
                color = Color.White,
                style = MaterialTheme.typography.headlineSmall,
                textAlign = TextAlign.Center
            )

            Button(
                onClick = onRequestPermission,
                colors = ButtonDefaults.buttonColors(
                    containerColor = Color(0xFFFF6B35)
                )
            ) {
                Text(text = stringResource(R.string.grant_permission))
            }
        }
    }
}
