package com.zoomx.mega.cameramega.utils

import android.text.TextUtils

object SystemPropertiesUtil {

    
    fun get(key: String): String? {
        val value = try {
            val clazz = Class.forName("android.os.SystemProperties")
            val getMethod = clazz.getMethod("get", String::class.java, String::class.java)
            getMethod.invoke(clazz, key, null) as? String
        } catch (e: Exception) {
            e.printStackTrace()
            null
        }
        if (TextUtils.isEmpty(value)) return null
        return value
    }
}