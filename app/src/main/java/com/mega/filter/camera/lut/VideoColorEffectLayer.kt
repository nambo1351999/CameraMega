package com.mega.filter.camera.lut

import com.mega.filter.camera.model.ColorRecipeParams

data class VideoColorEffectLayer(
    val lutConfig: LutConfig?,
    val recipeParams: ColorRecipeParams?,
)
