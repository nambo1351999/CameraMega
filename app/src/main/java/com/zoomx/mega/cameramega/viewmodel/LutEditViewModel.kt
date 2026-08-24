package com.zoomx.mega.cameramega.viewmodel

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.zoomx.mega.cameramega.data.ContentRepository
import com.zoomx.mega.cameramega.lut.BaselineColorCorrectionTarget
import com.zoomx.mega.cameramega.model.ColorRecipeParams
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
