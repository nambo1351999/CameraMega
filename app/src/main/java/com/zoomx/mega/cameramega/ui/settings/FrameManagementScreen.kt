package com.zoomx.mega.cameramega.ui.settings

import android.net.Uri
import android.widget.Toast
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.zoomx.mega.cameramega.R
import com.zoomx.mega.cameramega.frame.FrameInfo
import com.zoomx.mega.cameramega.ui.camera.autoRotate
import com.zoomx.mega.cameramega.utils.PLog
import com.zoomx.mega.cameramega.viewmodel.CameraViewModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import sh.calvin.reorderable.ReorderableItem
import sh.calvin.reorderable.rememberReorderableLazyListState
import com.zoomx.mega.cameramega.ui.icons.AppIcons

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun FrameManagementScreen(
    viewModel: CameraViewModel,
    onBack: () -> Unit,
    onCreateFrameClick: () -> Unit,
    onEditFrameStyle: (String) -> Unit,
    modifier: Modifier = Modifier
) {
    val currentFrameId = viewModel.currentFrameId
    val phantomFrameId by viewModel.phantomFrameId.collectAsState()
    val availableFrames = viewModel.availableFrameList
    val customImportManager = viewModel.getCustomImportManager()
    val scope = rememberCoroutineScope()
    val context = LocalContext.current

    
    var localFrameList by remember { mutableStateOf(availableFrames) }

    
    LaunchedEffect(availableFrames) {
        val existingIds = localFrameList.map { it.id }.toSet()
        val newItems = availableFrames.filter { it.id !in existingIds }
        val updatedExisting = localFrameList.mapNotNull { local ->
            availableFrames.find { it.id == local.id }
        }
        localFrameList = newItems + updatedExisting
    }

    
    var showRenameDialog by remember { mutableStateOf(false) }
    var renamingFrame by remember { mutableStateOf<FrameInfo?>(null) }
    var renameText by remember { mutableStateOf("") }

    
    var showCopyDialog by remember { mutableStateOf(false) }
    var copyingFrame by remember { mutableStateOf<FrameInfo?>(null) }
    var copyText by remember { mutableStateOf("") }

    
    var showDeleteDialog by remember { mutableStateOf(false) }
    var deletingFrame by remember { mutableStateOf<FrameInfo?>(null) }

    
    var isImporting by remember { mutableStateOf(false) }
    var pendingExportBytes by remember { mutableStateOf(ByteArray(0)) }

    
    var showCreateMenu by remember { mutableStateOf(false) }

    
    val frameJsonPicker = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.GetContent()
    ) { uri: Uri? ->
        uri?.let {
            isImporting = true
            scope.launch {
                withContext(Dispatchers.IO) {
                    customImportManager.importFrame(it)
                }
                viewModel.refreshCustomContent()
                isImporting = false
            }
        }
    }

    val exportLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.CreateDocument("application/json")
    ) { uri: Uri? ->
        uri?.let {
            scope.launch {
                withContext(Dispatchers.IO) {
                    try {
                        context.contentResolver.openOutputStream(it)?.use { output ->
                            output.write(pendingExportBytes)
                        }
                        withContext(Dispatchers.Main) {
                            Toast.makeText(
                                context,
                                context.getString(R.string.frame_export_success),
                                Toast.LENGTH_SHORT
                            ).show()
                        }
                    } catch (e: Exception) {
                        PLog.e("FrameManagementScreen", "Failed to export frame", e)
                        withContext(Dispatchers.Main) {
                            Toast.makeText(
                                context,
                                context.getString(R.string.frame_export_failed),
                                Toast.LENGTH_SHORT
                            ).show()
                        }
                    }
                }
            }
        }
    }

    
    val lazyListState = rememberLazyListState()
    val reorderableLazyListState = rememberReorderableLazyListState(lazyListState) { from, to ->
        val fromId = from.key as? String ?: return@rememberReorderableLazyListState
        val toId = to.key as? String ?: return@rememberReorderableLazyListState

        
        if (fromId == "none" || toId == "none") return@rememberReorderableLazyListState

        
        val fromIndexInLocal = localFrameList.indexOfFirst { it.id == fromId }
        val toIndexInLocal = localFrameList.indexOfFirst { it.id == toId }

        if (fromIndexInLocal != -1 && toIndexInLocal != -1) {
            
            localFrameList = localFrameList.toMutableList().apply {
                add(toIndexInLocal, removeAt(fromIndexInLocal))
            }
        }
    }

    val backgroundColor = Color(0xFF151515)

    Column(
        modifier = modifier
            .fillMaxSize()
            .background(backgroundColor)
            .navigationBarsPadding()
    ) {
        
        TopAppBar(
            title = {
                Text(
                    text = stringResource(R.string.frame_management_title),
                    color = Color.White,
                    fontSize = 20.sp,
                    fontWeight = FontWeight.Medium
                )
            },
            navigationIcon = {
                IconButton(
                    onClick = {
                        
                        viewModel.saveFrameOrder(localFrameList.map { it.id })
                        onBack()
                    },
                    modifier = Modifier.autoRotate()
                ) {
                    Icon(
                        imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                        contentDescription = stringResource(R.string.back),
                        tint = Color.White
                    )
                }
            },
            actions = {
                
                IconButton(
                    onClick = { showCreateMenu = true },
                    enabled = !isImporting
                ) {
                    if (isImporting) {
                        CircularProgressIndicator(
                            modifier = Modifier.size(24.dp),
                            color = Color.White,
                            strokeWidth = 2.dp
                        )
                    } else {
                        Icon(
                            imageVector = Icons.Default.Add,
                            contentDescription = stringResource(R.string.frame_editor_create_menu),
                            tint = Color.White
                        )
                    }
                }
                DropdownMenu(
                    expanded = showCreateMenu,
                    onDismissRequest = { showCreateMenu = false }
                ) {
                    DropdownMenuItem(
                        text = { Text(stringResource(R.string.frame_editor_new_title)) },
                        onClick = {
                            showCreateMenu = false
                            onCreateFrameClick()
                        }
                    )
                    DropdownMenuItem(
                        text = { Text(stringResource(R.string.import_frame_json)) },
                        onClick = {
                            showCreateMenu = false
                            frameJsonPicker.launch("*/*")
                        }
                    )
                }
            },
            colors = TopAppBarDefaults.topAppBarColors(
                containerColor = backgroundColor
            )
        )

        
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(horizontal = 16.dp),
            state = lazyListState,
            verticalArrangement = Arrangement.spacedBy(8.dp),
            contentPadding = PaddingValues(vertical = 16.dp)
        ) {
            
            item(key = "none") {
                FrameManagementItem(
                    name = stringResource(R.string.none),
                    isBuiltIn = true,
                    isDefault = currentFrameId == null,
                    isDragging = false,
                    canDrag = false,
                    onSetDefault = {
                        viewModel.setFrame(null)
                    },
                        onEditStyle = null,
                        onCopy = null,
                        onExport = null,
                        onRename = null,
                        onDelete = null
                )
            }

            itemsIndexed(localFrameList, key = { _, it -> it.id }) { index, frameInfo ->
                ReorderableItem(reorderableLazyListState, key = frameInfo.id) { isDragging ->
                    val copySuffix = stringResource(R.string.copy_suffix)
                    FrameManagementItem(
                        name = frameInfo.getName(),
                        isBuiltIn = frameInfo.isBuiltIn,
                        isDefault = currentFrameId == frameInfo.id,
                        isDragging = isDragging,
                        canDrag = true,
                        onSetDefault = {
                            viewModel.setFrame(frameInfo.id)
                        },
                        onEditStyle = {
                            onEditFrameStyle(frameInfo.id)
                        },
                        onCopy = {
                            copyingFrame = frameInfo
                            copyText = frameInfo.getName() + copySuffix
                            showCopyDialog = true
                        },
                        onExport = {
                            scope.launch {
                                val bytes = viewModel.exportFrameToJson(frameInfo)
                                if (bytes != null) {
                                    pendingExportBytes = bytes
                                    exportLauncher.launch("${frameInfo.getName()}.json")
                                } else {
                                    Toast.makeText(
                                        context,
                                        context.getString(R.string.frame_export_failed),
                                        Toast.LENGTH_SHORT
                                    ).show()
                                }
                            }
                        },
                        onRename = if (!frameInfo.isBuiltIn) {
                            {
                                renamingFrame = frameInfo
                                renameText = frameInfo.getName()
                                showRenameDialog = true
                            }
                        } else null,
                        onDelete = if (!frameInfo.isBuiltIn) {
                            {
                                deletingFrame = frameInfo
                                showDeleteDialog = true
                            }
                        } else null,
                        dragModifier = Modifier.draggableHandle()
                    )
                }
            }
        }
    }

    
    if (showRenameDialog && renamingFrame != null) {
        AlertDialog(
            onDismissRequest = { showRenameDialog = false },
            title = {
                Text(stringResource(R.string.rename_dialog_title))
            },
            text = {
                OutlinedTextField(
                    value = renameText,
                    onValueChange = { renameText = it },
                    label = { Text(stringResource(R.string.name)) },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth()
                )
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        scope.launch {
                            withContext(Dispatchers.IO) {
                                customImportManager.updateFrameName(renamingFrame!!.id, renameText)
                            }
                            viewModel.refreshCustomContent()
                            showRenameDialog = false
                            renamingFrame = null
                        }
                    }
                ) {
                    Text(stringResource(R.string.confirm))
                }
            },
            dismissButton = {
                TextButton(onClick = { showRenameDialog = false }) {
                    Text(stringResource(R.string.cancel))
                }
            }
        )
    }

    
    if (showCopyDialog && copyingFrame != null) {
        AlertDialog(
            onDismissRequest = { showCopyDialog = false },
            title = {
                Text(stringResource(R.string.copy_frame_dialog_title))
            },
            text = {
                OutlinedTextField(
                    value = copyText,
                    onValueChange = { copyText = it },
                    label = { Text(stringResource(R.string.name)) },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth()
                )
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        viewModel.copyFrame(copyingFrame!!, copyText)
                        showCopyDialog = false
                        copyingFrame = null
                    }
                ) {
                    Text(stringResource(R.string.confirm))
                }
            },
            dismissButton = {
                TextButton(onClick = { showCopyDialog = false }) {
                    Text(stringResource(R.string.cancel))
                }
            }
        )
    }

    
    if (showDeleteDialog && deletingFrame != null) {
        AlertDialog(
            onDismissRequest = { showDeleteDialog = false },
            title = {
                Text(stringResource(R.string.delete_confirm_title))
            },
            text = {
                Text(stringResource(R.string.delete_frame_confirm_message, deletingFrame!!.getName()))
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        scope.launch {
                            withContext(Dispatchers.IO) {
                                customImportManager.deleteCustomFrame(deletingFrame!!.id)
                            }
                            
                            if (currentFrameId == deletingFrame!!.id) {
                                viewModel.setFrame(null)
                            }
                            if (phantomFrameId == deletingFrame!!.id) {
                                viewModel.setPhantomFrame(null)
                            }
                            viewModel.refreshCustomContent()
                            showDeleteDialog = false
                            deletingFrame = null
                        }
                    },
                    colors = ButtonDefaults.textButtonColors(
                        contentColor = MaterialTheme.colorScheme.error
                    )
                ) {
                    Text(stringResource(R.string.delete))
                }
            },
            dismissButton = {
                TextButton(onClick = { showDeleteDialog = false }) {
                    Text(stringResource(R.string.cancel))
                }
            }
        )
    }

    
    DisposableEffect(Unit) {
        onDispose {
            viewModel.saveFrameOrder(localFrameList.map { it.id })
        }
    }

}

