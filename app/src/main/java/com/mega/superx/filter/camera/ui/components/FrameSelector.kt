package com.mega.superx.filter.camera.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.mega.superx.filter.camera.R
import com.mega.superx.filter.camera.frame.FrameInfo
import com.mega.superx.filter.camera.ui.theme.AccentOrange

@Composable
fun FrameSelector(
    availableFrames: List<FrameInfo>,
    currentFrameId: String?,
    onFrameSelected: (String?) -> Unit,
    modifier: Modifier = Modifier
) {
    val listState = rememberLazyListState()

    LaunchedEffect(currentFrameId, availableFrames) {
        val selectedIndex = currentFrameId
            ?.let { frameId -> availableFrames.indexOfFirst { it.id == frameId } }
            ?.takeIf { it >= 0 }
            ?.plus(1)
            ?: 0
        listState.scrollToItem((selectedIndex - 1).coerceAtLeast(0))
    }

    LazyRow(
        modifier = modifier
            .fillMaxWidth(),
        state = listState,
        horizontalArrangement = Arrangement.spacedBy(12.dp),
        contentPadding = PaddingValues(horizontal = 8.dp)
    ) {
        
        item(key = "frame:none") {
            FrameItem(
                name = stringResource(R.string.none),
                isSelected = currentFrameId == null,
                onClick = { onFrameSelected(null) }
            )
        }
        
        
        items(availableFrames, key = { it.id }) { frame ->
            FrameItem(
                name = frame.name,
                isSelected = currentFrameId == frame.id,
                isCustom = !frame.isBuiltIn,
                onClick = { onFrameSelected(frame.id) }
            )
        }
    }
}

@Composable
private fun FrameItem(
    name: String,
    isSelected: Boolean,
    onClick: () -> Unit,
    isCustom: Boolean = false,
    modifier: Modifier = Modifier
) {
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        modifier = modifier
            .width(64.dp)
            .clickable { onClick() }
    ) {
        Box(
            modifier = Modifier
                .size(56.dp)
                .clip(RoundedCornerShape(8.dp))
                .background(
                    if (isSelected) AccentOrange.copy(alpha = 0.3f)
                    else Color.White.copy(alpha = 0.1f)
                )
                .then(
                    if (isSelected) Modifier.border(2.dp, AccentOrange, RoundedCornerShape(8.dp))
                    else Modifier
                ),
            contentAlignment = Alignment.Center
        ) {
            Text(
                text = name.take(2).uppercase(),
                color = if (isSelected) AccentOrange else Color.White.copy(alpha = 0.7f),
                fontSize = 16.sp
            )

            if (isCustom) {
                Box(
                    modifier = Modifier
                        .align(Alignment.TopStart)
                        .background(
                            color = Color(0xFF4CAF50),
                            shape = RoundedCornerShape(bottomEnd = 4.dp)
                        )
                        .padding(horizontal = 4.dp, vertical = 2.dp)
                ) {
                    Text(
                        text = stringResource(R.string.custom_tag),
                        color = Color.White,
                        fontSize = 8.sp,
                        fontWeight = FontWeight.Bold,
                        lineHeight = 8.sp
                    )
                }
            }
        }
        
        Spacer(modifier = Modifier.height(4.dp))
        
        Text(
            text = name,
            color = if (isSelected) AccentOrange else Color.White.copy(alpha = 0.7f),
            fontSize = 11.sp,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
            textAlign = TextAlign.Center
        )
    }
}

@Composable
fun FrameControlPanel(
    availableFrames: List<FrameInfo>,
    currentFrameId: String?,
    showAppBranding: Boolean,
    onFrameSelected: (String?) -> Unit,
    onBrandingToggle: (Boolean) -> Unit,
    modifier: Modifier = Modifier
) {
    Column(modifier = modifier) {
        
        FrameSelector(
            availableFrames = availableFrames,
            currentFrameId = currentFrameId,
            onFrameSelected = onFrameSelected
        )
        
        
        if (currentFrameId != null) {
            Spacer(modifier = Modifier.height(12.dp))
            
            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text(
                    text = stringResource(R.string.show_app_branding),
                    color = Color.White,
                    fontSize = 14.sp
                )
                
                Switch(
                    checked = showAppBranding,
                    onCheckedChange = onBrandingToggle,
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentOrange,
                        checkedTrackColor = AccentOrange.copy(alpha = 0.5f),
                        uncheckedThumbColor = Color.Gray,
                        uncheckedTrackColor = Color.Gray.copy(alpha = 0.3f)
                    )
                )
            }
        }
    }
}
