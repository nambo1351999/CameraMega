package com.mega.filter.camera.lut

import android.opengl.GLES11Ext
import android.opengl.GLES30
import android.util.Log
import com.mega.filter.camera.utils.PLog
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer

object GlUtils {
    
    private const val TAG = "GlUtils"
    
    
    fun compileShader(type: Int, source: String): Int {
        val shader = GLES30.glCreateShader(type)
        if (shader == 0) {
            PLog.e(TAG, "Failed to create shader")
            return 0
        }
        
        GLES30.glShaderSource(shader, source)
        GLES30.glCompileShader(shader)
        
        val compileStatus = IntArray(1)
        GLES30.glGetShaderiv(shader, GLES30.GL_COMPILE_STATUS, compileStatus, 0)
        
        if (compileStatus[0] == 0) {
            val errorLog = GLES30.glGetShaderInfoLog(shader)
            PLog.e(TAG, "Shader compilation failed: $errorLog")
            GLES30.glDeleteShader(shader)
            return 0
        }
        
        return shader
    }
    
    
    fun linkProgram(vertexShader: Int, fragmentShader: Int): Int {
        val program = GLES30.glCreateProgram()
        if (program == 0) {
            PLog.e(TAG, "Failed to create program")
            return 0
        }
        
        GLES30.glAttachShader(program, vertexShader)
        GLES30.glAttachShader(program, fragmentShader)
        GLES30.glLinkProgram(program)
        
        val linkStatus = IntArray(1)
        GLES30.glGetProgramiv(program, GLES30.GL_LINK_STATUS, linkStatus, 0)
        
        if (linkStatus[0] == 0) {
            val errorLog = GLES30.glGetProgramInfoLog(program)
            PLog.e(TAG, "Program linking failed: $errorLog")
            GLES30.glDeleteProgram(program)
            return 0
        }
        
        return program
    }
    
    
    fun create3DTexture(lutConfig: LutConfig): Int {
        val textureIds = IntArray(1)
        GLES30.glGenTextures(1, textureIds, 0)
        val textureId = textureIds[0]
        
        if (textureId == 0) {

            return 0
        }
        
        GLES30.glBindTexture(GLES30.GL_TEXTURE_3D, textureId)
        
        
        GLES30.glPixelStorei(GLES30.GL_UNPACK_ALIGNMENT, 1)
        
        
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_MIN_FILTER, GLES30.GL_LINEAR)
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_MAG_FILTER, GLES30.GL_LINEAR)
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_WRAP_S, GLES30.GL_CLAMP_TO_EDGE)
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_WRAP_T, GLES30.GL_CLAMP_TO_EDGE)
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_WRAP_R, GLES30.GL_CLAMP_TO_EDGE)
        
        
        if (lutConfig.configDataType == LutConfig.CONFIG_DATA_TYPE_UINT16) {
            
            val buffer = lutConfig.toFloatBuffer()
            GLES30.glTexImage3D(
                GLES30.GL_TEXTURE_3D,
                0,
                GLES30.GL_RGB16F,
                lutConfig.size,
                lutConfig.size,
                lutConfig.size,
                0,
                GLES30.GL_RGB,
                GLES30.GL_FLOAT,
                buffer
            )
        } else {
            
            val buffer = lutConfig.toByteBuffer()
            GLES30.glTexImage3D(
                GLES30.GL_TEXTURE_3D,
                0,
                GLES30.GL_RGB8,
                lutConfig.size,
                lutConfig.size,
                lutConfig.size,
                0,
                GLES30.GL_RGB,
                GLES30.GL_UNSIGNED_BYTE,
                buffer
            )
        }
        
        
        GLES30.glPixelStorei(GLES30.GL_UNPACK_ALIGNMENT, 4)
        
        GLES30.glBindTexture(GLES30.GL_TEXTURE_3D, 0)
        
        checkGlError("create3DTexture")

        
        return textureId
    }
    
    
    fun createOESTexture(): Int {
        val textureIds = IntArray(1)
        GLES30.glGenTextures(1, textureIds, 0)
        val textureId = textureIds[0]
        
        if (textureId == 0) {
            PLog.e(TAG, "Failed to generate OES texture")
            return 0
        }
        
        GLES30.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureId)
        
        
        GLES30.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES30.GL_TEXTURE_MIN_FILTER, GLES30.GL_LINEAR)
        GLES30.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES30.GL_TEXTURE_MAG_FILTER, GLES30.GL_LINEAR)
        GLES30.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES30.GL_TEXTURE_WRAP_S, GLES30.GL_CLAMP_TO_EDGE)
        GLES30.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES30.GL_TEXTURE_WRAP_T, GLES30.GL_CLAMP_TO_EDGE)
        
        GLES30.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0)
        
        checkGlError("createOESTexture")
        
        return textureId
    }
    
    
    fun createBuffer(data: FloatArray): Int {
        val bufferIds = IntArray(1)
        GLES30.glGenBuffers(1, bufferIds, 0)
        val bufferId = bufferIds[0]
        
        val buffer = createFloatBuffer(data)
        
        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, bufferId)
        GLES30.glBufferData(
            GLES30.GL_ARRAY_BUFFER,
            data.size * 4,
            buffer,
            GLES30.GL_STATIC_DRAW
        )
        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, 0)
        
        return bufferId
    }
    
    
    fun createFloatBuffer(data: FloatArray): FloatBuffer {
        val buffer = ByteBuffer.allocateDirect(data.size * 4)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer()
        buffer.put(data)
        buffer.position(0)
        return buffer
    }
    
    
    fun deleteTexture(textureId: Int) {
        if (textureId != 0) {
            GLES30.glDeleteTextures(1, intArrayOf(textureId), 0)
        }
    }
    
    
    fun deleteProgram(programId: Int) {
        if (programId != 0) {
            GLES30.glDeleteProgram(programId)
        }
    }
    
    
    fun checkGlError(tag: String) {
        var error: Int
        while (GLES30.glGetError().also { error = it } != GLES30.GL_NO_ERROR) {
            PLog.e(TAG, "$tag: glError $error")
        }
    }
}
