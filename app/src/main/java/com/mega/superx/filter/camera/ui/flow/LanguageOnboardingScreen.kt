package com.mega.superx.filter.camera.ui.flow

import androidx.appcompat.app.AppCompatDelegate
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.core.os.LocaleListCompat
import com.mega.superx.filter.camera.R
import kotlinx.coroutines.launch

enum class AppLanguage(val tag: String?) {
    SYSTEM(null),
    VI("vi"),
    EN("en"),
    ZH("zh"),
}

@Composable
fun LanguageOnboardingScreen(
    onComplete: () -> Unit,
    onLanguageApplied: suspend () -> Unit,
) {
    var selected by remember { mutableStateOf(AppLanguage.SYSTEM) }
    val scope = rememberCoroutineScope()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(CmFlow.colors.bgScreen)
            .padding(CmFlowSpace.s6),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
    ) {
        Text(
            text = stringResource(R.string.language_onboarding_title),
            style = CmFlow.type.displayL,
            color = CmFlow.colors.textPrimary,
            textAlign = TextAlign.Center,
        )
        Text(
            text = stringResource(R.string.language_onboarding_subtitle),
            style = CmFlow.type.bodyL,
            color = CmFlow.colors.textSecondary,
            textAlign = TextAlign.Center,
            modifier = Modifier.padding(top = CmFlowSpace.s3, bottom = CmFlowSpace.s6),
        )

        AppLanguage.entries.forEach { language ->
            val label = when (language) {
                AppLanguage.SYSTEM -> stringResource(R.string.language_system)
                AppLanguage.VI -> stringResource(R.string.language_vi)
                AppLanguage.EN -> stringResource(R.string.language_en)
                AppLanguage.ZH -> stringResource(R.string.language_zh)
            }
            Text(
                text = label,
                style = CmFlow.type.titleM,
                color = if (selected == language) CmFlow.colors.textOnAccent else CmFlow.colors.textPrimary,
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(vertical = CmFlowSpace.s1)
                    .background(
                        if (selected == language) CmFlow.colors.accent else CmFlow.colors.surface,
                        CmFlowShape.Button,
                    )
                    .clickable { selected = language }
                    .padding(vertical = CmFlowSpace.s4),
                textAlign = TextAlign.Center,
            )
        }

        Button(
            onClick = {
                scope.launch {
                    val locales = when (val tag = selected.tag) {
                        null -> LocaleListCompat.getEmptyLocaleList()
                        else -> LocaleListCompat.forLanguageTags(tag)
                    }
                    AppCompatDelegate.setApplicationLocales(locales)
                    onLanguageApplied()
                    onComplete()
                }
            },
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = CmFlowSpace.s7),
            shape = CmFlowShape.Button,
            colors = ButtonDefaults.buttonColors(
                containerColor = CmFlow.colors.accent,
                contentColor = CmFlow.colors.textOnAccent,
            ),
        ) {
            Text(
                text = stringResource(R.string.action_continue),
                style = CmFlow.type.labelL,
                modifier = Modifier.padding(vertical = CmFlowSpace.s2),
            )
        }
    }
}
