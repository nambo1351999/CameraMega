package com.mega.superx.filter.camera.ui.flow

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import com.mega.superx.filter.camera.R
import kotlinx.coroutines.delay

@Composable
fun SplashScreen(
    isAppReady: Boolean,
    isFirstLaunch: Boolean,
    languagePromptComplete: Boolean,
    onNavigate: (isFirstLaunch: Boolean, languagePromptComplete: Boolean) -> Unit,
) {
    LaunchedEffect(isAppReady) {
        if (!isAppReady) return@LaunchedEffect
        delay(MIN_SPLASH_MS)
        onNavigate(isFirstLaunch, languagePromptComplete)
    }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(CmFlow.colors.bgCamera),
        contentAlignment = Alignment.Center,
    ) {
        Image(
            painter = painterResource(R.drawable.splash_logo),
            contentDescription = null,
            modifier = Modifier.size(120.dp),
        )
    }
}

private const val MIN_SPLASH_MS = 400L
