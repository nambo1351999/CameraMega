package com.mega.filter.camera.utils

import kotlin.math.sqrt

class SplineInterpolator(private val x: FloatArray, private val y: FloatArray) {
    private val m: FloatArray

    init {
        require(x.size == y.size) { "x and y must have same size" }
        require(x.size >= 2) { "At least 2 points required" }
        for (i in 0 until x.size - 1) {
            require(x[i] < x[i + 1]) { "x must be strictly increasing" }
        }

        val n = x.size
        val d = FloatArray(n - 1) 
        val m = FloatArray(n)

        for (i in 0 until n - 1) {
            d[i] = (y[i + 1] - y[i]) / (x[i + 1] - x[i])
        }

        
        m[0] = d[0]
        for (i in 1 until n - 1) {
            m[i] = (d[i - 1] + d[i]) / 2f
        }
        m[n - 1] = d[n - 2]

        
        for (i in 0 until n - 1) {
            if (d[i] == 0f) {
                m[i] = 0f
                m[i + 1] = 0f
            } else {
                val a = m[i] / d[i]
                val b = m[i + 1] / d[i]
                val h = sqrt(a * a + b * b)
                if (h > 3f) {
                    val t = 3f / h
                    m[i] = t * a * d[i]
                    m[i + 1] = t * b * d[i]
                }
            }
        }
        this.m = m
    }

    fun interpolate(xi: Float): Float {
        val n = x.size
        if (xi <= x[0]) return y[0]
        if (xi >= x[n - 1]) return y[n - 1]

        
        var low = 0
        var high = n - 2
        while (low <= high) {
            val mid = (low + high) / 2
            if (xi < x[mid]) {
                high = mid - 1
            } else if (xi > x[mid + 1]) {
                low = mid + 1
            } else {
                
                val h = x[mid + 1] - x[mid]
                val t = (xi - x[mid]) / h
                val t2 = t * t
                val t3 = t2 * t
                return (2 * t3 - 3 * t2 + 1) * y[mid] +
                        (t3 - 2 * t2 + t) * h * m[mid] +
                        (-2 * t3 + 3 * t2) * y[mid + 1] +
                        (t3 - t2) * h * m[mid + 1]
            }
        }
        return y[n - 1]
    }

    
    fun generateLut(size: Int): FloatArray {
        val lut = FloatArray(size)
        
        val step = 1.0f / (size - 1)
        for (i in 0 until size) {
            lut[i] = interpolate(i * step)
        }
        return lut
    }
}
