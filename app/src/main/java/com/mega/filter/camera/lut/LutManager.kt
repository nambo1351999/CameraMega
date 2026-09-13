package com.mega.filter.camera.lut

import android.content.Context
import android.util.LruCache
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.floatPreferencesKey
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import com.mega.filter.camera.data.CustomImportManager
import com.mega.filter.camera.mgc.PhotonLookContract
import com.mega.filter.camera.model.ColorRecipeParams
import com.mega.filter.camera.utils.PLog
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.firstOrNull
import kotlinx.coroutines.flow.map

private val Context.colorRecipeDataStore: DataStore<Preferences> by preferencesDataStore(name = "color_recipe_preferences")

class LutManager(private val context: Context) {

    companion object {
        private const val TAG = "LutManager"

        
        private const val CACHE_SIZE = 5

        
        private const val BUILT_IN_LUT_FOLDER = "luts"

        
        private fun recipeKey(lutId: String, target: BaselineColorCorrectionTarget? = null) =
            stringPreferencesKey(
                target?.let { "${it.name.lowercase()}_${lutId}_recipe" } ?: "${lutId}_recipe"
            )

        
        private val legacyFieldNames = listOf(
            "exposure", "contrast", "saturation", "temperature", "tint", "fade", "color",
            "highlights", "shadows", "toneToe", "toneShoulder", "tonePivot",
            "paletteX", "paletteY", "paletteDensity",
            "filmGrain", "vignette", "flash", "bleachBypass", "clarity", "bloom", "softLight", "halation", "redHalation", "chromaticAberration",
            "noise", "lowRes",
            "skinHue", "skinChroma", "skinLightness",
            "redHue", "redChroma", "redLightness",
            "orangeHue", "orangeChroma", "orangeLightness",
            "yellowHue", "yellowChroma", "yellowLightness",
            "greenHue", "greenChroma", "greenLightness",
            "cyanHue", "cyanChroma", "cyanLightness",
            "blueHue", "blueChroma", "blueLightness",
            "purpleHue", "purpleChroma", "purpleLightness",
            "magentaHue", "magentaChroma", "magentaLightness",
            "gradingShadowHue", "gradingShadowAmount", "gradingShadowLuminance",
            "gradingMidtoneHue", "gradingMidtoneAmount", "gradingMidtoneLuminance",
            "gradingHighlightHue", "gradingHighlightAmount", "gradingHighlightLuminance",
            "gradingBalance", "gradingBlending",
            "lutIntensity"
        )

        private fun readLegacyParams(preferences: Preferences, lutId: String): ColorRecipeParams {
            fun f(name: String, default: Float = 0f) =
                preferences[floatPreferencesKey("${lutId}_$name")] ?: default
            fun s(name: String) =
                preferences[stringPreferencesKey("${lutId}_$name")] ?: ""
            return ColorRecipeParams(
                exposure = f("exposure"),
                contrast = f("contrast", 1f),
                saturation = f("saturation", 1f),
                temperature = f("temperature"),
                tint = f("tint"),
                fade = f("fade"),
                color = f("color"),
                highlights = f("highlights"),
                shadows = f("shadows"),
                toneToe = f("toneToe"),
                toneShoulder = f("toneShoulder"),
                tonePivot = f("tonePivot"),
                paletteX = f("paletteX", 0.5f),
                paletteY = f("paletteY", 0.5f),
                paletteDensity = f("paletteDensity", 1f),
                filmGrain = f("filmGrain"),
                vignette = f("vignette"),
                flash = f("flash"),
                bleachBypass = f("bleachBypass"),
                clarity = f("clarity"),
                bloom = f("bloom"),
                softLight = f("softLight"),
                halation = 0f,
                redHalation = f("redHalation"),
                chromaticAberration = f("chromaticAberration"),
                noise = f("noise"),
                lowRes = f("lowRes"),
                skinHue = f("skinHue"),
                skinChroma = f("skinChroma"),
                skinLightness = f("skinLightness"),
                redHue = f("redHue"),
                redChroma = f("redChroma"),
                redLightness = f("redLightness"),
                orangeHue = f("orangeHue"),
                orangeChroma = f("orangeChroma"),
                orangeLightness = f("orangeLightness"),
                yellowHue = f("yellowHue"),
                yellowChroma = f("yellowChroma"),
                yellowLightness = f("yellowLightness"),
                greenHue = f("greenHue"),
                greenChroma = f("greenChroma"),
                greenLightness = f("greenLightness"),
                cyanHue = f("cyanHue"),
                cyanChroma = f("cyanChroma"),
                cyanLightness = f("cyanLightness"),
                blueHue = f("blueHue"),
                blueChroma = f("blueChroma"),
                blueLightness = f("blueLightness"),
                purpleHue = f("purpleHue"),
                purpleChroma = f("purpleChroma"),
                purpleLightness = f("purpleLightness"),
                magentaHue = f("magentaHue"),
                magentaChroma = f("magentaChroma"),
                magentaLightness = f("magentaLightness"),
                gradingShadowHue = f("gradingShadowHue"),
                gradingShadowAmount = f("gradingShadowAmount"),
                gradingShadowLuminance = f("gradingShadowLuminance"),
                gradingMidtoneHue = f("gradingMidtoneHue"),
                gradingMidtoneAmount = f("gradingMidtoneAmount"),
                gradingMidtoneLuminance = f("gradingMidtoneLuminance"),
                gradingHighlightHue = f("gradingHighlightHue"),
                gradingHighlightAmount = f("gradingHighlightAmount"),
                gradingHighlightLuminance = f("gradingHighlightLuminance"),
                gradingBalance = f("gradingBalance"),
                gradingBlending = f("gradingBlending", 0.5f),
                lutIntensity = f("lutIntensity", 1f),
                remarks = s("remarks"),
            )
        }

        private fun androidx.datastore.preferences.core.MutablePreferences.removeLegacyKeys(lutId: String) {
            legacyFieldNames.forEach { name -> remove(floatPreferencesKey("${lutId}_$name")) }
            remove(stringPreferencesKey("${lutId}_remarks"))
        }
    }

    
    private val lutCache = LruCache<String, LutConfig>(CACHE_SIZE)

    
    private val tendencyCache = mutableMapOf<String, FloatArray>()

    
    private var availableLuts: List<LutInfo> = emptyList()

    
    private val customImportManager = CustomImportManager(context)

    
    fun getColorRecipeParams(
        lutId: String,
        target: BaselineColorCorrectionTarget? = null
    ): Flow<ColorRecipeParams> {
        return context.colorRecipeDataStore.data.map { preferences ->
            val json = preferences[recipeKey(lutId, target)]
            if (json != null) ColorRecipeParams.fromJson(json)
            else if (target == null) readLegacyParams(preferences, lutId) else ColorRecipeParams.DEFAULT
        }
    }

    
    fun initialize() {
        val configuredBuiltInLuts = LutParser.listAvailableLuts(context, BUILT_IN_LUT_FOLDER)
        customImportManager.initializeBuiltInLutCategoriesIfNeeded(configuredBuiltInLuts)
        val builtInLuts = configuredBuiltInLuts.map { it.copy(category = "") }
        val customLuts = customImportManager.getCustomLuts()
        val categoryOverrides = customImportManager.getCategoryOverrides()
        val favoriteOverrides = customImportManager.getFavoriteOverrides()

        
        val allLuts = (customLuts + builtInLuts).distinctBy { it.id }

        
        availableLuts = allLuts.map { lut ->
            val overriddenCategory = categoryOverrides[lut.id]
            val overriddenFavorite = favoriteOverrides[lut.id]
            lut.copy(
                category = overriddenCategory ?: lut.category,
                isFavorite = overriddenFavorite ?: lut.isFavorite
            )
        }

        PLog.d(TAG, "Found ${availableLuts.size} LUT files (${customLuts.size} custom, ${builtInLuts.size} built-in)")
    }

    
    fun getAvailableLuts(): List<LutInfo> = availableLuts

    
    fun getLutInfo(id: String): LutInfo? {
        return availableLuts.find { it.id == id }
    }

    
    fun loadLut(id: String): LutConfig? {
        
        lutCache.get(id)?.let {
            
            return it
        }

        
        val lutInfo = getLutInfo(id) ?: run {

            return null
        }

        
        return try {
            val lutConfig = if (lutInfo.isBuiltIn) {
                if (lutInfo.fileName.isBlank()) {
                    return null
                }
                
                LutParser.parseFromAssets(context, lutInfo.fileName)
            } else {
                
                java.io.File(lutInfo.fileName).inputStream().use { inputStream ->
                    LutParser.parse(inputStream, lutInfo.getName())
                }
            }

            if (lutConfig.isValid()) {
                
                lutCache.put(id, lutConfig)

                lutConfig
            } else {
                PLog.e(TAG, "Invalid LUT data: $id")
                null
            }
        } catch (e: Exception) {
            PLog.w(TAG, "Failed to load LUT: $id", e)
            null
        }
    }

    
    fun evictLut(id: String) {
        lutCache.remove(id)
    }

    
    fun clearCache() {
        lutCache.evictAll()
        PLog.d(TAG, "LUT cache cleared")
    }

    
    fun getCacheInfo(): String {
        return "LUT Cache: ${lutCache.size()}/${CACHE_SIZE}, hits=${lutCache.hitCount()}, misses=${lutCache.missCount()}"
    }

    
    fun getLutTendency(id: String): FloatArray? {
        tendencyCache[id]?.let { return it }
        
        val lutConfig = loadLut(id) ?: return null
        val tendency = LutColorAnalyzer.analyzeTendency(lutConfig)
        tendencyCache[id] = tendency
        return tendency
    }

    
    fun recommendLutsForColor(targetColor: Int, limit: Int = 5): List<LutInfo> {
        return availableLuts
            .mapNotNull { info ->
                val tendency = getLutTendency(info.id) ?: return@mapNotNull null
                val score = LutColorAnalyzer.calculateSuitability(targetColor, tendency)
                info to score
            }
            .sortedByDescending { it.second }
            .take(limit)
            .map { it.first }
    }

    

    
    suspend fun saveColorRecipeParams(
        lutId: String,
        params: ColorRecipeParams,
        target: BaselineColorCorrectionTarget? = null
    ) {
        context.colorRecipeDataStore.edit { preferences ->
            preferences[recipeKey(lutId, target)] = params.toJson()
            if (target == null) {
                preferences.removeLegacyKeys(lutId)
            }
        }
        if (target == null) {
            PhotonLookContract.notifyLookChanged(context)
        }

    }

    
    suspend fun loadColorRecipeParams(
        lutId: String,
        target: BaselineColorCorrectionTarget? = null
    ): ColorRecipeParams {
        return context.colorRecipeDataStore.data.map { preferences ->
            val json = preferences[recipeKey(lutId, target)]
            if (json != null) ColorRecipeParams.fromJson(json)
            else if (target == null) readLegacyParams(preferences, lutId) else ColorRecipeParams.DEFAULT
        }.firstOrNull() ?: ColorRecipeParams.DEFAULT
    }

    
    suspend fun resetColorRecipeParams(
        lutId: String,
        target: BaselineColorCorrectionTarget? = null
    ) {
        saveColorRecipeParams(lutId, ColorRecipeParams.DEFAULT, target)
        PLog.d(TAG, "Color recipe params reset to default for LUT [$lutId]")
    }

    
    suspend fun deleteColorRecipeParams(
        lutId: String,
        target: BaselineColorCorrectionTarget? = null
    ) {
        context.colorRecipeDataStore.edit { preferences ->
            preferences.remove(recipeKey(lutId, target))
            if (target == null) {
                preferences.removeLegacyKeys(lutId)
            }
        }
        PLog.d(TAG, "Color recipe params deleted for LUT [$lutId]")
    }
}
