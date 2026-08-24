package com.zoomx.mega.cameramega.lut

object Shaders {

    
    
    val VERTEX_SHADER = """
        #version 300 es

        in vec4 aPosition;
        in vec2 aTexCoord;

        out vec2 vTexCoord;
        out vec2 vRawCoord;

        uniform mat4 uMVPMatrix;

        uniform mat4 uSTMatrix;
        uniform vec4 uCropRect;

        void main() {

            gl_Position = uMVPMatrix * aPosition;
            vec2 croppedCoord = vec2(
                mix(uCropRect.x, uCropRect.z, aTexCoord.x),
                mix(uCropRect.y, uCropRect.w, aTexCoord.y)
            );

            vTexCoord = (uSTMatrix * vec4(croppedCoord, 0.0, 1.0)).xy;
            vRawCoord = croppedCoord;
        }
    """.trimIndent()

    
    val FRAGMENT_SHADER_PASSTHROUGH = """
        #version 300 es
        #extension GL_OES_EGL_image_external_essl3 : require

        precision mediump float;

        in vec2 vTexCoord;
        out vec4 fragColor;

        uniform samplerExternalOES uCameraTexture;

        void main() {
            fragColor = texture(uCameraTexture, vTexCoord);
        }
    """.trimIndent()

    
    val FRAGMENT_SHADER_COPY_2D = """
        #version 300 es
        precision mediump float;

        in vec2 vTexCoord;
        out vec4 fragColor;

        uniform sampler2D uCameraTexture;

        void main() {
            fragColor = texture(uCameraTexture, vTexCoord);
        }
    """.trimIndent()

    
    val SIMPLE_VERTEX_SHADER = """
        #version 300 es
        in vec4 aPosition;
        in vec2 aTexCoord;
        out vec2 vTexCoord;
        void main() {
            gl_Position = aPosition;
            vTexCoord = aTexCoord;
        }
    """.trimIndent()

    
    val HDF_PREVIEW_EXTRACT_BLUR_H = """
        #version 300 es
        precision highp float;
        in vec2 vTexCoord;
        out vec4 fragColor;
        uniform sampler2D uInputTexture;
        uniform vec2 uTexelSize;
        uniform float uThreshold;
        uniform float uStrength;
        void main() {
            vec3 color = texture(uInputTexture, vTexCoord).rgb;
            float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
            float extractionVal = mix(luma, max(color.r, max(color.g, color.b)), 0.6);
            float highlightMask = smoothstep(uThreshold - 0.1, uThreshold + 0.25, extractionVal);
            float midMask = smoothstep(uThreshold - 0.5, uThreshold, extractionVal) * 0.4;
            float mask = (highlightMask + midMask * uStrength);
            vec3 sum = color * mask * 0.204164;
            float blurOffsets[4] = float[](1.407333, 3.294215, 5.176470, 7.058823);
            float blurWeights[4] = float[](0.304005, 0.093910, 0.010416, 0.000005);
            for (int i = 0; i < 4; i++) {
                float off = blurOffsets[i] * uTexelSize.x * 2.0;
                sum += texture(uInputTexture, vTexCoord + vec2(off, 0.0)).rgb * blurWeights[i];
                sum += texture(uInputTexture, vTexCoord - vec2(off, 0.0)).rgb * blurWeights[i];
            }
            fragColor = vec4(sum, 1.0);
        }
    """.trimIndent()

    
    val HDF_PREVIEW_BLUR_V = """
        #version 300 es
        precision highp float;
        in vec2 vTexCoord;
        out vec4 fragColor;
        uniform sampler2D uInputTexture;
        uniform vec2 uTexelSize;
        void main() {
            vec3 sum = texture(uInputTexture, vTexCoord).rgb * 0.204164;
            float blurOffsets[4] = float[](1.407333, 3.294215, 5.176470, 7.058823);
            float blurWeights[4] = float[](0.304005, 0.093910, 0.010416, 0.000005);
            for (int i = 0; i < 4; i++) {
                float off = blurOffsets[i] * uTexelSize.y * 2.0;
                sum += texture(uInputTexture, vTexCoord + vec2(0.0, off)).rgb * blurWeights[i];
                sum += texture(uInputTexture, vTexCoord - vec2(0.0, off)).rgb * blurWeights[i];
            }
            fragColor = vec4(sum, 1.0);
        }
    """.trimIndent()

    
    val SOFT_LIGHT_PREVIEW_BLUR_H = """
        #version 300 es
        precision highp float;
        in vec2 vTexCoord;
        out vec4 fragColor;
        uniform sampler2D uInputTexture;
        uniform vec2 uTexelSize;
        void main() {
            vec3 sum = texture(uInputTexture, vTexCoord).rgb * 0.204164;
            float blurOffsets[4] = float[](1.407333, 3.294215, 5.176470, 7.058823);
            float blurWeights[4] = float[](0.304005, 0.093910, 0.010416, 0.000005);
            for (int i = 0; i < 4; i++) {
                float off = blurOffsets[i] * uTexelSize.x * 2.8;
                sum += texture(uInputTexture, vTexCoord + vec2(off, 0.0)).rgb * blurWeights[i];
                sum += texture(uInputTexture, vTexCoord - vec2(off, 0.0)).rgb * blurWeights[i];
            }
            fragColor = vec4(sum, 1.0);
        }
    """.trimIndent()

    
    val HDF_PREVIEW_COMPOSITE = """
        #version 300 es
        precision highp float;
        in vec2 vTexCoord;
        out vec4 fragColor;
        uniform sampler2D uOriginalTexture;
        uniform sampler2D uBloomTexture;
        uniform float uHalation;
        uniform sampler2D uRedHalationTexture;
        uniform float uRedHalation;
        uniform sampler2D uSoftLightTexture;
        uniform float uSoftLight;
        
        void main() {
            vec4 color = texture(uOriginalTexture, vTexCoord);
            
            if (uSoftLight > 0.0) {
                vec3 softBlur = texture(uSoftLightTexture, vTexCoord).rgb;
                vec3 screen = vec3(1.0) - (vec3(1.0) - color.rgb) * (vec3(1.0) - softBlur);
                vec3 softGlow = mix(color.rgb, screen, 0.42);
                color.rgb = mix(color.rgb, softGlow, uSoftLight * 0.75);
                float softLuma = dot(softBlur, vec3(0.2126, 0.7152, 0.0722));
                color.rgb += vec3(softLuma) * (uSoftLight * 0.025);
                color.rgb = (color.rgb - 0.5) * (1.0 - uSoftLight * 0.05) + 0.5;
            }
            
            if (uHalation > 0.0) {
                vec3 bloom = texture(uBloomTexture, vTexCoord).rgb;
                float bLuma = dot(bloom, vec3(0.2126, 0.7152, 0.0722));
                bloom = mix(vec3(bLuma), bloom, 1.6);
                vec3 bloomEffect = bloom * uHalation * 1.4;
                color.rgb = vec3(1.0) - (vec3(1.0) - color.rgb) * (vec3(1.0) - bloomEffect);
                float mist = bLuma * uHalation * 0.15;
                color.rgb += mist;
                color.rgb = (color.rgb - 0.5) * (1.0 - uHalation * 0.08) + 0.5;
            }
            
            if (uRedHalation > 0.0) {
                vec3 halationBlur = texture(uRedHalationTexture, vTexCoord).rgb;
                float halationMask = smoothstep(0.001, 0.06, dot(halationBlur, vec3(0.2126, 0.7152, 0.0722)));
                vec3 halationStrength = vec3(0.42, 0.14, 0.02) * uRedHalation;
                color.rgb += halationBlur * halationStrength * halationMask;
            }
            
            fragColor = clamp(color, 0.0, 1.0);
        }
    """.trimIndent()

    
    val BEVY_BLOOM_DOWNSAMPLE_FIRST = """
        #version 300 es
        precision highp float;
        in vec2 vTexCoord;
        out vec4 fragColor;
        uniform sampler2D uInputTexture;
        uniform vec2 uInputTexelSize;
        uniform vec4 uThreshold;

        float tonemappingLuminance(vec3 v) {
            return dot(v, vec3(0.2126, 0.7152, 0.0722));
        }

        float karisAverage(vec3 color) {
            float luma = tonemappingLuminance(pow(max(color, vec3(0.0)), vec3(1.0 / 2.2))) / 4.0;
            return 1.0 / (1.0 + luma);
        }

        vec3 thresholdHighlight(vec3 color) {
            float luma = tonemappingLuminance(color);
            float mask = 0.0;
            if (uThreshold.z > 0.0) {
                mask = smoothstep(uThreshold.y, uThreshold.y + uThreshold.z, luma);
            } else {
                mask = step(uThreshold.x, luma);
            }
            return color * mask;
        }

        vec3 sampleInput(vec2 uv) {
            vec3 color = texture(uInputTexture, uv).rgb;
            if (uThreshold.x > 0.0 || uThreshold.z > 0.0) {
                color = thresholdHighlight(color);
            }
            return color;
        }

        vec3 sample13Tap(vec2 uv) {
            vec2 ps = uInputTexelSize;
            vec2 pl = 2.0 * ps;
            vec2 ns = -ps;
            vec2 nl = -pl;
            vec3 a = sampleInput(uv + vec2(nl.x, pl.y));
            vec3 b = sampleInput(uv + vec2(0.0, pl.y));
            vec3 c = sampleInput(uv + vec2(pl.x, pl.y));
            vec3 d = sampleInput(uv + vec2(nl.x, 0.0));
            vec3 e = sampleInput(uv);
            vec3 f = sampleInput(uv + vec2(pl.x, 0.0));
            vec3 g = sampleInput(uv + vec2(nl.x, nl.y));
            vec3 h = sampleInput(uv + vec2(0.0, nl.y));
            vec3 i = sampleInput(uv + vec2(pl.x, nl.y));
            vec3 j = sampleInput(uv + vec2(ns.x, ps.y));
            vec3 k = sampleInput(uv + vec2(ps.x, ps.y));
            vec3 l = sampleInput(uv + vec2(ns.x, ns.y));
            vec3 m = sampleInput(uv + vec2(ps.x, ns.y));

            vec3 group0 = (a + b + d + e) * (0.125 / 4.0);
            vec3 group1 = (b + c + e + f) * (0.125 / 4.0);
            vec3 group2 = (d + e + g + h) * (0.125 / 4.0);
            vec3 group3 = (e + f + h + i) * (0.125 / 4.0);
            vec3 group4 = (j + k + l + m) * (0.5 / 4.0);
            group0 *= karisAverage(group0);
            group1 *= karisAverage(group1);
            group2 *= karisAverage(group2);
            group3 *= karisAverage(group3);
            group4 *= karisAverage(group4);
            return group0 + group1 + group2 + group3 + group4;
        }

        void main() {
            vec3 sampleColor = sample13Tap(vTexCoord);
            fragColor = vec4(clamp(sampleColor, vec3(0.0), vec3(1.0)), 1.0);
        }
    """.trimIndent()

    
    val BEVY_BLOOM_DOWNSAMPLE = """
        #version 300 es
        precision highp float;
        in vec2 vTexCoord;
        out vec4 fragColor;
        uniform sampler2D uInputTexture;
        uniform vec2 uInputTexelSize;

        vec3 sample13Tap(vec2 uv) {
            vec2 ps = uInputTexelSize;
            vec2 pl = 2.0 * ps;
            vec2 ns = -ps;
            vec2 nl = -pl;
            vec3 a = texture(uInputTexture, uv + vec2(nl.x, pl.y)).rgb;
            vec3 b = texture(uInputTexture, uv + vec2(0.0, pl.y)).rgb;
            vec3 c = texture(uInputTexture, uv + vec2(pl.x, pl.y)).rgb;
            vec3 d = texture(uInputTexture, uv + vec2(nl.x, 0.0)).rgb;
            vec3 e = texture(uInputTexture, uv).rgb;
            vec3 f = texture(uInputTexture, uv + vec2(pl.x, 0.0)).rgb;
            vec3 g = texture(uInputTexture, uv + vec2(nl.x, nl.y)).rgb;
            vec3 h = texture(uInputTexture, uv + vec2(0.0, nl.y)).rgb;
            vec3 i = texture(uInputTexture, uv + vec2(pl.x, nl.y)).rgb;
            vec3 j = texture(uInputTexture, uv + vec2(ns.x, ps.y)).rgb;
            vec3 k = texture(uInputTexture, uv + vec2(ps.x, ps.y)).rgb;
            vec3 l = texture(uInputTexture, uv + vec2(ns.x, ns.y)).rgb;
            vec3 m = texture(uInputTexture, uv + vec2(ps.x, ns.y)).rgb;
            vec3 sampleColor = (a + c + g + i) * 0.03125;
            sampleColor += (b + d + f + h) * 0.0625;
            sampleColor += (e + j + k + l + m) * 0.125;
            return sampleColor;
        }

        void main() {
            fragColor = vec4(sample13Tap(vTexCoord), 1.0);
        }
    """.trimIndent()

    
    val BEVY_BLOOM_UPSAMPLE = """
        #version 300 es
        precision highp float;
        in vec2 vTexCoord;
        out vec4 fragColor;
        uniform sampler2D uInputTexture;
        uniform vec2 uInputTexelSize;

        void main() {
            float x = uInputTexelSize.x;
            float y = uInputTexelSize.y;
            vec2 uv = vTexCoord;
            vec3 a = texture(uInputTexture, vec2(uv.x - x, uv.y + y)).rgb;
            vec3 b = texture(uInputTexture, vec2(uv.x, uv.y + y)).rgb;
            vec3 c = texture(uInputTexture, vec2(uv.x + x, uv.y + y)).rgb;
            vec3 d = texture(uInputTexture, vec2(uv.x - x, uv.y)).rgb;
            vec3 e = texture(uInputTexture, vec2(uv.x, uv.y)).rgb;
            vec3 f = texture(uInputTexture, vec2(uv.x + x, uv.y)).rgb;
            vec3 g = texture(uInputTexture, vec2(uv.x - x, uv.y - y)).rgb;
            vec3 h = texture(uInputTexture, vec2(uv.x, uv.y - y)).rgb;
            vec3 i = texture(uInputTexture, vec2(uv.x + x, uv.y - y)).rgb;
            vec3 sampleColor = e * 0.25;
            sampleColor += (b + d + f + h) * 0.125;
            sampleColor += (a + c + g + i) * 0.0625;
            fragColor = vec4(sampleColor, 1.0);
        }
    """.trimIndent()

    
    val BEVY_BLOOM_COMPOSITE = """
        #version 300 es
        precision highp float;
        in vec2 vTexCoord;
        out vec4 fragColor;
        uniform sampler2D uBloomTexture;
        uniform sampler2D uBloomTextureNext;
        uniform vec2 uBloomTexelSize;
        uniform vec2 uBloomTexelSizeNext;
        uniform float uBlend;
        uniform float uMipBlend;

        vec3 sampleTentLower(vec2 uv) {
            float x = uBloomTexelSize.x;
            float y = uBloomTexelSize.y;
            vec3 a = texture(uBloomTexture, vec2(uv.x - x, uv.y + y)).rgb;
            vec3 b = texture(uBloomTexture, vec2(uv.x, uv.y + y)).rgb;
            vec3 c = texture(uBloomTexture, vec2(uv.x + x, uv.y + y)).rgb;
            vec3 d = texture(uBloomTexture, vec2(uv.x - x, uv.y)).rgb;
            vec3 e = texture(uBloomTexture, vec2(uv.x, uv.y)).rgb;
            vec3 f = texture(uBloomTexture, vec2(uv.x + x, uv.y)).rgb;
            vec3 g = texture(uBloomTexture, vec2(uv.x - x, uv.y - y)).rgb;
            vec3 h = texture(uBloomTexture, vec2(uv.x, uv.y - y)).rgb;
            vec3 i = texture(uBloomTexture, vec2(uv.x + x, uv.y - y)).rgb;
            vec3 sampleColor = e * 0.25;
            sampleColor += (b + d + f + h) * 0.125;
            sampleColor += (a + c + g + i) * 0.0625;
            return sampleColor;
        }

        vec3 sampleTentUpper(vec2 uv) {
            float x = uBloomTexelSizeNext.x;
            float y = uBloomTexelSizeNext.y;
            vec3 a = texture(uBloomTextureNext, vec2(uv.x - x, uv.y + y)).rgb;
            vec3 b = texture(uBloomTextureNext, vec2(uv.x, uv.y + y)).rgb;
            vec3 c = texture(uBloomTextureNext, vec2(uv.x + x, uv.y + y)).rgb;
            vec3 d = texture(uBloomTextureNext, vec2(uv.x - x, uv.y)).rgb;
            vec3 e = texture(uBloomTextureNext, vec2(uv.x, uv.y)).rgb;
            vec3 f = texture(uBloomTextureNext, vec2(uv.x + x, uv.y)).rgb;
            vec3 g = texture(uBloomTextureNext, vec2(uv.x - x, uv.y - y)).rgb;
            vec3 h = texture(uBloomTextureNext, vec2(uv.x, uv.y - y)).rgb;
            vec3 i = texture(uBloomTextureNext, vec2(uv.x + x, uv.y - y)).rgb;
            vec3 sampleColor = e * 0.25;
            sampleColor += (b + d + f + h) * 0.125;
            sampleColor += (a + c + g + i) * 0.0625;
            return sampleColor;
        }

        void main() {
            vec3 lowerBloom = sampleTentLower(vTexCoord);
            vec3 upperBloom = sampleTentUpper(vTexCoord);
            vec3 bloom = mix(lowerBloom, upperBloom, uMipBlend) * uBlend;
            fragColor = vec4(clamp(bloom, 0.0, 1.0), 1.0);
        }
    """.trimIndent()

    
    val HALATION_PREVIEW_EXTRACT_BLUR_H = """
        #version 300 es
        precision highp float;
        in vec2 vTexCoord;
        out vec4 fragColor;
        uniform sampler2D uInputTexture;
        uniform vec2 uTexelSize;
        uniform float uThreshold;
        uniform float uStrength;
        void main() {
            vec3 tint = vec3(1.0, 0.28, 0.04);
            
            #define EXTRACT(sampleColor) \
                (max(sampleColor - vec3(uThreshold), vec3(0.0)) * tint * (1.5 + uStrength * 3.0) * smoothstep(uThreshold - 0.24, uThreshold + 0.36, max(sampleColor.r, max(sampleColor.g, sampleColor.b))))

            vec3 color = texture(uInputTexture, vTexCoord).rgb;
            vec3 sum = EXTRACT(color) * 0.204164;
            
            float blurOffsets[4] = float[](1.407333, 3.294215, 5.176470, 7.058823);
            float blurWeights[4] = float[](0.304005, 0.093910, 0.010416, 0.000005);
            for (int i = 0; i < 4; i++) {
                float off = blurOffsets[i] * uTexelSize.x * 2.0;
                sum += EXTRACT(texture(uInputTexture, vTexCoord + vec2(off, 0.0)).rgb) * blurWeights[i];
                sum += EXTRACT(texture(uInputTexture, vTexCoord - vec2(off, 0.0)).rgb) * blurWeights[i];
            }
            fragColor = vec4(sum, 1.0);
        }
    """.trimIndent()

    
    val HALATION_PREVIEW_BLUR_V = HDF_PREVIEW_BLUR_V

    
    val FRAGMENT_SHADER_FOCUS_PEAKING = """
        #version 300 es
        precision highp float;
        in vec2 vTexCoord;
        out vec4 fragColor;
        uniform sampler2D uInputTexture;
        uniform vec2 uTexelSize;
        uniform float uThreshold;
        uniform vec3 uPeakColor;

        void main() {
            vec4 color = texture(uInputTexture, vTexCoord);

            float l00 = dot(texture(uInputTexture, vTexCoord + vec2(-uTexelSize.x, -uTexelSize.y)).rgb, vec3(0.299, 0.587, 0.114));
            float l10 = dot(texture(uInputTexture, vTexCoord + vec2(0.0, -uTexelSize.y)).rgb, vec3(0.299, 0.587, 0.114));
            float l20 = dot(texture(uInputTexture, vTexCoord + vec2(uTexelSize.x, -uTexelSize.y)).rgb, vec3(0.299, 0.587, 0.114));
            float l01 = dot(texture(uInputTexture, vTexCoord + vec2(-uTexelSize.x, 0.0)).rgb, vec3(0.299, 0.587, 0.114));
            float l21 = dot(texture(uInputTexture, vTexCoord + vec2(uTexelSize.x, 0.0)).rgb, vec3(0.299, 0.587, 0.114));
            float l02 = dot(texture(uInputTexture, vTexCoord + vec2(-uTexelSize.x, uTexelSize.y)).rgb, vec3(0.299, 0.587, 0.114));
            float l12 = dot(texture(uInputTexture, vTexCoord + vec2(0.0, uTexelSize.y)).rgb, vec3(0.299, 0.587, 0.114));
            float l22 = dot(texture(uInputTexture, vTexCoord + vec2(uTexelSize.x, uTexelSize.y)).rgb, vec3(0.299, 0.587, 0.114));

            float gx = l00 + 2.0 * l01 + l02 - l20 - 2.0 * l21 - l22;
            float gy = l00 + 2.0 * l10 + l20 - l02 - 2.0 * l12 - l22;
            float edge = sqrt(gx * gx + gy * gy);
            float peakFactor = smoothstep(uThreshold, uThreshold * 1.5, edge);
            fragColor = vec4(mix(color.rgb, uPeakColor, peakFactor * 0.9), color.a);
        }
    """.trimIndent()

    
    val FULL_QUAD_VERTICES = floatArrayOf(
        
        -1.0f, -1.0f,  
        1.0f, -1.0f,  
        -1.0f, 1.0f,  
        1.0f, 1.0f   
    )

    
    val TEXTURE_COORDS = floatArrayOf(
        
        0.0f, 0.0f,  
        1.0f, 0.0f,  
        0.0f, 1.0f,  
        1.0f, 1.0f   
    )

    
    val POST_PROCESS_TEXTURE_COORDS = floatArrayOf(
        0.0f, 1.0f, 
        1.0f, 1.0f, 
        0.0f, 0.0f, 
        1.0f, 0.0f  
    )

    
    val DRAW_ORDER = shortArrayOf(
        0, 1, 2,  
        1, 3, 2   
    )

    
    val JBU_UPSAMPLE_FRAGMENT_SHADER = """#version 300 es
        precision highp float;
        in vec2 vTexCoord;
        out vec4 fragColor;

        uniform sampler2D uLowResDepth;  
        uniform sampler2D uHighResGuide; 
        uniform vec2 uLowResTexelSize;   

        const float SIGMA_S = 0.85;
        const float SIGMA_R = 0.12;

        void main() {
            vec3 guideColor = texture(uHighResGuide, vTexCoord).rgb;
            

            float baseDepth = texture(uLowResDepth, vTexCoord).r;
            
            ivec2 lowResSize = textureSize(uLowResDepth, 0);
            vec2 pos = vTexCoord / uLowResTexelSize - 0.5;
            ivec2 p0 = ivec2(floor(pos));
            vec2 f = fract(pos);
            
            float totalWeight = 0.0;
            float totalDepth = 0.0;

            for (int y = -1; y <= 2; y++) {
                for (int x = -1; x <= 2; x++) {
                    ivec2 sampleIndex = p0 + ivec2(x, y);
                    if (any(lessThan(sampleIndex, ivec2(0))) ||
                        any(greaterThanEqual(sampleIndex, lowResSize))) {
                        continue;
                    }

                    vec2 sampleCoord = (vec2(sampleIndex) + 0.5) * uLowResTexelSize;
                    float d = texelFetch(uLowResDepth, sampleIndex, 0).r;
                    vec3 c = textureGrad(
                        uHighResGuide,
                        sampleCoord,
                        vec2(uLowResTexelSize.x, 0.0),
                        vec2(0.0, uLowResTexelSize.y)
                    ).rgb;

                    vec2 delta = vec2(float(x), float(y)) - f;
                    float wS = exp(
                        -dot(delta, delta) / (2.0 * SIGMA_S * SIGMA_S)
                    );
                    float dC = distance(guideColor, c);
                    float wC = exp(-(dC * dC) / (2.0 * SIGMA_R * SIGMA_R));

                    float w = wS * wC;
                    totalDepth += d * w;
                    totalWeight += w;
                }
            }

            float finalDepth = totalWeight > 0.001 ? totalDepth / totalWeight : baseDepth;
            fragColor = vec4(vec3(finalDepth), 1.0);
        }
    """.trimIndent()

    
    val DEPTH_REFINE_FRAGMENT_SHADER = """#version 300 es
        precision highp float;
        in vec2 vTexCoord;
        out vec4 fragColor;

        uniform sampler2D uDepthTexture;
        uniform vec2 uTexelSize;

        void main() {
            float center = texture(uDepthTexture, vTexCoord).r;
            float n = texture(uDepthTexture, vTexCoord + vec2(0, uTexelSize.y)).r;
            float s = texture(uDepthTexture, vTexCoord - vec2(0, uTexelSize.y)).r;
            float e = texture(uDepthTexture, vTexCoord + vec2(uTexelSize.x, 0)).r;
            float w = texture(uDepthTexture, vTexCoord - vec2(uTexelSize.x, 0)).r;
            float ne = texture(uDepthTexture, vTexCoord + uTexelSize).r;
            float nw = texture(
                uDepthTexture,
                vTexCoord + vec2(-uTexelSize.x, uTexelSize.y)
            ).r;
            float se = texture(
                uDepthTexture,
                vTexCoord + vec2(uTexelSize.x, -uTexelSize.y)
            ).r;
            float sw = texture(uDepthTexture, vTexCoord - uTexelSize).r;

            float blurred = (
                center * 4.0 +
                (n + s + e + w) * 2.0 +
                ne + nw + se + sw
            ) * (1.0 / 16.0);
            float localMin = min(
                min(min(center, n), min(s, e)),
                min(min(w, ne), min(min(nw, se), sw))
            );
            float localMax = max(
                max(max(center, n), max(s, e)),
                max(max(w, ne), max(max(nw, se), sw))
            );
            float edgeGate = smoothstep(0.004, 0.035, localMax - localMin);
            float refined = clamp(
                center + (center - blurred) * (0.65 * edgeGate),
                localMin,
                localMax
            );
            fragColor = vec4(vec3(clamp(refined, 0.0, 1.0)), 1.0);
        }
    """.trimIndent()

    
    val DEPTH_READBACK_FRAGMENT_SHADER = """#version 300 es
        precision highp float;
        in vec2 vTexCoord;
        out vec4 fragColor;

        uniform sampler2D uDepthTexture;

        void main() {
            float depth = texture(uDepthTexture, vTexCoord).r;
            fragColor = vec4(vec3(clamp(depth, 0.0, 1.0)), 1.0);
        }
    """.trimIndent()

    
    val COMPACT_BOKEH_HIGHLIGHT_FRAGMENT_SHADER = """
        #version 300 es
        precision highp float;

        in vec2 vTexCoord;
        out vec4 fragColor;

        uniform sampler2D uInputTexture;
        uniform sampler2D uDepthTexture;
        uniform mat4 uDepthMatrix;
        uniform float uMaxBlurRadius;
        uniform float uAperture;
        uniform float uFocusDepth;
        uniform vec2 uTexelSize;
        uniform float uMinNeighborhoodLumaDifference;
        uniform int uLinearInput;

        const float LENS_GAMMA = 2.2;
        const float MIN_HIGHLIGHT_CORE_RADIUS_PIXELS = 2.5;
        const float MIN_HIGHLIGHT_CORE_DIRECTION_COUNT = 12.0;
        const vec2 PROBE_DIRECTIONS[16] = vec2[](
            vec2( 1.0,  0.0),
            vec2( 0.92387953,  0.38268343),
            vec2( 0.70710678,  0.70710678),
            vec2( 0.38268343,  0.92387953),
            vec2( 0.0,  1.0),
            vec2(-0.38268343,  0.92387953),
            vec2(-0.70710678,  0.70710678),
            vec2(-0.92387953,  0.38268343),
            vec2(-1.0,  0.0),
            vec2(-0.92387953, -0.38268343),
            vec2(-0.70710678, -0.70710678),
            vec2(-0.38268343, -0.92387953),
            vec2( 0.0, -1.0),
            vec2( 0.38268343, -0.92387953),
            vec2( 0.70710678, -0.70710678),
            vec2( 0.92387953, -0.38268343)
        );

        vec3 toLinear(vec3 color) {
            if (uLinearInput != 0) return max(color, vec3(0.0));
            return pow(clamp(color, 0.0, 1.0), vec3(LENS_GAMMA));
        }

        float luminance(vec3 color) {
            return dot(color, vec3(0.2126, 0.7152, 0.0722));
        }

        float computeCoc(float depth) {
            float gap = max(uFocusDepth - depth - 0.015, 0.0);
            float defocus = pow(gap, 1.1);
            return clamp(
                defocus * uMaxBlurRadius * (1.0 / max(uAperture, 0.45)),
                0.0,
                uMaxBlurRadius
            );
        }

        int evaluateDarkRing(
            vec2 centerUV,
            float centerLuma,
            float ringProbeRadius,
            out vec3 surroundLinear,
            out float maxRingLuma,
            out vec2 ringBrightnessMoment
        ) {
            surroundLinear = vec3(0.0);
            maxRingLuma = 0.0;
            ringBrightnessMoment = vec2(0.0);

            vec2 ringUvExtent = ringProbeRadius * uTexelSize;
            if (any(lessThan(centerUV - ringUvExtent, vec2(0.0))) ||
                any(greaterThan(centerUV + ringUvExtent, vec2(1.0)))) {

                return 0;
            }

            bool allRingSamplesAreDarker = true;
            for (int i = 0; i < 16; i++) {
                vec2 ringUV = centerUV +
                    PROBE_DIRECTIONS[i] * ringProbeRadius * uTexelSize;
                vec3 ringLinear = toLinear(
                    textureLod(uInputTexture, ringUV, 0.0).rgb
                );
                float ringLuma = luminance(ringLinear);

                if (ringLuma > centerLuma) return -1;
                if (ringLuma >= centerLuma) {
                    allRingSamplesAreDarker = false;
                }

                surroundLinear += ringLinear;
                maxRingLuma = max(maxRingLuma, ringLuma);
                ringBrightnessMoment += PROBE_DIRECTIONS[i] * ringLuma;
            }
            surroundLinear *= 1.0 / 16.0;
            return allRingSamplesAreDarker ? 1 : 0;
        }

        void main() {
            vec2 depthUV = clamp(
                (uDepthMatrix * vec4(vTexCoord, 0.0, 1.0)).xy,
                0.0,
                1.0
            );
            float coc = computeCoc(texture(uDepthTexture, depthUV).r);
            if (coc < 1.5) {
                fragColor = vec4(0.0);
                return;
            }

            vec3 centerLinear = toLinear(textureLod(uInputTexture, vTexCoord, 0.0).rgb);
            float centerLuma = luminance(centerLinear);
            if (centerLuma <= 0.24) {
                fragColor = vec4(0.0);
                return;
            }

            vec3 surroundLinear = vec3(0.0);
            float maxRingLuma = 0.0;
            vec2 ringBrightnessMoment = vec2(0.0);

            float nearRingRadius = clamp(coc * 0.70, 5.0, 32.0);
            float middleRingRadius = clamp(
                max(coc * 1.40, uMaxBlurRadius * 0.40),
                10.0,
                64.0
            );
            float farRingRadius = clamp(
                max(coc * 2.40, uMaxBlurRadius * 0.80),
                16.0,
                128.0
            );

            int ringResult = evaluateDarkRing(
                vTexCoord,
                centerLuma,
                nearRingRadius,
                surroundLinear,
                maxRingLuma,
                ringBrightnessMoment
            );
            if (ringResult == 0) {
                ringResult = evaluateDarkRing(
                    vTexCoord,
                    centerLuma,
                    middleRingRadius,
                    surroundLinear,
                    maxRingLuma,
                    ringBrightnessMoment
                );
            }
            if (ringResult == 0) {
                ringResult = evaluateDarkRing(
                    vTexCoord,
                    centerLuma,
                    farRingRadius,
                    surroundLinear,
                    maxRingLuma,
                    ringBrightnessMoment
                );
            }
            if (ringResult != 1) {
                fragColor = vec4(0.0);
                return;
            }

            float surroundLuma = luminance(surroundLinear);
            float coreBrightnessThreshold = mix(
                surroundLuma,
                centerLuma,
                0.35
            );
            float brightCoreSampleCount = 0.0;
            for (int i = 0; i < 16; i++) {
                vec2 coreUV = vTexCoord + PROBE_DIRECTIONS[i]
                    * MIN_HIGHLIGHT_CORE_RADIUS_PIXELS * uTexelSize;
                float coreLuma = luminance(toLinear(
                    textureLod(uInputTexture, coreUV, 0.0).rgb
                ));
                if (coreLuma >= coreBrightnessThreshold) {
                    brightCoreSampleCount += 1.0;
                }
            }
            if (brightCoreSampleCount < MIN_HIGHLIGHT_CORE_DIRECTION_COUNT) {
                fragColor = vec4(0.0);
                return;
            }

            float contrast = max(centerLuma - surroundLuma, 0.0);
            float relativeContrast = contrast / max(centerLuma, 0.06);
            float neighborhoodContrastGate = smoothstep(
                uMinNeighborhoodLumaDifference,
                uMinNeighborhoodLumaDifference + 0.04,
                contrast
            );

            float mediumHighlightGate = smoothstep(0.18, 0.50, centerLuma)
                * max(
                    smoothstep(0.06, 0.18, contrast),
                    smoothstep(0.18, 0.38, relativeContrast)
                );

            float strongPointGate = smoothstep(0.65, 0.90, centerLuma)
                * max(
                    smoothstep(0.04, 0.14, contrast),
                    smoothstep(0.12, 0.30, relativeContrast)
                );

            float peakDominance = centerLuma - maxRingLuma;
            float localMaximumGate = smoothstep(0.0, 0.05, peakDominance);
            float normalizedMoment = length(ringBrightnessMoment / 16.0)
                / max(centerLuma, 0.06);
            float centerednessGate = 1.0 - smoothstep(
                0.05,
                0.2,
                normalizedMoment
            );

            float highlightGate = max(
                mediumHighlightGate,
                strongPointGate
            );

            float pointShapeGate = localMaximumGate
                * mix(0.35, 1.0, centerednessGate);
            float compactHighlight = highlightGate
                * pointShapeGate
                * neighborhoodContrastGate;

            vec3 residual = max(centerLinear - surroundLinear, vec3(0.0));

            vec3 sourceSignal = mix(residual, centerLinear, 0.35);
            fragColor = vec4(sourceSignal * compactHighlight, compactHighlight);
        }
    """.trimIndent()

    
    val PSF_SPLAT_FRAGMENT_SHADER = """
        #version 300 es
        precision highp float;

        in vec2 vTexCoord;
        out vec4 fragColor;

        uniform sampler2D uInputTexture;
        uniform sampler2D uDepthTexture;

        uniform mat4 uDepthMatrix;
        uniform float uMaxBlurRadius;
        uniform float uAperture;
        uniform float uFocusDepth;
        uniform vec2 uTexelSize;
        uniform int uLinearInput;

        const float GOLDEN_ANGLE = 2.39996323;
        const int SAMPLES = 640;
        const float LENS_GAMMA = 2.2;

        float computeCoc(float depth) {
            float gap = max(uFocusDepth - depth - 0.015, 0.0);
            float defocus = pow(gap, 1.1);
            return clamp(defocus * uMaxBlurRadius * (1.0 / max(uAperture, 0.45)), 0.0, uMaxBlurRadius);
        }

        float apertureWeight(vec2 offsetPixels, float coc) {
            vec2 p = offsetPixels / max(coc, 0.001);
            float lenP = length(p);

            float support = 1.0 - smoothstep(0.86, 1.0, lenP);
            float radialTransmission = mix(
                1.0,
                0.90,
                smoothstep(0.0, 0.86, lenP)
            );
            float rim = smoothstep(0.70, 0.82, lenP)
                * (1.0 - smoothstep(0.88, 0.97, lenP));
            return support * radialTransmission * (1.0 + rim * 0.10);
        }

        vec3 toLinear(vec3 color) {
            if (uLinearInput != 0) return max(color, vec3(0.0));
            return pow(clamp(color, 0.0, 1.0), vec3(LENS_GAMMA));
        }

        vec3 toDisplay(vec3 color) {
            if (uLinearInput != 0) return max(color, vec3(0.0));
            return pow(max(color, vec3(0.0)), vec3(1.0 / LENS_GAMMA));
        }

        void main() {
            vec2 depthUV = clamp((uDepthMatrix * vec4(vTexCoord, 0.0, 1.0)).xy, 0.0, 1.0);
            vec4 centerColor = texture(uInputTexture, vTexCoord);
            float centerDepth = texture(uDepthTexture, depthUV).r;

            float centerCoc = computeCoc(centerDepth);

            if (centerCoc < 0.2) {
                fragColor = centerColor;
                return;
            }

            const float rotation = 0.0;

            float centerWeight = 4.0 / (centerCoc * 0.3 + 1.0);
            float sampleFootprintUv = uMaxBlurRadius
                * 1.8
                * uTexelSize.x
                / sqrt(float(SAMPLES));
            float inputIntegrationLod = max(
                0.0,
                log2(sampleFootprintUv * float(textureSize(uInputTexture, 0).x))
            );
            vec3 centerLinear = toLinear(centerColor.rgb);
            vec3 accColor = centerLinear * centerWeight;
            float accWeight = centerWeight;

            float softBase = max(2.5, uMaxBlurRadius * 0.08);

            for (int i = 0; i < SAMPLES; i++) {
                float f = float(i + 1);
                float r = sqrt(f / float(SAMPLES)) * uMaxBlurRadius;
                float theta = f * GOLDEN_ANGLE + rotation;

                vec2 offset = vec2(cos(theta), sin(theta)) * r * uTexelSize;
                vec2 sampleUV = clamp(vTexCoord + offset, 0.0, 1.0);
                vec2 offsetPixels = offset / uTexelSize;

                vec3 sColor = textureLod(
                    uInputTexture,
                    sampleUV,
                    inputIntegrationLod
                ).rgb;
                vec2 sDepthUV = clamp((uDepthMatrix * vec4(sampleUV, 0.0, 1.0)).xy, 0.0, 1.0);
                float sDepth = texture(uDepthTexture, sDepthUV).r;

                float sCoc = computeCoc(sDepth);

                float fW = smoothstep(r - softBase, r + softBase * 0.5, sCoc);
                float bW = smoothstep(r - softBase, r + softBase * 0.5, centerCoc);

                float sourceIsNearer = smoothstep(
                    0.025,
                    0.075,
                    sDepth - centerDepth
                );
                float centerOccludesSource = smoothstep(
                    0.025,
                    0.075,
                    centerDepth - sDepth
                );
                float sourceVisibility = 1.0 - centerOccludesSource;

                float weight = mix(bW, fW, sourceIsNearer);
                weight *= apertureWeight(offsetPixels, max(sCoc, centerCoc));
                weight *= sourceVisibility;

                if (weight > 0.0001) {
                    vec3 sLinear = toLinear(sColor);
                    accColor += sLinear * weight;
                    accWeight += weight;
                }
            }

            vec3 finalLinear = accWeight > 0.001
                ? accColor / accWeight
                : toLinear(centerColor.rgb);

            vec3 finalColor = toDisplay(finalLinear);
            if (uLinearInput == 0) {
                finalColor = clamp(finalColor, 0.0, 1.0);
            }

            fragColor = vec4(finalColor, centerColor.a);
        }
    """.trimIndent()

    
    val ANALYTIC_BOKEH_HIGHLIGHT_VERTEX_SHADER = """
        #version 300 es
        precision highp float;

        in vec2 aPosition;
        in vec2 aCenterUv;
        in float aCocPixels;
        in vec3 aSignal;

        uniform vec2 uImageSize;

        out vec2 vOffsetPixels;
        flat out float vCocPixels;
        flat out vec3 vSignal;

        void main() {
            vec2 offsetPixels = aPosition * aCocPixels;
            vec2 centerNdc = aCenterUv * 2.0 - 1.0;
            vec2 offsetNdc = offsetPixels * 2.0 / uImageSize;
            gl_Position = vec4(centerNdc + offsetNdc, 0.0, 1.0);
            vOffsetPixels = offsetPixels;
            vCocPixels = aCocPixels;
            vSignal = aSignal;
        }
    """.trimIndent()

