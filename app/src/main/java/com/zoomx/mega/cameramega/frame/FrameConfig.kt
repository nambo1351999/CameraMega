package com.zoomx.mega.cameramega.frame

import android.graphics.Color

data class FrameConfig(
    val id: String,
    val name: String,
    val templateId: String,
    val backgroundColor: Int = Color.WHITE,
    val paddingDp: Int = 40,
    val showAppBranding: Boolean = true,
    val customText: String? = null
)

data class FrameInfo(
    val id: String,
    val path: String,
    val nameMap: Map<String, String>,
    val previewResId: Int = 0,
    val isBuiltIn: Boolean = true,
    val isEditable: Boolean = false,
) {
    
    val name: String
        get() = getName()

    fun getName(locale: java.util.Locale = java.util.Locale.getDefault()): String {
        val language = if (locale.language == "zh") "zh" else "en"
        return nameMap[language] ?: nameMap["en"] ?: id
    }
}

data class FrameTemplate(
    val id: String,
    val nameMap: Map<String, String>,
    val version: Int = 1,
    val layout: FrameLayout,
    val elements: List<FrameElement>,
    val elementsTop: List<FrameElement>? = null
) {
    fun getName(locale: java.util.Locale = java.util.Locale.getDefault()): String {
        val language = if (locale.language == "zh") "zh" else "en"
        return nameMap[language] ?: nameMap["en"] ?: id
    }
}

data class FrameLayout(
    val position: FramePosition = FramePosition.BOTTOM,
    val heightDp: Int = 80,
    val backgroundColor: Int = Color.WHITE,
    val borderColor: Int = backgroundColor, 
    val lineSpacingDp: Int = 8, 
    val paddingDp: Int = 16,
    val borderWidthDp: Int = 0,  
    val photoCornerRadiusDp: Int = 0,
    val photoShadowEnabled: Boolean = false,
    val photoShadowRadiusDp: Int = 0,
    val photoShadowOffsetXDp: Int = 0,
    val photoShadowOffsetYDp: Int = 2,
    val photoShadowColor: Int = 0xCC000000.toInt(),
    val imageResName: String? = null,  
    val imagePath: String? = null  
)

enum class FramePosition {
    TOP,
    BOTTOM,
    BOTH,
    OVERLAY,  
    BORDER,   
    IMAGE     
}

sealed class FrameElement(
    open val line: Int,
    open val size: Int,
) {
    
    data class Text(
        val textType: TextType,
        val alignment: ElementAlignment = ElementAlignment.START,
        val fontSizeSp: Int = 14,
        val color: Int = Color.DKGRAY,
        val fontWeight: FontWeight = FontWeight.NORMAL,
        val fontFamily: String? = null,
        val overrideText: String? = null,
        val format: String? = null,
        val prefix: String? = null,
        val suffix: String? = null,
        override val line: Int = 0
    ) : FrameElement(line = line, size = fontSizeSp)
    
    
    data class Logo(
        val logoType: LogoType,
        val overrideSource: String? = null,
        val alignment: ElementAlignment = ElementAlignment.CENTER,
        val sizeDp: Int = 24,
        val maxWidth: Int = 0,
        val light: Boolean = false,
        val marginDp: Int = 8,
        override val line: Int = 0
    ) : FrameElement(line = line, size = sizeDp)
    
    
    data class Divider(
        val orientation: DividerOrientation = DividerOrientation.VERTICAL,
        val alignment: ElementAlignment = ElementAlignment.CENTER,
        val lengthDp: Int = 16,
        val thicknessDp: Int = 1,
        val color: Int = Color.LTGRAY,
        val marginDp: Int = 8,
        override val line: Int = 0
    ) : FrameElement(line, size = lengthDp)
    
    
    data class Spacer(
        val widthDp: Int = 8,
        override val line: Int = 0
    ) : FrameElement(line, size = widthDp)
}

enum class TextType {
    DEVICE_MODEL,     
    BRAND,            
    DATE,             
    TIME,             
    DATETIME,         
    LOCATION,         
    ISO,              
    SHUTTER_SPEED,    
    FOCAL_LENGTH,     
    FOCAL_LENGTH_35MM,     
    APERTURE,         
    RESOLUTION,       
    FILTER_NAME,      
    CUSTOM,           
    APP_NAME          
}

enum class LogoType {
    BRAND,    
    APP       
}

enum class ElementAlignment {
    START,
    CENTER,
    END
}

enum class FontWeight {
    NORMAL,
    MEDIUM,
    BOLD
}

enum class DividerOrientation {
    HORIZONTAL,
    VERTICAL
}
