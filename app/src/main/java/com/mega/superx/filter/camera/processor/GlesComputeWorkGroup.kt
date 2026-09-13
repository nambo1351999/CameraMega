package com.mega.superx.filter.camera.processor

internal object GlesComputeWorkGroup {
    const val BASELINE_MAX_INVOCATIONS = 128
    const val BASELINE_MAX_SIZE_X = 128
    const val BASELINE_MAX_SIZE_Y = 128
    const val BASELINE_MAX_SIZE_Z = 64

    const val IMAGE_TILE_SIZE = 8
    const val LINEAR_SIZE = 128

    private val layoutPattern =
        Regex("""layout\s*\(([^)]*local_size_[xyz][^)]*)\)\s*in\s*;""")
    private val axisPattern = Regex("""local_size_([xyz])\s*=\s*(\d+)""")

    data class Size(
        val x: Int,
        val y: Int,
        val z: Int,
    ) {
        val invocations: Int
            get() = x * y * z
    }

    fun imageGroupCount(value: Int): Int =
        groupCount(value, IMAGE_TILE_SIZE)

    fun linearGroupCount(value: Int): Int =
        groupCount(value, LINEAR_SIZE)

    fun groupCount(value: Int, localSize: Int): Int {
        require(value >= 0)
        require(localSize > 0)
        return (value + localSize - 1) / localSize
    }

    fun declaredSize(source: String): Size? {
        val layout = layoutPattern.find(source)?.groupValues?.get(1) ?: return null
        val sizes = axisPattern.findAll(layout).associate { match ->
            match.groupValues[1] to match.groupValues[2].toInt()
        }
        return Size(
            x = sizes["x"] ?: 1,
            y = sizes["y"] ?: 1,
            z = sizes["z"] ?: 1,
        )
    }

    
    fun requireBaselineCompatible(source: String, name: String) {
        val size = requireNotNull(declaredSize(source)) {
            "Compute shader $name does not declare a local work-group size"
        }
        require(size.x in 1..BASELINE_MAX_SIZE_X) {
            "Compute shader $name local_size_x=${size.x} exceeds GLES 3.1 baseline"
        }
        require(size.y in 1..BASELINE_MAX_SIZE_Y) {
            "Compute shader $name local_size_y=${size.y} exceeds GLES 3.1 baseline"
        }
        require(size.z in 1..BASELINE_MAX_SIZE_Z) {
            "Compute shader $name local_size_z=${size.z} exceeds GLES 3.1 baseline"
        }
        require(size.invocations <= BASELINE_MAX_INVOCATIONS) {
            "Compute shader $name has ${size.invocations} local invocations; " +
                "GLES 3.1 only guarantees $BASELINE_MAX_INVOCATIONS"
        }
    }
}