@Composable
private fun FrameManagementItem(
    name: String,
    isBuiltIn: Boolean,
    isDefault: Boolean,
    isDragging: Boolean,
    canDrag: Boolean,
    onSetDefault: () -> Unit,
    onEditStyle: (() -> Unit)?,
    onCopy: (() -> Unit)?,
    onExport: (() -> Unit)?,
    onRename: (() -> Unit)?,
    onDelete: (() -> Unit)?,
    dragModifier: Modifier = Modifier,
    modifier: Modifier = Modifier
) {
    var showActionsMenu by remember { mutableStateOf(false) }
    val borderColor = if (isDefault) Color(0xFFFF6B35) else Color.White.copy(alpha = 0.2f)
    val backgroundColor = when {
        isDragging -> Color.White.copy(alpha = 0.2f)
        isDefault -> Color.White.copy(alpha = 0.1f)
        else -> Color.White.copy(alpha = 0.05f)
    }

    Row(
        modifier = modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(12.dp))
            .background(backgroundColor)
            .border(
                width = if (isDefault) 2.dp else 1.dp,
                color = borderColor,
                shape = RoundedCornerShape(12.dp)
            )
            .padding(horizontal = 12.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        
        if (canDrag) {
            Box(
                modifier = Modifier.size(40.dp),
                contentAlignment = Alignment.Center
            ) {
                Icon(
                    imageVector = AppIcons.DragHandle,
                    contentDescription = "Drag to reorder",
                    tint = Color.White.copy(alpha = 0.5f),
                    modifier = dragModifier.size(24.dp)
                )
            }
            Spacer(modifier = Modifier.width(12.dp))
        } else {
            Spacer(modifier = Modifier.width(52.dp)) 
        }

        
        Column(
            modifier = Modifier
                .weight(1f)
                .clip(RoundedCornerShape(8.dp))
                .clickable(onClick = onSetDefault)
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(
                    text = name,
                    color = Color.White,
                    fontSize = 16.sp,
                    fontWeight = FontWeight.Medium,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                    modifier = Modifier.weight(1f, fill = false)
                )

                Spacer(modifier = Modifier.width(8.dp))

                
                val typeText = if (isBuiltIn) {
                    stringResource(R.string.built_in)
                } else {
                    stringResource(R.string.custom)
                }
                Text(
                    text = typeText,
                    color = if (isBuiltIn) Color.White.copy(alpha = 0.5f) else Color(0xFFFF6B35),
                    fontSize = 11.sp,
                    modifier = Modifier
                        .clip(RoundedCornerShape(4.dp))
                        .background(
                            if (isBuiltIn) Color.White.copy(alpha = 0.1f)
                            else Color(0xFFFF6B35).copy(alpha = 0.2f)
                        )
                        .padding(horizontal = 6.dp, vertical = 2.dp)
                )
            }

            
            if (isDefault) {
                Spacer(modifier = Modifier.height(4.dp))
                Text(
                    text = stringResource(R.string.current_default),
                    color = Color(0xFFFF6B35),
                    fontSize = 12.sp
                )
            }
        }

        if (onEditStyle != null || onCopy != null || onExport != null || onRename != null || onDelete != null) {
            Box {
                IconButton(
                    onClick = { showActionsMenu = true },
                    modifier = Modifier.size(36.dp)
                ) {
                    Icon(
                        imageVector = Icons.Default.MoreVert,
                        contentDescription = stringResource(R.string.more_options),
                        tint = Color.White.copy(alpha = 0.7f),
                        modifier = Modifier.size(20.dp)
                    )
                }
                DropdownMenu(
                    expanded = showActionsMenu,
                    onDismissRequest = { showActionsMenu = false }
                ) {
                    onEditStyle?.let {
                        DropdownMenuItem(
                            text = { Text(stringResource(R.string.frame_editor_edit_title)) },
                            onClick = {
                                showActionsMenu = false
                                it()
                            }
                        )
                    }
                    onCopy?.let {
                        DropdownMenuItem(
                            text = { Text(stringResource(R.string.copy)) },
                            onClick = {
                                showActionsMenu = false
                                it()
                            }
                        )
                    }
                    onExport?.let {
                        DropdownMenuItem(
                            text = { Text(stringResource(R.string.export_frame_json)) },
                            onClick = {
                                showActionsMenu = false
                                it()
                            }
                        )
                    }
                    onRename?.let {
                        DropdownMenuItem(
                            text = { Text(stringResource(R.string.rename)) },
                            onClick = {
                                showActionsMenu = false
                                it()
                            }
                        )
                    }
                    onDelete?.let {
                        DropdownMenuItem(
                            text = { Text(stringResource(R.string.delete)) },
                            onClick = {
                                showActionsMenu = false
                                it()
                            }
                        )
                    }
                }
            }
        }
    }
}
