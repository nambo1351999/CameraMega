package com.mega.superx.filter.camera.ui.flow

import androidx.activity.compose.BackHandler
import androidx.annotation.DrawableRes
import androidx.annotation.StringRes
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.pager.HorizontalPager
import androidx.compose.foundation.pager.rememberPagerState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.util.lerp
import com.mega.superx.filter.camera.R
import kotlinx.coroutines.launch
import kotlin.math.absoluteValue

private data class OnboardingPage(
    @param:StringRes val titleRes: Int,
    @param:StringRes val descriptionRes: Int,
    @param:DrawableRes val imageRes: Int,
)

private val pages = listOf(
    OnboardingPage(R.string.onboarding_1_title, R.string.onboarding_1_desc, R.drawable.onboarding_raw_pro),
    OnboardingPage(R.string.onboarding_2_title, R.string.onboarding_2_desc, R.drawable.onboarding_jpg_max),
    OnboardingPage(R.string.onboarding_3_title, R.string.onboarding_3_desc, R.drawable.onboarding_film_lut),
)

@Composable
fun OnboardingScreen(
    onComplete: (skipped: Boolean) -> Unit,
) {
    val pagerState = rememberPagerState(pageCount = { pages.size })
    val scope = rememberCoroutineScope()
    val isLastPage = pagerState.currentPage == pages.lastIndex

    BackHandler(enabled = pagerState.currentPage > 0) {
        scope.launch { pagerState.animateScrollToPage(pagerState.currentPage - 1) }
    }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(CmFlow.colors.bgCamera),
    ) {
        if (!isLastPage) {
            TextButton(
                onClick = { onComplete(true) },
                modifier = Modifier
                    .align(Alignment.TopEnd)
                    .padding(CmFlowSpace.s4),
            ) {
                Text(
                    text = stringResource(R.string.action_skip),
                    style = CmFlow.type.labelL,
                    color = CmFlow.colors.textSecondary,
                )
            }
        }

        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(horizontal = CmFlowSpace.s6),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            HorizontalPager(
                state = pagerState,
                modifier = Modifier
                    .weight(1f)
                    .padding(top = CmFlowSpace.s10),
            ) { page ->
                val pageOffset = (
                    (pagerState.currentPage - page) + pagerState.currentPageOffsetFraction
                    ).absoluteValue.coerceIn(0f, 1f)

                Column(
                    modifier = Modifier
                        .fillMaxSize()
                        .graphicsLayer {
                            translationX = lerp(
                                start = 0f,
                                stop = size.width * 0.4f,
                                fraction = pageOffset,
                            ) * if (pagerState.currentPage > page) 1f else -1f
                        },
                    horizontalAlignment = Alignment.CenterHorizontally,
                    verticalArrangement = Arrangement.Center,
                ) {
                    OnboardingIllustration(imageRes = pages[page].imageRes)

                    Text(
                        text = stringResource(pages[page].titleRes),
                        style = CmFlow.type.displayL.copy(fontWeight = FontWeight.Bold),
                        color = CmFlow.colors.textPrimary,
                        textAlign = TextAlign.Center,
                        modifier = Modifier.padding(top = CmFlowSpace.s7),
                    )
                    Text(
                        text = stringResource(pages[page].descriptionRes),
                        style = CmFlow.type.bodyL,
                        color = CmFlow.colors.textSecondary,
                        textAlign = TextAlign.Center,
                        modifier = Modifier.padding(
                            top = CmFlowSpace.s4,
                            start = CmFlowSpace.s2,
                            end = CmFlowSpace.s2,
                        ),
                    )
                }
            }

            Row(
                horizontalArrangement = Arrangement.spacedBy(CmFlowSpace.s2),
                modifier = Modifier.padding(bottom = CmFlowSpace.s6),
            ) {
                pages.indices.forEach { index ->
                    val isActive = index == pagerState.currentPage
                    Box(
                        modifier = Modifier
                            .size(if (isActive) 10.dp else 8.dp)
                            .clip(CircleShape)
                            .background(
                                if (isActive) CmFlow.colors.accent else CmFlow.colors.border,
                            ),
                    )
                }
            }

            Button(
                onClick = {
                    scope.launch {
                        if (isLastPage) {
                            onComplete(false)
                        } else {
                            pagerState.animateScrollToPage(pagerState.currentPage + 1)
                        }
                    }
                },
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(bottom = CmFlowSpace.s6),
                colors = ButtonDefaults.buttonColors(
                    containerColor = CmFlow.colors.accent,
                    contentColor = CmFlow.colors.textOnAccent,
                ),
                shape = CmFlowShape.Button,
            ) {
                Text(
                    text = stringResource(
                        if (isLastPage) R.string.action_start else R.string.action_continue,
                    ),
                    style = CmFlow.type.labelL,
                    modifier = Modifier.padding(vertical = CmFlowSpace.s2),
                )
            }
        }
    }
}

@Composable
private fun OnboardingIllustration(@DrawableRes imageRes: Int) {
    Image(
        painter = painterResource(imageRes),
        contentDescription = null,
        modifier = Modifier
            .size(200.dp)
            .clip(CircleShape),
        contentScale = ContentScale.Crop,
    )
}
