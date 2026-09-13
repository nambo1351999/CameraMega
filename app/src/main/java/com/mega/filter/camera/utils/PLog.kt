package com.mega.filter.camera.utils

import android.os.Build
import android.util.Log
import java.text.SimpleDateFormat
import java.util.*
import java.util.concurrent.ConcurrentLinkedQueue

object PLog {
    private const val TAG = "LogManager"
    private const val MAX_LOG_SIZE = 500 

    
    private val logQueue = ConcurrentLinkedQueue<LogEntry>()

    
    enum class LogLevel {
        VERBOSE, DEBUG, INFO, WARNING, ERROR
    }

    
    data class LogEntry(
        val timestamp: Long,
        val level: LogLevel,
        val tag: String,
        val message: String,
        val throwable: Throwable? = null
    ) {
        fun getFormattedTime(): String {
            val sdf = SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.getDefault())
            return sdf.format(Date(timestamp))
        }

        fun getFormattedLog(): String {
            val levelStr = when (level) {
                LogLevel.VERBOSE -> "V"
                LogLevel.DEBUG -> "D"
                LogLevel.INFO -> "I"
                LogLevel.WARNING -> "W"
                LogLevel.ERROR -> "E"
            }

            val throwableStr = throwable?.let { "\n${Log.getStackTraceString(it)}" } ?: ""
            return "${getFormattedTime()} $levelStr/$tag: $message$throwableStr"
        }
    }

    
    fun v(tag: String, message: String) {
        addLog(LogLevel.VERBOSE, tag, message)
        Log.v("PLog_$tag", message)
    }

    
    fun d(tag: String, message: String) {
        addLog(LogLevel.DEBUG, tag, message)
        Log.d("PLog_$tag", message)
    }

    
    fun i(tag: String, message: String) {
        addLog(LogLevel.INFO, tag, message)
        Log.i("PLog_$tag", message)
    }

    
    fun w(tag: String, message: String, throwable: Throwable? = null) {
        addLog(LogLevel.WARNING, tag, message, throwable)
        if (throwable != null) {
            Log.w("PLog_$tag", message, throwable)
        } else {
            Log.w("PLog_$tag", message)
        }
    }

    
    fun e(tag: String, message: String, throwable: Throwable? = null) {
        addLog(LogLevel.ERROR, tag, message, throwable)
        if (throwable != null) {
            BuglyHelper.error(Throwable("$tag: $message", throwable))
            Log.e("PLog_$tag", message, throwable)
        } else {
            Log.e("PLog_$tag", message)
        }
    }

    
    private fun addLog(level: LogLevel, tag: String, message: String, throwable: Throwable? = null) {
        val entry = LogEntry(
            timestamp = System.currentTimeMillis(),
            level = level,
            tag = tag,
            message = message,
            throwable = throwable
        )

        logQueue.offer(entry)

        
        while (logQueue.size > MAX_LOG_SIZE) {
            logQueue.poll()
        }
    }

    
    fun getAllLogs(): List<LogEntry> {
        return logQueue.reversed()
    }

    
    fun getFormattedLogs(): String {
        if (logQueue.isEmpty()) {
            return "暂无日志记录"
        }

        return buildString {
            appendLine("=== MyCamera 日志记录 ===")
            appendLine("导出时间: ${SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(Date())}")
            appendLine("日志总数: ${logQueue.size}")
            appendLine("厂商: ${Build.MANUFACTURER}")
            appendLine("型号: ${Build.MODEL}")
            appendLine("Android 版本: ${Build.VERSION.RELEASE}")
            appendLine("=".repeat(50))
            appendLine()

            getAllLogs().forEach { entry ->
                appendLine(entry.getFormattedLog())
            }
        }
    }

    
    fun clearLogs() {
        logQueue.clear()
        Log.i(TAG, "日志已清空")
    }

    
    fun getLogsByLevel(level: LogLevel): List<LogEntry> {
        return logQueue.filter { it.level == level }
    }

    
    fun getLogsByTag(tag: String): List<LogEntry> {
        return logQueue.filter { it.tag == tag }
    }

    
    fun getLogStats(): Map<LogLevel, Int> {
        return LogLevel.entries.associateWith { level ->
            logQueue.count { it.level == level }
        }
    }
}
