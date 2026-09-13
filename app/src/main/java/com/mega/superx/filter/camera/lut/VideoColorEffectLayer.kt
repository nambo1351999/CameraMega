package com.mega.superx.filter.camera.lut

import com.mega.superx.filter.camera.model.ColorRecipeParams

data class VideoColorEffectLayer(
    val lutConfig: LutConfig?,
    val recipeParams: ColorRecipeParams?,
)
