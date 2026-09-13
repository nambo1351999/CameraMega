package com.mega.superx.filter.camera.data

import android.content.Context
import com.mega.superx.filter.camera.frame.FrameInfo
import com.mega.superx.filter.camera.frame.FrameManager
import com.mega.superx.filter.camera.frame.FrameRenderer
import com.mega.superx.filter.camera.gallery.GalleryRepository
import com.mega.superx.filter.camera.gallery.PhotoProcessor
import com.mega.superx.filter.camera.lut.LutImageProcessor
import com.mega.superx.filter.camera.lut.LutInfo
import com.mega.superx.filter.camera.lut.LutManager
import com.mega.superx.filter.camera.processor.DepthBokehProcessor
import com.mega.superx.filter.camera.raw.DcpInfo
import com.mega.superx.filter.camera.raw.DcpManager
import com.mega.superx.filter.camera.raw.RawNoiseProfileInfo
import com.mega.superx.filter.camera.raw.RawNoiseProfileManager
import com.mega.superx.filter.camera.utils.StartupTrace
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

class ContentRepository private constructor(context: Context) {

    companion object {
        @Volatile
        private var INSTANCE: ContentRepository? = null

        fun getInstance(context: Context): ContentRepository {
            return INSTANCE ?: synchronized(this) {
                INSTANCE ?: ContentRepository(context.applicationContext).also {
                    INSTANCE = it
                }
            }
        }
    }

    private val appContext = context.applicationContext
    private fun <T> startupInit(name: String, block: () -> T): T =
        StartupTrace.measure("ContentRepository.$name") { block() }

    val lutManager = startupInit("LutManager()") { LutManager(appContext) }
    val dcpManager = startupInit("DcpManager()") { DcpManager(appContext) }
    val rawNoiseProfileManager = startupInit("RawNoiseProfileManager()") {
        RawNoiseProfileManager(appContext)
    }
    val frameManager = startupInit("FrameManager()") { FrameManager(appContext) }
    private val customImportManager = startupInit("CustomImportManager()") { CustomImportManager(appContext) }

    val imageProcessor = startupInit("LutImageProcessor()") { LutImageProcessor(appContext) }

    
    private val _availableLuts = MutableStateFlow<List<LutInfo>>(emptyList())
    val availableLuts: StateFlow<List<LutInfo>> = _availableLuts.asStateFlow()

    private val _availableFrames = MutableStateFlow<List<FrameInfo>>(emptyList())
    val availableFrames: StateFlow<List<FrameInfo>> = _availableFrames.asStateFlow()

    private val _availableDcps = MutableStateFlow<List<DcpInfo>>(emptyList())
    val availableDcps: StateFlow<List<DcpInfo>> = _availableDcps.asStateFlow()

    private val _availableRawNoiseProfiles = MutableStateFlow<List<RawNoiseProfileInfo>>(emptyList())
    val availableRawNoiseProfiles: StateFlow<List<RawNoiseProfileInfo>> =
        _availableRawNoiseProfiles.asStateFlow()

    
    val frameRenderer = startupInit("FrameRenderer()") { FrameRenderer(appContext, lutManager) }

    val depthBokehProcessor = startupInit("DepthBokehProcessor()") { DepthBokehProcessor(appContext) }

    
    val userPreferencesRepository = startupInit("UserPreferencesRepository()") {
        UserPreferencesRepository(appContext)
    }

    val photoProcessor = startupInit("PhotoProcessor()") {
        PhotoProcessor(
            lutManager,
            imageProcessor,
            frameManager,
            frameRenderer,
            depthBokehProcessor,
            userPreferencesRepository
        )
    }

    val galleryRepository = startupInit("GalleryRepository()") { GalleryRepository(appContext) }

    
    fun initialize() {
        StartupTrace.measure("ContentRepository.lutManager.initialize") {
            lutManager.initialize()
        }
        StartupTrace.measure("ContentRepository.frameManager.initialize") {
            frameManager.initialize()
        }
        _availableLuts.value = StartupTrace.measure("ContentRepository.getAvailableLuts") {
            lutManager.getAvailableLuts()
        }
        _availableFrames.value = StartupTrace.measure("ContentRepository.getAvailableFrames") {
            frameManager.getAvailableFrames()
        }
        _availableDcps.value = StartupTrace.measure("ContentRepository.getAvailableDcps") {
            dcpManager.getAvailableDcps()
        }
        _availableRawNoiseProfiles.value = StartupTrace.measure(
            "ContentRepository.getAvailableRawNoiseProfiles",
        ) {
            rawNoiseProfileManager.getAvailableProfiles()
        }
        StartupTrace.mark(
            "ContentRepository.initialize populated",
            "luts=${_availableLuts.value.size}, frames=${_availableFrames.value.size}, dcps=${_availableDcps.value.size}"
        )
    }

    
    fun getAvailableLuts(): List<LutInfo> = _availableLuts.value

    
    fun getAvailableFrames(): List<FrameInfo> = _availableFrames.value

    fun getAvailableDcps(): List<DcpInfo> = _availableDcps.value

    fun getAvailableRawNoiseProfiles(): List<RawNoiseProfileInfo> =
        _availableRawNoiseProfiles.value

    
    fun getCustomImportManager(): CustomImportManager = customImportManager

    
    fun refreshCustomContent() {
        lutManager.initialize()
        frameManager.initialize()
        _availableLuts.value = lutManager.getAvailableLuts()
        _availableFrames.value = frameManager.getAvailableFrames()
        _availableDcps.value = dcpManager.getAvailableDcps()
        _availableRawNoiseProfiles.value = rawNoiseProfileManager.getAvailableProfiles()
    }
}