    val ANALYTIC_BOKEH_HIGHLIGHT_FRAGMENT_SHADER = """
        #version 300 es
        precision highp float;

        in vec2 vOffsetPixels;
        flat in float vCocPixels;
        flat in vec3 vSignal;
        out vec4 fragColor;

        uniform int uLinearInput;

        float apertureTransmission(float normalizedDistance) {
            float support = 1.0 - smoothstep(0.86, 1.0, normalizedDistance);
            float radialTransmission = mix(
                1.0,
                0.90,
                smoothstep(0.0, 0.86, normalizedDistance)
            );
            float rim = smoothstep(0.70, 0.82, normalizedDistance)
                * (1.0 - smoothstep(0.88, 0.97, normalizedDistance));
            return support * radialTransmission * (1.0 + rim * 0.10);
        }

        void main() {
            float normalizedDistance = length(vOffsetPixels)
                / max(vCocPixels, 0.001);
            if (normalizedDistance >= 1.0) discard;

            float transmission = apertureTransmission(normalizedDistance);
            if (uLinearInput != 0) {

                fragColor = vec4(vSignal * transmission, 0.0);
            } else {

                vec3 reconstructedHighlight = vSignal * (0.72 * transmission);
                vec3 compressedHighlight = reconstructedHighlight
                    / (vec3(1.0) + reconstructedHighlight * 2.0);
                vec3 highlightOpacity = min(
                    vec3(0.52),
                    vec3(1.0) - exp(-compressedHighlight * 2.8)
                );
                fragColor = vec4(highlightOpacity, 0.0);
            }
        }
    """.trimIndent()

    
    val BOKEH_COMPOSITE_FRAGMENT_SHADER = """
        #version 300 es
        precision highp float;

        in vec2 vTexCoord;
        out vec4 fragColor;

        uniform sampler2D uOriginalTexture;
        uniform sampler2D uBokehTexture;
        uniform sampler2D uHighlightTexture;
        uniform sampler2D uDepthTexture;
        uniform mat4 uDepthMatrix;
        uniform float uMaxBlurRadius;
        uniform float uAperture;
        uniform float uFocusDepth;
        uniform vec2 uDepthTexelSize;
        uniform int uLinearInput;

        float computeCoc(float depth) {
            float gap = max(uFocusDepth - depth - 0.015, 0.0);
            float defocus = pow(gap, 1.1);
            return clamp(
                defocus * uMaxBlurRadius * (1.0 / max(uAperture, 0.45)),
                0.0,
                uMaxBlurRadius
            );
        }

        float protectedForegroundDepth(vec2 depthUV) {
            float protectedDepth = 0.0;
            for (int y = -1; y <= 1; y++) {
                for (int x = -1; x <= 1; x++) {
                    vec2 sampleUV = clamp(
                        depthUV + vec2(float(x), float(y)) * uDepthTexelSize,
                        0.0,
                        1.0
                    );
                    protectedDepth = max(
                        protectedDepth,
                        texture(uDepthTexture, sampleUV).r
                    );
                }
            }
            return protectedDepth;
        }

        void main() {
            vec4 originalColor = texture(uOriginalTexture, vTexCoord);
            vec3 backgroundColor = texture(uBokehTexture, vTexCoord).rgb;
            vec3 highlightLayer = texture(uHighlightTexture, vTexCoord).rgb;
            vec2 depthUV = clamp(
                (uDepthMatrix * vec4(vTexCoord, 0.0, 1.0)).xy,
                0.0,
                1.0
            );
            float centerDepth = texture(uDepthTexture, depthUV).r;
            float protectedDepth = protectedForegroundDepth(depthUV);
            float coc = computeCoc(centerDepth);
            float backgroundMix = smoothstep(0.2, 1.2, coc);
            float foregroundOcclusion = smoothstep(
                uFocusDepth - 0.035,
                uFocusDepth - 0.015,
                protectedDepth
            );
            backgroundMix *= 1.0 - foregroundOcclusion;

            vec3 backgroundWithHighlights;
            if (uLinearInput != 0) {
                backgroundWithHighlights = backgroundColor + highlightLayer;
            } else {
                vec3 highlightOpacity = clamp(highlightLayer, 0.0, 1.0);
                backgroundWithHighlights = backgroundColor
                    + (vec3(1.0) - backgroundColor) * highlightOpacity;
            }
            fragColor = vec4(
                mix(originalColor.rgb, backgroundWithHighlights, backgroundMix),
                originalColor.a
            );
        }
    """.trimIndent()
}
