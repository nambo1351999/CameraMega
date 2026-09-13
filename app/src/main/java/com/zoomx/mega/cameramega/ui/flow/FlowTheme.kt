package com.zoomx.mega.cameramega.ui.flow

import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.zoomx.mega.cameramega.ui.theme.AccentOrange
import com.zoomx.mega.cameramega.ui.theme.DarkBackground
import com.zoomx.mega.cameramega.ui.theme.DarkSurface
import com.zoomx.mega.cameramega.ui.theme.DarkSurfaceVariant
import com.zoomx.mega.cameramega.ui.theme.TextPrimary
import com.zoomx.mega.cameramega.ui.theme.TextSecondary

@Immutable
data class CmFlowColors(
    val bgCamera: Color = DarkBackground,
    val bgScreen: Color = Color(0xFF0C0C12),
    val surface: Color = DarkSurface,
    val textPrimary: Color = TextPrimary,
    val textSecondary: Color = TextSecondary,
    val textOnAccent: Color = Color.White,
    val border: Color = DarkSurfaceVariant,
    val accent: Color = AccentOrange,
    val accentSecondary: Color = Color(0xFF7E6BF0),
    val accentSecondaryContainer: Color = Color(0x1F7E6BF0),
    val accentSecondaryText: Color = Color(0xFFC4BAFF),
)

@Immutable
data class CmFlowTypography(
    val displayL: TextStyle = TextStyle(fontSize = 28.sp, fontWeight = FontWeight.Bold),
    val titleL: TextStyle = TextStyle(fontSize = 18.sp, fontWeight = FontWeight.SemiBold),
    val titleM: TextStyle = TextStyle(fontSize = 16.sp, fontWeight = FontWeight.SemiBold),
    val titleS: TextStyle = TextStyle(fontSize = 14.sp, fontWeight = FontWeight.Medium),
    val bodyL: TextStyle = TextStyle(fontSize = 16.sp, fontWeight = FontWeight.Normal),
    val bodyS: TextStyle = TextStyle(fontSize = 13.sp, fontWeight = FontWeight.Normal),
    val labelL: TextStyle = TextStyle(fontSize = 15.sp, fontWeight = FontWeight.Medium),
    val labelS: TextStyle = TextStyle(fontSize = 11.sp, fontWeight = FontWeight.Medium),
)

object CmFlow {
    val colors: CmFlowColors
        @Composable get() = LocalCmFlowColors.current

    val type: CmFlowTypography
        @Composable get() = LocalCmFlowTypography.current
}

val LocalCmFlowColors = staticCompositionLocalOf { CmFlowColors() }
val LocalCmFlowTypography = staticCompositionLocalOf { CmFlowTypography() }

object CmFlowSpace {
    val s1 = 4.dp
    val s2 = 8.dp
    val s3 = 12.dp
    val s4 = 16.dp
    val s6 = 24.dp
    val s7 = 28.dp
    val s10 = 40.dp
}

object CmFlowShape {
    val Card = RoundedCornerShape(16.dp)
    val Button = RoundedCornerShape(12.dp)
    val Badge = RoundedCornerShape(6.dp)
    val SettingsCard = RoundedCornerShape(14.dp)
}

object CmFlowScrim {
    val Card = Brush.verticalGradient(
        0f to Color.Transparent,
        0.55f to Color.Black.copy(alpha = 0.15f),
        1f to Color.Black.copy(alpha = 0.70f),
    )
}
