package com.mega.superx.filter.camera.gallery

import android.os.Environment

enum class PhotoSavePath(val relativePath: String?) {
    DCIM_PHOTON(Environment.DIRECTORY_DCIM + "/CameraMega"),
    EXTERNAL_TREE(null);

    companion object {
        fun fromPersistedName(name: String?): PhotoSavePath {
            return entries.firstOrNull { it.name == name } ?: DCIM_PHOTON
        }
    }
}
