package com.mega.filter.camera.model

import androidx.annotation.Keep
import com.google.gson.Gson

@Keep
data class EffectParams(
    val vignette: Float = 0f,               
    val flash: Float = 0f,                  
    val filmGrain: Float = 0f,             
    val clarity: Float = 0f,               
    val bloom: Float = 0f,                 
    val softLight: Float = 0f,             
    val hdf: Float = 0f,                   
    val halation: Float = 0f,              
    val chromaticAberration: Float = 0f,   
    val noise: Float = 0f,                 
    val lowRes: Float = 0f                 
) {
    
    fun isDefault(): Boolean {
        return vignette == 0f &&
                flash == 0f &&
                filmGrain == 0f &&
                clarity == 0f &&
                bloom == 0f &&
                softLight == 0f &&
                halation == 0f &&
                chromaticAberration == 0f &&
                noise == 0f &&
                lowRes == 0f
    }

    
    fun applyTo(recipe: ColorRecipeParams): ColorRecipeParams {
        return recipe.copy(
            vignette = vignette,
            flash = flash,
            filmGrain = filmGrain,
            clarity = clarity,
            bloom = bloom,
            softLight = softLight,
            halation = 0f,             
            redHalation = halation,     
            chromaticAberration = chromaticAberration,
            noise = noise,
            lowRes = lowRes
        )
    }

    fun toJson(): String = gson.toJson(copy(hdf = 0f))

    companion object {
        private val gson = Gson()
        val DEFAULT = EffectParams()

        fun fromJson(json: String): EffectParams {
            return try {
                gson.fromJson(json, EffectParams::class.java)?.copy(hdf = 0f) ?: DEFAULT
            } catch (e: Exception) {
                DEFAULT
            }
        }
    }
}

fun ColorRecipeParams.toEffectParams(): EffectParams {
    return EffectParams(
        vignette = vignette,
        flash = flash,
        filmGrain = filmGrain,
        clarity = clarity,
        bloom = bloom,
        softLight = softLight,
        hdf = 0f,
        halation = redHalation,
        chromaticAberration = chromaticAberration,
        noise = noise,
        lowRes = lowRes
    )
}
