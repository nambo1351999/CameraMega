package com.mega.superx.filter.camera.frame

import android.content.Context
import android.net.Uri
import android.util.Log
import android.util.LruCache
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import com.mega.superx.filter.camera.data.CustomImportManager
import com.mega.superx.filter.camera.utils.PLog
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.firstOrNull
import kotlinx.coroutines.flow.map
import org.json.JSONObject
import java.io.File
import java.util.UUID

private val Context.framePropertiesDataStore: DataStore<Preferences> by preferencesDataStore(name = "frame_properties_preferences")

class FrameManager(private val context: Context) {
    
    companion object {
        private const val TAG = "FrameManager"
        private const val CACHE_SIZE = 5

        
        private fun customPropertiesKey(frameId: String) = stringPreferencesKey("${frameId}_customProperties")
    }

    private val customImportManager = CustomImportManager(context)
    
    
    private val templateCache = LruCache<String, FrameTemplate>(CACHE_SIZE)
    
    
    private var availableFrames: List<FrameInfo> = emptyList()
    
    
    fun initialize() {
        val builtInFrames = FrameTemplateParser.listAvailableFrames(context)
        val customFrames = customImportManager.getCustomFrames()
        availableFrames = (builtInFrames + customFrames).distinctBy { it.id }
        PLog.d(TAG, "Found ${availableFrames.size} frame templates (${builtInFrames.size} built-in, ${customFrames.size} custom)")
    }
    
    
    fun getAvailableFrames(): List<FrameInfo> = availableFrames
    
    
    fun getFrameInfo(id: String): FrameInfo? {
        return availableFrames.find { it.id == id }
    }

    fun createEditorDraft(frameId: String?, imageFrame: Boolean = false): FrameEditorDraft {
        if (frameId == null) {
            return FrameEditorDraft.createNew(imageFrame = imageFrame)
        }

        val template = loadTemplate(frameId)
        val frameInfo = getFrameInfo(frameId)
        return if (template != null) {
            FrameEditorDraft.fromTemplate(template, frameInfo)
        } else {
            FrameEditorDraft.createNew(imageFrame = imageFrame)
        }
    }
    
    
    fun loadTemplate(id: String): FrameTemplate? {
        
        templateCache.get(id)?.let {

            return it
        }
        
        
        val frameInfo = getFrameInfo(id) ?: run {
            PLog.e(TAG, "Frame not found: $id")
            return null
        }
        
        
        return try {
            val template = if (frameInfo.isBuiltIn) {
                FrameTemplateParser.parseFromAssets(context, frameInfo.path)
            } else {
                
                val filePath = frameInfo.path
                FrameTemplateParser.parseFromFile(filePath)
            }
            
            if (template != null) {
                templateCache.put(id, template)
                PLog.d(TAG, "Frame template loaded: $id (builtIn=${frameInfo.isBuiltIn})")
            }
            template
        } catch (e: Exception) {
            PLog.e(TAG, "Failed to load frame template: $id", e)
            null
        }
    }
    
    
    fun preloadTemplate(id: String) {
        if (templateCache.get(id) != null) {
            return
        }
        
        Thread {
            loadTemplate(id)
        }.start()
    }
    
    
    fun evictTemplate(id: String) {
        templateCache.remove(id)
    }
    
    
    fun clearCache() {
        templateCache.evictAll()
        PLog.d(TAG, "Frame template cache cleared")
    }
    
    
    fun getCacheInfo(): String {
        return "Frame Cache: ${templateCache.size()}/$CACHE_SIZE, hits=${templateCache.hitCount()}, misses=${templateCache.missCount()}"
    }

    fun importEditorFrameImage(uri: Uri, frameIdHint: String? = null): String? {
        return customImportManager.importEditorFrameImage(uri, frameIdHint)
    }

    fun saveEditorDraft(draft: FrameEditorDraft): String? {
        val overwriteFrameId = draft.editableFrameId?.takeIf { !draft.isBuiltInSource }
        val templateId = overwriteFrameId ?: draft.sourceFrameId ?: "custom_${UUID.randomUUID()}"
        val template = draft.toTemplate(templateId)
        val validationErrors = FrameTemplateParser.validateTemplate(template)
        if (validationErrors.isNotEmpty()) {
            PLog.e(TAG, "Frame draft validation failed: $validationErrors")
            return null
        }

        val savedId = customImportManager.saveFrameTemplate(template, overwriteFrameId)
        if (savedId != null) {
            draft.sourceFrameId?.let { evictTemplate(it) }
            evictTemplate(savedId)
            initialize()
        }
        return savedId
    }

    

    
    fun getCustomProperties(frameId: String): Flow<Map<String, String>> {
        return context.framePropertiesDataStore.data.map { preferences ->
            val jsonString = preferences[customPropertiesKey(frameId)]
            if (jsonString != null) {
                try {
                    jsonToMap(jsonString)
                } catch (e: Exception) {
                    PLog.e(TAG, "Failed to parse custom properties JSON for frame [$frameId]", e)
                    emptyMap()
                }
            } else {
                emptyMap()
            }
        }
    }

    
    suspend fun saveCustomProperties(frameId: String, properties: Map<String, String>) {
        context.framePropertiesDataStore.edit { preferences ->
            val jsonString = mapToJson(properties)
            preferences[customPropertiesKey(frameId)] = jsonString
        }
        PLog.d(TAG, "Custom properties saved for frame [$frameId]: $properties")
    }

    
    suspend fun loadCustomProperties(frameId: String): Map<String, String> {
        return context.framePropertiesDataStore.data.map { preferences ->
            val jsonString = preferences[customPropertiesKey(frameId)]
            if (jsonString != null) {
                try {
                    jsonToMap(jsonString)
                } catch (e: Exception) {
                    PLog.e(TAG, "Failed to parse custom properties JSON for frame [$frameId]", e)
                    emptyMap()
                }
            } else {
                emptyMap()
            }
        }.firstOrNull() ?: emptyMap()
    }

    
    suspend fun deleteCustomProperties(frameId: String) {
        context.framePropertiesDataStore.edit { preferences ->
            preferences.remove(customPropertiesKey(frameId))
        }
        PLog.d(TAG, "Custom properties deleted for frame [$frameId]")
    }

    

    private fun mapToJson(map: Map<String, String>): String {
        val jsonObject = JSONObject()
        map.forEach { (key, value) ->
            jsonObject.put(key, value)
        }
        return jsonObject.toString()
    }

    private fun jsonToMap(jsonString: String): Map<String, String> {
        val result = mutableMapOf<String, String>()
        val jsonObject = JSONObject(jsonString)
        jsonObject.keys().forEach { key ->
            result[key] = jsonObject.getString(key)
        }
        return result
    }
}
