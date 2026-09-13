package com.mega.filter.camera.gallery

import android.net.Uri

enum class MediaType {
    IMAGE,
    VIDEO
}

data class MediaData(
    val id: String,
    val uri: Uri,
    val thumbnailUri: Uri,
    val displayName: String,
    val dateAdded: Long,
    val size: Long,
    val width: Int = 0,
    val height: Int = 0,
    val mediaType: MediaType = MediaType.IMAGE,
    val mimeType: String? = null,
    val durationMs: Long? = null,
    val sourceUri: Uri? = null,
    var isMotionPhoto: Boolean = false,
    var isBurstPhoto: Boolean = false,
    
    var metadata: MediaMetadata? = null,
    var relatedPhoto: MediaData? = null
) {
    val isVideo: Boolean
        get() = mediaType == MediaType.VIDEO

    val isImage: Boolean
        get() = mediaType == MediaType.IMAGE

    
    fun getFormattedDate(): String {
        val date = dateAdded
        val sdf = java.text.SimpleDateFormat("yyyy-MM-dd HH:mm", java.util.Locale.getDefault())
        return sdf.format(java.util.Date(date))
    }
    
    
    fun getFormattedSize(): String {
        return when {
            size < 1024 -> "$size B"
            size < 1024 * 1024 -> "${size / 1024} KB"
            else -> String.format("%.1f MB", size / (1024.0 * 1024.0))
        }
    }
    
    
    fun getResolution(): String {
        val displayedPhoto = relatedPhoto ?: this
        val displayedMetadata = displayedPhoto.metadata
        val displayedWidth = displayedMetadata?.width?.takeIf { it > 0 } ?: displayedPhoto.width
        val displayedHeight = displayedMetadata?.height?.takeIf { it > 0 } ?: displayedPhoto.height
        return "${displayedWidth}x${displayedHeight}"
    }

    fun getFormattedDuration(): String {
        val totalSeconds = ((durationMs ?: 0L) / 1000L).coerceAtLeast(0L)
        val hours = totalSeconds / 3600L
        val minutes = (totalSeconds % 3600L) / 60L
        val seconds = totalSeconds % 60L
        return if (hours > 0L) {
            String.format("%d:%02d:%02d", hours, minutes, seconds)
        } else {
            String.format("%02d:%02d", minutes, seconds)
        }
    }
}
