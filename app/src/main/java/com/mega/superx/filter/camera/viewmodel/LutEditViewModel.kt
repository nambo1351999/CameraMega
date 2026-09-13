package com.mega.superx.filter.camera.viewmodel

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.mega.superx.filter.camera.data.ContentRepository
import com.mega.superx.filter.camera.lut.BaselineColorCorrectionTarget
import com.mega.superx.filter.camera.model.ColorRecipeParams
import kotlinx.coroutines.launch

class LutEditViewModel(application: Application) : AndroidViewModel(application) {

    private val contentRepository = ContentRepository.getInstance(application)

    suspend fun getColorRecipe(
        lutId: String,
        target: BaselineColorCorrectionTarget? = null
    ) = contentRepository.lutManager.loadColorRecipeParams(lutId, target)

    
    fun saveLutColorRecipe(
        lutId: String,
        params: ColorRecipeParams,
        target: BaselineColorCorrectionTarget? = null
    ) {
        viewModelScope.launch {
            contentRepository.lutManager.saveColorRecipeParams(lutId, params, target)
        }
    }
}
