package com.mega.superx.filter.camera.ui.flow.components

import androidx.annotation.DrawableRes
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import com.mega.superx.filter.camera.ui.flow.CmFlow
import com.mega.superx.filter.camera.ui.flow.CmFlowScrim
import com.mega.superx.filter.camera.ui.flow.CmFlowShape
import com.mega.superx.filter.camera.ui.flow.CmFlowSpace

@Composable
fun CaptureCard(
    title: String,
    subtitle: String,
    @DrawableRes imageRes: Int,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Card(
        modifier = modifier
            .height(88.dp)
            .clickable(onClick = onClick),
        shape = CmFlowShape.SettingsCard,
        colors = CardDefaults.cardColors(
            containerColor = Color.White.copy(alpha = 0.10f),
        ),
    ) {
        Box(modifier = Modifier.fillMaxSize()) {
            Image(
                painter = painterResource(imageRes),
                contentDescription = null,
                modifier = Modifier.fillMaxSize(),
                contentScale = ContentScale.Crop,
            )
            Box(modifier = Modifier.fillMaxSize().background(CmFlowScrim.Card))
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(CmFlowSpace.s3),
                verticalArrangement = Arrangement.Center,
            ) {
                Text(
                    text = title,
                    style = CmFlow.type.titleS,
                    color = CmFlow.colors.textPrimary,
                )
                Text(
                    text = subtitle,
                    style = CmFlow.type.bodyS,
                    color = CmFlow.colors.textSecondary,
                )
            }
        }
    }
}
