package com.mega.superx.filter.camera.ui.camera

import java.util.Locale
import kotlin.math.roundToInt

internal fun formatZoomRatioLabel(ratio: Float): String {
    val roundedToTenth = (ratio * 10).roundToInt() / 10f
    return if (roundedToTenth < 10f) {
        String.format(Locale.US, "%.1fx", roundedToTenth)
    } else {
        "${ratio.roundToInt()}x"
    }
}
