package com.zoomx.mega.cameramega.lut

internal object BasicToneLutShader {
    val GLSL = """
        uniform mediump sampler3D uBasicToneLut;
        uniform float uBasicToneIntensity;

        vec3 applyBasicToneLut(vec3 color) {
            float intensity = clamp(uBasicToneIntensity, 0.0, 1.0);
            if (intensity < 0.001) {
                return color;
            }
            const float lutSize = 32.0;
            float scale = (lutSize - 1.0) / lutSize;
            float offset = 1.0 / (2.0 * lutSize);
            vec3 coordinate = clamp(color, 0.0, 1.0) * scale + offset;
            vec3 endpointColor = texture(uBasicToneLut, coordinate).rgb;
            return mix(color, endpointColor, intensity);
        }
    """.trimIndent()
}
