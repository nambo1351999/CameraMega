package com.mega.filter.camera.ui.flow

import androidx.annotation.DrawableRes
import androidx.annotation.StringRes
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import com.mega.filter.camera.R
import com.mega.filter.camera.ui.theme.AccentOrange

enum class HomeMode(
    @param:StringRes val titleRes: Int,
    val badgeLabel: String,
    @param:DrawableRes val imageRes: Int,
    val isHardwareDependent: Boolean = false,
) {
    PRO_PHOTO(R.string.home_mode_pro_photo, "PRO", R.drawable.home_card_pro_photo),
    RAW_MAX(R.string.home_mode_raw_max, "RAW", R.drawable.home_card_raw_max),
    JPG_MAX(R.string.home_mode_jpg_max, "MAX", R.drawable.home_card_jpg_max),
    VIDEO_PRO(R.string.home_mode_video_pro, "4K", R.drawable.home_card_video_pro),
    PHANTOM(R.string.home_mode_phantom, "PIP", R.drawable.home_card_phantom, isHardwareDependent = true),
    FILM_LUT(R.string.home_mode_film_lut, "LUT", R.drawable.home_card_film_lut),
    ;

    companion object {
        val homeCards: List<HomeMode> = entries.filter { it != FILM_LUT }
    }
}

fun HomeMode.cardBackgroundBrush(): Brush = when (this) {
    HomeMode.PRO_PHOTO -> Brush.linearGradient(
        listOf(Color(0xFF1A1A28), AccentOrange.copy(alpha = 0.35f), Color(0xFF12121B)),
    )
    HomeMode.RAW_MAX -> Brush.linearGradient(
        listOf(Color(0xFF102018), Color(0xFF1E4030), Color(0xFF0A1820)),
    )
    HomeMode.JPG_MAX -> Brush.linearGradient(
        listOf(Color(0xFF2A1810), Color(0xFF5C3820), Color(0xFF1A2838)),
    )
    HomeMode.VIDEO_PRO -> Brush.linearGradient(
        listOf(Color(0xFF201018), Color(0xFF401828)),
    )
    HomeMode.PHANTOM -> Brush.linearGradient(
        listOf(Color(0xFF1A1A2E), Color(0xFF3D2B5C), Color(0xFF16213E)),
    )
    HomeMode.FILM_LUT -> Brush.linearGradient(
        listOf(Color(0xFF281828), Color(0xFF402848), Color(0xFF182838)),
    )
}
