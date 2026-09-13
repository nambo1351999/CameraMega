package com.zoomx.mega.cameramega.ui.flow.components

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.zoomx.mega.cameramega.R
import com.zoomx.mega.cameramega.ui.flow.CmFlow
import com.zoomx.mega.cameramega.ui.flow.CmFlowScrim
import com.zoomx.mega.cameramega.ui.flow.CmFlowShape
import com.zoomx.mega.cameramega.ui.flow.CmFlowSpace

@Composable
fun FilmLutCard(
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Card(
        modifier = modifier
            .fillMaxWidth()
            .height(120.dp)
            .clickable(onClick = onClick),
        shape = CmFlowShape.Card,
        colors = CardDefaults.cardColors(
            containerColor = Color.White.copy(alpha = 0.08f),
        ),
    ) {
        Box(modifier = Modifier.fillMaxSize()) {
            Image(
                painter = painterResource(R.drawable.home_card_film_lut),
                contentDescription = null,
                modifier = Modifier.fillMaxSize(),
                contentScale = ContentScale.Crop,
            )
            Box(modifier = Modifier.fillMaxSize().background(CmFlowScrim.Card))
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(CmFlowSpace.s4),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Column {
                    Text(
                        text = stringResource(R.string.home_mode_film_lut),
                        style = CmFlow.type.titleL,
                        color = Color.White,
                    )
                    Text(
                        text = stringResource(R.string.home_film_lut_sub),
                        style = CmFlow.type.bodyS,
                        color = CmFlow.colors.textSecondary,
                        modifier = Modifier.padding(top = CmFlowSpace.s1),
                    )
                }
                Text(text = "🎞️", style = CmFlow.type.displayL)
            }
        }
    }
}
