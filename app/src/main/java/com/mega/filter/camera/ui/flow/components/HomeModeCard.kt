package com.mega.filter.camera.ui.flow.components

import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import com.mega.filter.camera.R
import com.mega.filter.camera.ui.flow.CmFlow
import com.mega.filter.camera.ui.flow.CmFlowScrim
import com.mega.filter.camera.ui.flow.CmFlowShape
import com.mega.filter.camera.ui.flow.CmFlowSpace
import com.mega.filter.camera.ui.flow.HomeMode
import java.util.Locale

@Composable
fun HomeModeCard(
    mode: HomeMode,
    isLoading: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val colors = CmFlow.colors
    val interactionSource = remember { MutableInteractionSource() }
    val isPressed by interactionSource.collectIsPressedAsState()
    val pressScale by animateFloatAsState(
        targetValue = if (isPressed) 0.97f else 1f,
        animationSpec = tween(120),
        label = "cardPressScale",
    )

    val transition = rememberInfiniteTransition(label = "shimmer")
    val shimmerAlpha by transition.animateFloat(
        initialValue = 0.25f,
        targetValue = 0.5f,
        animationSpec = infiniteRepeatable(tween(900), RepeatMode.Reverse),
        label = "shimmerAlpha",
    )

    Card(
        modifier = modifier
            .aspectRatio(1f)
            .scale(pressScale)
            .clickable(
                interactionSource = interactionSource,
                indication = null,
                enabled = !isLoading,
                onClick = onClick,
            ),
        shape = CmFlowShape.Card,
        colors = CardDefaults.cardColors(
            containerColor = Color.White.copy(alpha = 0.08f),
        ),
    ) {
        Box(modifier = Modifier.fillMaxSize()) {
            if (isLoading) {
                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .background(colors.surface.copy(alpha = shimmerAlpha)),
                )
                return@Box
            }

            Image(
                painter = painterResource(mode.imageRes),
                contentDescription = null,
                modifier = Modifier.fillMaxSize(),
                contentScale = ContentScale.Crop,
            )
            Box(modifier = Modifier.fillMaxSize().background(CmFlowScrim.Card))

            if (mode.isHardwareDependent) {
                HardwareBadge(
                    modifier = Modifier
                        .align(Alignment.TopStart)
                        .padding(CmFlowSpace.s2),
                )
            }

            Column(
                modifier = Modifier
                    .align(Alignment.BottomStart)
                    .padding(CmFlowSpace.s4),
            ) {
                Text(
                    text = stringResource(mode.titleRes),
                    style = CmFlow.type.titleL,
                    color = Color.White,
                )
                ModeBadge(
                    label = mode.badgeLabel,
                    modifier = Modifier.padding(top = CmFlowSpace.s1),
                )
            }
        }
    }
}

@Composable
private fun ModeBadge(label: String, modifier: Modifier = Modifier) {
    Box(
        modifier = modifier
            .clip(CmFlowShape.Badge)
            .background(CmFlow.colors.accent)
            .padding(horizontal = 6.dp, vertical = 3.dp),
    ) {
        Text(
            text = label.uppercase(Locale.US),
            style = CmFlow.type.labelS,
            color = CmFlow.colors.textOnAccent,
        )
    }
}

@Composable
private fun HardwareBadge(modifier: Modifier = Modifier) {
    val colors = CmFlow.colors
    val hardwareDescription = stringResource(R.string.cd_hardware_dependent)
    Row(
        modifier = modifier
            .clip(CmFlowShape.Badge)
            .background(colors.accentSecondaryContainer)
            .border(0.5.dp, colors.accentSecondary, CmFlowShape.Badge)
            .padding(horizontal = 6.dp, vertical = 3.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(
            text = stringResource(R.string.badge_hardware_dependent),
            style = CmFlow.type.labelS,
            color = colors.accentSecondaryText,
            modifier = Modifier.semantics {
                contentDescription = hardwareDescription
            },
        )
    }
}
