package com.mega.superx.filter.camera.ui.flow.components

import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import com.mega.superx.filter.camera.ui.flow.CmFlow
import com.mega.superx.filter.camera.ui.flow.CmFlowSpace

@Composable
fun SectionTitle(
    text: String,
    modifier: Modifier = Modifier,
) {
    Text(
        text = text,
        style = CmFlow.type.labelS.copy(fontWeight = FontWeight.Bold),
        color = CmFlow.colors.textSecondary,
        textAlign = TextAlign.Start,
        modifier = modifier.padding(bottom = CmFlowSpace.s1),
    )
}
