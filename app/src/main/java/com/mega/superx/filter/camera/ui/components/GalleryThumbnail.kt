package com.mega.superx.filter.camera.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material3.Icon
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import coil.compose.AsyncImage
import coil.request.ImageRequest
import com.mega.superx.filter.camera.gallery.MediaData
import com.mega.superx.filter.camera.viewmodel.GalleryViewModel
import com.mega.superx.filter.camera.ui.icons.AppIcons

@Composable
fun GalleryThumbnail(
    latestPhoto: MediaData?,
    viewModel: GalleryViewModel,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val context = LocalContext.current
    val refreshKey = latestPhoto?.id?.let { viewModel.getPreparedPhotoThumbnailRefreshKey(it) } ?: 0L
    val buttonShape = RoundedCornerShape(10.dp)
    val thumbnailShape = RoundedCornerShape(7.dp)

    PhysicalButton(
        modifier = modifier
            .size(48.dp),
        onClick = onClick,
        shape = buttonShape,
        contentShape = thumbnailShape
    ) {
        if (latestPhoto != null) {
            AsyncImage(
                model = ImageRequest.Builder(context)
                    .data(latestPhoto.thumbnailUri)
                    .memoryCacheKey(
                        "gallery_thumbnail_${latestPhoto.id}_${latestPhoto.thumbnailUri}_${refreshKey}"
                    )
                    .crossfade(true)
                    .build(),
                contentDescription = "Gallery",
                contentScale = ContentScale.Crop,
                modifier = Modifier.fillMaxSize()
            )
            if (latestPhoto.isVideo) {
                Icon(
                    imageVector = Icons.Default.PlayArrow,
                    contentDescription = null,
                    tint = Color.White,
                    modifier = Modifier.size(20.dp)
                )
            }
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .background(
                        Brush.verticalGradient(
                            colors = listOf(
                                Color.White.copy(alpha = 0.08f),
                                Color.Transparent,
                                Color.Black.copy(alpha = 0.14f)
                            )
                        )
                    )
            )
        } else {
            
            Icon(
                imageVector = AppIcons.PhotoLibrary,
                contentDescription = "Gallery",
                tint = Color.White.copy(alpha = 0.7f),
                modifier = Modifier.size(24.dp)
            )
        }
    }
}
