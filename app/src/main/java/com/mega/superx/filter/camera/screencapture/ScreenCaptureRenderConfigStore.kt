package com.mega.superx.filter.camera.screencapture

import android.content.Context
import com.mega.superx.filter.camera.lut.BaselineColorCorrectionTarget
import com.mega.superx.filter.camera.lut.ColorCorrectionPipelineResolver
import com.mega.superx.filter.camera.data.ContentRepository
import com.mega.superx.filter.camera.lut.LutConfig
import com.mega.superx.filter.camera.model.ColorRecipeParams
import com.mega.superx.filter.camera.model.EffectParams
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.firstOrNull

object ScreenCaptureRenderConfigStore {
    data class RenderConfig(
        val baselineLutConfig: LutConfig?,
        val baselineColorRecipeParams: ColorRecipeParams,
        val creativeLutConfig: LutConfig?,
        val creativeColorRecipeParams: ColorRecipeParams,
        val crop: PhantomPipCrop
    )

    private val _config = MutableStateFlow(
        RenderConfig(
            baselineLutConfig = null,
            baselineColorRecipeParams = ColorRecipeParams.DEFAULT,
            creativeLutConfig = null,
            creativeColorRecipeParams = ColorRecipeParams.DEFAULT,
            crop = PhantomPipCrop()
        )
    )
    val config: StateFlow<RenderConfig> = _config.asStateFlow()

    fun save(
        baselineLutConfig: LutConfig?,
        baselineColorRecipeParams: ColorRecipeParams,
        creativeLutConfig: LutConfig?,
        creativeColorRecipeParams: ColorRecipeParams,
        crop: PhantomPipCrop
    ) {
        _config.value = RenderConfig(
            baselineLutConfig = baselineLutConfig,
            baselineColorRecipeParams = baselineColorRecipeParams,
            creativeLutConfig = creativeLutConfig,
            creativeColorRecipeParams = creativeColorRecipeParams,
            crop = crop.normalized()
        )
    }

    suspend fun syncFromPreferences(
        context: Context,
        lutIdOverride: String? = null,
        cropOverride: PhantomPipCrop? = null,
        creativeRecipeParamsOverride: ColorRecipeParams? = null,
        effectParamsOverride: EffectParams? = null
    ) {
        val repository = ContentRepository.getInstance(context.applicationContext)
        val preferences = repository.userPreferencesRepository.userPreferences.firstOrNull()
        val effectivePreferences = preferences?.let {
            if (lutIdOverride == null) {
                it
            } else {
                it.copy(lutId = lutIdOverride)
            }
        }
        val colorCorrection = effectivePreferences?.let {
            ColorCorrectionPipelineResolver(repository.lutManager).resolveFromPreferences(
                target = BaselineColorCorrectionTarget.PHANTOM,
                preferences = it
            )
        }
        val baselineLayer = colorCorrection?.baselineLayer
        val creativeLayer = colorCorrection?.creativeLayer
        val creativeRecipeParams = creativeRecipeParamsOverride
            ?: creativeLayer?.colorRecipeParams
            ?: ColorRecipeParams.DEFAULT
        val effectParams = effectParamsOverride
            ?: effectivePreferences?.activeEffectParams
            ?: EffectParams.DEFAULT

        save(
            baselineLutConfig = baselineLayer?.lutConfig,
            baselineColorRecipeParams = baselineLayer?.colorRecipeParams ?: ColorRecipeParams.DEFAULT,
            creativeLutConfig = creativeLayer?.lutConfig,
            creativeColorRecipeParams = effectParams.applyTo(creativeRecipeParams),
            crop = cropOverride ?: preferences?.phantomPipCrop ?: _config.value.crop
        )
    }
}
