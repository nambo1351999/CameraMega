package com.mega.filter.camera.ui.flow

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.GridItemSpan
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Star
import androidx.compose.material3.CenterAlignedTopAppBar
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import com.mega.filter.camera.R
import com.mega.filter.camera.ui.flow.components.CaptureCard
import com.mega.filter.camera.ui.flow.components.FilmLutCard
import com.mega.filter.camera.ui.flow.components.HomeModeCard
import com.mega.filter.camera.ui.flow.components.SectionTitle
import kotlinx.coroutines.launch

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun HomeScreen(
    isLoading: Boolean,
    onModeSelected: (HomeMode) -> Unit,
    onQuickCapture: () -> Unit,
    onOpenGallery: () -> Unit,
    onOpenFilmLibrary: () -> Unit,
    onSettingsClick: () -> Unit,
) {
    val snackbarHostState = remember { SnackbarHostState() }
    val scope = rememberCoroutineScope()
    val premiumMessage = stringResource(R.string.premium_coming_soon)

    Scaffold(
        snackbarHost = { SnackbarHost(snackbarHostState) },
        topBar = {
            CenterAlignedTopAppBar(
                title = {
                    Text(
                        text = stringResource(R.string.app_name),
                        style = CmFlow.type.titleM.copy(fontWeight = FontWeight.Bold),
                    )
                },
                actions = {
                    IconButton(onClick = {
                        scope.launch { snackbarHostState.showSnackbar(premiumMessage) }
                    }) {
                        Icon(
                            Icons.Default.Star,
                            contentDescription = stringResource(R.string.cd_premium),
                            tint = CmFlow.colors.accent,
                        )
                    }
                    IconButton(onClick = onSettingsClick) {
                        Icon(
                            Icons.Default.Settings,
                            contentDescription = stringResource(R.string.cd_settings),
                        )
                    }
                },
                colors = TopAppBarDefaults.centerAlignedTopAppBarColors(
                    containerColor = CmFlow.colors.bgCamera,
                    titleContentColor = CmFlow.colors.textPrimary,
                    actionIconContentColor = CmFlow.colors.textPrimary,
                ),
            )
        },
        containerColor = CmFlow.colors.bgCamera,
    ) { padding ->
        LazyVerticalGrid(
            columns = GridCells.Fixed(2),
            modifier = Modifier
                .fillMaxSize()
                .padding(padding),
            contentPadding = PaddingValues(CmFlowSpace.s4),
            horizontalArrangement = Arrangement.spacedBy(CmFlowSpace.s3),
            verticalArrangement = Arrangement.spacedBy(CmFlowSpace.s3),
        ) {
            item(span = { GridItemSpan(maxLineSpan) }) {
                SectionTitle(stringResource(R.string.home_section_capture_modes))
            }

            items(HomeMode.homeCards, key = { it.name }) { mode ->
                HomeModeCard(
                    mode = mode,
                    isLoading = isLoading,
                    onClick = { onModeSelected(mode) },
                )
            }

            item(span = { GridItemSpan(maxLineSpan) }) {
                SectionTitle(
                    text = stringResource(R.string.home_section_quick),
                    modifier = Modifier.padding(top = CmFlowSpace.s2),
                )
            }

            item(span = { GridItemSpan(maxLineSpan) }) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(CmFlowSpace.s3),
                ) {
                    CaptureCard(
                        title = stringResource(R.string.home_capture_now),
                        subtitle = stringResource(R.string.home_capture_now_sub),
                        imageRes = R.drawable.home_quick_capture,
                        modifier = Modifier.weight(1f),
                        onClick = onQuickCapture,
                    )
                    CaptureCard(
                        title = stringResource(R.string.home_open_gallery),
                        subtitle = stringResource(R.string.home_open_gallery_sub),
                        imageRes = R.drawable.home_quick_gallery,
                        modifier = Modifier.weight(1f),
                        onClick = onOpenGallery,
                    )
                }
            }

            item(span = { GridItemSpan(maxLineSpan) }) {
                SectionTitle(
                    text = stringResource(R.string.home_section_creative),
                    modifier = Modifier.padding(top = CmFlowSpace.s2),
                )
            }

            item(span = { GridItemSpan(maxLineSpan) }) {
                FilmLutCard(onClick = onOpenFilmLibrary)
            }
        }
    }
}
