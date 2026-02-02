/* GPU fire simulation with WebGL-based fluid dynamics */
var TestFire = (function () {
    // Shorthand for element lookup by id.
    function $(id) { return document.getElementById(id); }

    // Entry point: builds the WebGL scene, UI, and animation loop.
    function start() {
        var content = $('content') || document.body;
        var canvas = document.createElement('canvas');
        canvas.style.position = 'fixed';
        canvas.style.top = '0';
        canvas.style.left = '0';
        canvas.style.width = '100%';
        canvas.style.height = '100%';
        canvas.style.zIndex = '-1';
        content.appendChild(canvas);

        var gl = canvas.getContext('webgl', { alpha: false, antialias: false, premultipliedAlpha: false });
        if (!gl) { console.error('WebGL not available'); return; }

        // Check if floating-point textures are supported.
        var floatTex = gl.getExtension('OES_texture_float');
        // Check if linear filtering on float textures is supported.
        var floatLinear = gl.getExtension('OES_texture_float_linear');
        // Check if float textures can be rendered to (float color buffers).
        var floatColor = gl.getExtension('WEBGL_color_buffer_float');

        // These decide whether the simulation can use float textures or fall back to 8-bit textures later.
        var textureType = gl.UNSIGNED_BYTE;
        if (floatTex && floatColor) { textureType = gl.FLOAT; }

        // Fullscreen quad vertex shader.
        var vs =
            'attribute vec2 position;\n' +
            'varying vec2 v_uv;\n' +
            'void main(){\n' +
            '    // Map from clip space [-1..1] to UV [0..1].\n' +
            '    v_uv = position * 0.5 + 0.5;\n' +
            '    gl_Position = vec4(position, 0.0, 1.0);\n' +
            '}\n';

        // Common shader header for all fragment shaders.
        var common =
            'precision highp float;\n' +
            'varying vec2 v_uv;\n';

        // Advect a scalar field (density) using the velocity field.
        var fs_advect = common +
            'uniform sampler2D uVelocity;\n' +
            'uniform sampler2D uSource;\n' +
            'uniform float dt;\n' +
            'uniform float dissipation;\n' +
            'uniform vec2 texelSize;\n' +
            'uniform float rdx;\n' +
            'uniform float velRange;\n' +
            '\n' +
            'vec2 decode(vec2 v, float r){\n' +
            // Decode velocity from 0..1 back to -r..r.\n'
            '    return v*r*2.0-r;\n' +
            '}\n' +
            '\n' +
            'void main(){\n' +
            '    vec2 uv = v_uv;\n' +
            '    vec2 vel = decode(texture2D(uVelocity, uv).xy, velRange);\n' +
            '    float scale = min(texelSize.x, texelSize.y) * rdx;\n' +
            '    vec2 prev = uv - dt * vel * scale;\n' +
            '    gl_FragColor = texture2D(uSource, prev) * dissipation;\n' +
            '}\n';

        // Advect the velocity field itself (semi-Lagrangian).
        var fs_advect_vel = common +
            'uniform sampler2D uVelocity;\n' +
            'uniform float dt;\n' +
            'uniform float dissipation;\n' +
            'uniform vec2 texelSize;\n' +
            'uniform float rdx;\n' +
            'uniform float velRange;\n' +
            '\n' +
            'vec2 decode(vec2 v, float r){ return v*r*2.0-r; }\n' +
            'vec2 encode(vec2 v, float r){ return clamp(v/r*0.5+0.5, 0.0, 1.0); }\n' +
            '\n' +
            'void main(){\n' +
            '    vec2 uv = v_uv;\n' +
            '    vec2 vel = decode(texture2D(uVelocity, uv).xy, velRange);\n' +
            '    float scale = min(texelSize.x, texelSize.y) * rdx;\n' +
            '    vec2 prev = uv - dt * vel * scale;\n' +
            '    vec2 sampled = decode(texture2D(uVelocity, prev).xy, velRange);\n' +
            '    sampled *= dissipation;\n' +
            '    gl_FragColor = vec4(encode(sampled, velRange), 0.0, 1.0);\n' +
            '}\n';
        // Compute velocity divergence for pressure solve.
        var fs_div = common +
            'uniform sampler2D uVelocity;\n' +
            'uniform vec2 texelSize;\n' +
            'uniform float velRange;\n' +
            '\n' +
            'vec2 decode(vec2 v, float r){ return v*r*2.0-r; }\n' +
            '\n' +
            'void main(){\n' +
            '    vec2 uv = v_uv;\n' +
            '    float vl = decode(texture2D(uVelocity, uv-vec2(texelSize.x,0.0)).xy, velRange).x;\n' +
            '    float vr = decode(texture2D(uVelocity, uv+vec2(texelSize.x,0.0)).xy, velRange).x;\n' +
            '    float vb = decode(texture2D(uVelocity, uv-vec2(0.0,texelSize.y)).xy, velRange).y;\n' +
            '    float vt = decode(texture2D(uVelocity, uv+vec2(0.0,texelSize.y)).xy, velRange).y;\n' +
            '    float d = 0.5*(vr - vl + vt - vb);\n' +
            '    gl_FragColor = vec4(d, 0.0, 0.0, 1.0);\n' +
            '}\n';

        // Jacobi iteration for pressure Poisson solve.
        var fs_jacobi = common +
            'uniform sampler2D uPressure;\n' +
            'uniform sampler2D uDivergence;\n' +
            'uniform vec2 texelSize;\n' +
            '\n' +
            'void main(){\n' +
            '    vec2 uv = v_uv;\n' +
            '    float pl = texture2D(uPressure, uv-vec2(texelSize.x,0.0)).x;\n' +
            '    float pr = texture2D(uPressure, uv+vec2(texelSize.x,0.0)).x;\n' +
            '    float pb = texture2D(uPressure, uv-vec2(0.0,texelSize.y)).x;\n' +
            '    float pt = texture2D(uPressure, uv+vec2(0.0,texelSize.y)).x;\n' +
            '    float b  = texture2D(uDivergence, uv).x;\n' +
            '    float p  = (pl + pr + pb + pt - b) * 0.25;\n' +
            '    gl_FragColor = vec4(p, 0.0, 0.0, 1.0);\n' +
            '}\n';

        // Subtract pressure gradient to make velocity divergence-free.
        var fs_grad = common +
            'uniform sampler2D uPressure;\n' +
            'uniform sampler2D uVelocity;\n' +
            'uniform vec2 texelSize;\n' +
            'uniform float velRange;\n' +
            '\n' +
            'vec2 decode(vec2 v, float r){ return v*r*2.0-r; }\n' +
            'vec2 encode(vec2 v, float r){ return clamp(v/r*0.5+0.5, 0.0, 1.0); }\n' +
            '\n' +
            'void main(){\n' +
            '    vec2 uv = v_uv;\n' +
            '    float pl = texture2D(uPressure, uv-vec2(texelSize.x,0.0)).x;\n' +
            '    float pr = texture2D(uPressure, uv+vec2(texelSize.x,0.0)).x;\n' +
            '    float pb = texture2D(uPressure, uv-vec2(0.0,texelSize.y)).x;\n' +
            '    float pt = texture2D(uPressure, uv+vec2(0.0,texelSize.y)).x;\n' +
            '    vec2 g = vec2(pr - pl, pt - pb) * 0.5;\n' +
            '    vec2 vel = decode(texture2D(uVelocity, uv).xy, velRange);\n' +
            '    vel -= g;\n' +
            '    gl_FragColor = vec4(encode(vel, velRange), 0.0, 1.0);\n' +
            '}\n';

        // Vorticity confinement: adds small swirling forces to preserve curls.
        var fs_vort = common +
            'uniform sampler2D uVelocity;\n' +
            'uniform vec2 texelSize;\n' +
            'uniform float curl;\n' +
            'uniform float dt;\n' +
            'uniform vec2 scale;\n' +
            'uniform float velRange;\n' +
            '\n' +
            'vec2 decode(vec2 v, float r){ return v*r*2.0-r; }\n' +
            'vec2 encode(vec2 v, float r){ return clamp(v/r*0.5+0.5, 0.0, 1.0); }\n' +
            '\n' +
            'float getCurl(vec2 uv, vec2 dx){\n' +
            '    vec2 vl = decode(texture2D(uVelocity, uv-vec2(dx.x,0.0)).xy, velRange);\n' +
            '    vec2 vr = decode(texture2D(uVelocity, uv+vec2(dx.x,0.0)).xy, velRange);\n' +
            '    vec2 vb = decode(texture2D(uVelocity, uv-vec2(0.0,dx.y)).xy, velRange);\n' +
            '    vec2 vt = decode(texture2D(uVelocity, uv+vec2(0.0,dx.y)).xy, velRange);\n' +
            '    return ((vr.y - vl.y) - (vt.x - vb.x)) * 0.5;\n' +
            '}\n' +
            '\n' +
            'void main(){\n' +
            '    vec2 uv = v_uv;\n' +
            '    vec2 dx = texelSize * scale;\n' +
            '    float cL = getCurl(uv-vec2(dx.x,0.0), dx);\n' +
            '    float cR = getCurl(uv+vec2(dx.x,0.0), dx);\n' +
            '    float cB = getCurl(uv-vec2(0.0,dx.y), dx);\n' +
            '    float cT = getCurl(uv+vec2(0.0,dx.y), dx);\n' +
            '    float cC = getCurl(uv, dx);\n' +
            '    vec2 grad = vec2(abs(cR) - abs(cL), abs(cT) - abs(cB));\n' +
            '    float len = length(grad) + 1e-5;\n' +
            '    vec2 n = grad / len;\n' +
            '    float scaleAvg = (scale.x + scale.y) * 0.5;\n' +
            '    vec2 f = vec2(n.y, -n.x) * cC * curl * dt * scaleAvg * 0.1;\n' +
            '    vec2 vel = decode(texture2D(uVelocity, uv).xy, velRange);\n' +
            '    vel += f;\n' +
            '    gl_FragColor = vec4(encode(vel, velRange), 0.0, 1.0);\n' +
            '}\n';

        // Buoyancy: pushes velocity upward based on heat (density).
        var fs_buoy = common +
            'uniform sampler2D uVelocity;\n' +
            'uniform sampler2D uDensity;\n' +
            'uniform float strength;\n' +
            'uniform float maxHeat;\n' +
            'uniform float heatPower;\n' +
            'uniform float minHeat;\n' +
            'uniform float velRange;\n' +
            '\n' +
            'vec2 decode(vec2 v, float r){ return v*r*2.0-r; }\n' +
            'vec2 encode(vec2 v, float r){ return clamp(v/r*0.5+0.5, 0.0, 1.0); }\n' +
            '\n' +
            'void main(){\n' +
            '    vec2 uv = v_uv;\n' +
            '    float d = texture2D(uDensity, uv).r;\n' +
            '    float normalized = clamp(d / maxHeat, 0.0, 1.0);\n' +
            '    float absHP = abs(heatPower);\n' +
            '    float curve = pow(normalized, absHP);\n' +
            '    float heatNorm = heatPower >= 0.0 ? curve : 1.0 - curve;\n' +
            '    float heat = mix(minHeat, 1.0, heatNorm) * d;\n' +
            '    vec2 vel = decode(texture2D(uVelocity, uv).xy, velRange);\n' +
            '    vel.y += heat * strength;\n' +
            '    gl_FragColor = vec4(encode(vel, velRange), 0.0, 1.0);\n' +
            '}\n';

        // Add density at a point with a gaussian falloff.
        var fs_splat = common +
            'uniform sampler2D uTarget;\n' +
            'uniform vec2 point;\n' +
            'uniform float radius;\n' +
            'uniform vec3 color;\n' +
            '\n' +
            'void main(){\n' +
            '    vec2 uv = v_uv;\n' +
            '    vec4 cur = texture2D(uTarget, uv);\n' +
            '    float d = distance(uv, point);\n' +
            '    float a = exp(-d*d/(radius*radius));\n' +
            '    cur.rgb += color * a;\n' +
            '    gl_FragColor = cur;\n' +
            '}\n';

        // Add velocity at a point with a gaussian falloff.
        var fs_splat_vel = common +
            'uniform sampler2D uTarget;\n' +
            'uniform vec2 point;\n' +
            'uniform float radius;\n' +
            'uniform vec2 force;\n' +
            'uniform float velRange;\n' +
            '\n' +
            'vec2 encode(vec2 v, float r){ return clamp(v/r*0.5+0.5, 0.0, 1.0); }\n' +
            'vec2 decode(vec2 v, float r){ return v*r*2.0-r; }\n' +
            '\n' +
            'void main(){\n' +
            '    vec2 uv = v_uv;\n' +
            '    vec4 cur = texture2D(uTarget, uv);\n' +
            '    float d = distance(uv, point);\n' +
            '    float a = exp(-d*d/(radius*radius));\n' +
            '    vec2 vel = decode(cur.xy, velRange);\n' +
            '    vel += force * a;\n' +
            '    gl_FragColor = vec4(encode(vel, velRange), 0.0, 1.0);\n' +
            '}\n';

        // Add text mask into density (uses text alpha as strength).
        var fs_text = common +
            'uniform sampler2D uTarget;\n' +
            'uniform sampler2D uText;\n' +
            'uniform float strength;\n' +
            'uniform float time;\n' +
            'uniform float noiseScale;\n' +
            'uniform float noiseFreq;\n' +
            'uniform float noiseSpeed;\n' +
            'uniform float noiseType;\n' +
            'uniform float maskNoiseScale;\n' +
            'uniform float maskNoiseFreq;\n' +
            'uniform float maskNoiseSpeed;\n' +
            'uniform float maskNoiseType;\n' +
            'uniform float maskNoiseContrast;\n' +
            'uniform float maskNoiseBrightnessTop;\n' +
            'uniform float maskNoiseBrightnessBottom;\n' +
            'uniform float maskTextTop;\n' +
            'uniform float maskTextBottom;\n' +
            'uniform float noiseDriftX;\n' +
            'uniform float noiseDriftY;\n' +
            'uniform float maskDriftX;\n' +
            'uniform float maskDriftY;\n' +
            '\n' +
            'float hash(vec2 p){ return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }\n' +
            'float hash3(vec3 p){ return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453); }\n' +
            'vec2 hash2(vec2 p){\n' +
            '    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)))) * 43758.5453);\n' +
            '}\n' +
            'vec3 hash23(vec3 p){\n' +
            '    return fract(sin(vec3(dot(p, vec3(127.1, 311.7, 74.7)), dot(p, vec3(269.5, 183.3, 246.1)), dot(p, vec3(113.5, 271.9, 124.6)))) * 43758.5453);\n' +
            '}\n' +
            'float noise(vec2 p){\n' +
            '    vec2 i = floor(p);\n' +
            '    vec2 f = fract(p);\n' +
            '    float a = hash(i);\n' +
            '    float b = hash(i + vec2(1.0, 0.0));\n' +
            '    float c = hash(i + vec2(0.0, 1.0));\n' +
            '    float d = hash(i + vec2(1.0, 1.0));\n' +
            '    vec2 u = f * f * (3.0 - 2.0 * f);\n' +
            '    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;\n' +
            '}\n' +
            'float noise3(vec3 p){\n' +
            '    vec3 i = floor(p);\n' +
            '    vec3 f = fract(p);\n' +
            '    vec3 u = f * f * (3.0 - 2.0 * f);\n' +
            '    float a = hash3(i);\n' +
            '    float b = hash3(i + vec3(1.0, 0.0, 0.0));\n' +
            '    float c = hash3(i + vec3(0.0, 1.0, 0.0));\n' +
            '    float d = hash3(i + vec3(1.0, 1.0, 0.0));\n' +
            '    float e = hash3(i + vec3(0.0, 0.0, 1.0));\n' +
            '    float f1 = hash3(i + vec3(1.0, 0.0, 1.0));\n' +
            '    float g = hash3(i + vec3(0.0, 1.0, 1.0));\n' +
            '    float h = hash3(i + vec3(1.0, 1.0, 1.0));\n' +
            '    float k0 = mix(a, b, u.x);\n' +
            '    float k1 = mix(c, d, u.x);\n' +
            '    float k2 = mix(e, f1, u.x);\n' +
            '    float k3 = mix(g, h, u.x);\n' +
            '    float k4 = mix(k0, k1, u.y);\n' +
            '    float k5 = mix(k2, k3, u.y);\n' +
            '    return mix(k4, k5, u.z);\n' +
            '}\n' +
            'float tri(float x){ return abs(fract(x) - 0.5) * 2.0; }\n' +
            'float sineNoise(vec2 p){ return sin((p.x + p.y) * 3.14159) * 0.5 + 0.5; }\n' +
            'float triNoise(vec2 p){ return tri(p.x + tri(p.y)); }\n' +
            'float stripeNoise(vec2 p){ return tri(p.x); }\n' +
            'float checkerNoise(vec2 p){ vec2 c = floor(p); return mod(c.x + c.y, 2.0); }\n' +
            'float rippleNoise(vec2 p){ vec2 c = p - vec2(0.5); float r = length(c); return sin(r * 12.56636) * 0.5 + 0.5; }\n' +
            'float spiralNoise(vec2 p){ vec2 c = p - vec2(0.5); float a = atan(c.y, c.x); float r = length(c); return sin(a * 4.0 + r * 10.0) * 0.5 + 0.5; }\n' +
            'float fbmNoise(vec2 p){\n' +
            '    float v = 0.0;\n' +
            '    float a = 0.5;\n' +
            '    for (int i = 0; i < 4; i++) {\n' +
            '        v += a * noise(p);\n' +
            '        p *= 2.0;\n' +
            '        a *= 0.5;\n' +
            '    }\n' +
            '    return v;\n' +
            '}\n' +
            'float ridgedNoise(vec2 p){ float n = noise(p); return 1.0 - abs(2.0 * n - 1.0); }\n' +
            'float cellularNoise(vec2 p){\n' +
            '    vec2 i = floor(p);\n' +
            '    vec2 f = fract(p);\n' +
            '    float minDist = 10.0;\n' +
            '    for (int y = -1; y <= 1; y++) {\n' +
            '        for (int x = -1; x <= 1; x++) {\n' +
            '            vec2 n = vec2(float(x), float(y));\n' +
            '            vec2 h = hash2(i + n);\n' +
            '            vec2 diff = n + h - f;\n' +
            '            minDist = min(minDist, dot(diff, diff));\n' +
            '        }\n' +
            '    }\n' +
            '    return 1.0 - clamp(minDist, 0.0, 1.0);\n' +
            '}\n' +
            'float fbmNoise3(vec3 p){\n' +
            '    float v = 0.0;\n' +
            '    float a = 0.5;\n' +
            '    for (int i = 0; i < 4; i++) {\n' +
            '        v += a * noise3(p);\n' +
            '        p *= 2.0;\n' +
            '        a *= 0.5;\n' +
            '    }\n' +
            '    return v;\n' +
            '}\n' +
            'float ridgedNoise3(vec3 p){ float n = noise3(p); return 1.0 - abs(2.0 * n - 1.0); }\n' +
            'float cellularNoise3(vec3 p){\n' +
            '    vec3 i = floor(p);\n' +
            '    vec3 f = fract(p);\n' +
            '    float minDist = 10.0;\n' +
            '    for (int z = -1; z <= 1; z++) {\n' +
            '        for (int y = -1; y <= 1; y++) {\n' +
            '            for (int x = -1; x <= 1; x++) {\n' +
            '                vec3 n = vec3(float(x), float(y), float(z));\n' +
            '                vec3 h = hash23(i + n);\n' +
            '                vec3 diff = n + h - f;\n' +
            '                minDist = min(minDist, dot(diff, diff));\n' +
            '            }\n' +
            '        }\n' +
            '    }\n' +
            '    return 1.0 - clamp(minDist, 0.0, 1.0);\n' +
            '}\n' +
            '// Additional noise functions\n' +
            'float turbulence(vec2 p) {\n' +
            '    float v = 0.0; float a = 0.5; float f = 1.0;\n' +
            '    for (int i = 0; i < 4; i++) { v += abs(noise(p * f) - 0.5) * a; a *= 0.5; f *= 2.0; }\n' +
            '    return clamp(v * 2.0, 0.0, 1.0);\n' +
            '}\n' +
            'float turbulence3(vec3 p) {\n' +
            '    float v = 0.0; float a = 0.5; float f = 1.0;\n' +
            '    for (int i = 0; i < 4; i++) { v += abs(noise3(p * f) - 0.5) * a; a *= 0.5; f *= 2.0; }\n' +
            '    return clamp(v * 2.0, 0.0, 1.0);\n' +
            '}\n' +
            'float marble(vec2 p) { return sin(p.x * 3.0 + fbmNoise(p) * 5.0) * 0.5 + 0.5; }\n' +
            'float marble3(vec3 p) { return sin(p.x * 3.0 + fbmNoise3(p) * 5.0) * 0.5 + 0.5; }\n' +
            'float wood(vec2 p) { float r = length(p - vec2(0.5)) * 10.0; return sin(r + fbmNoise(p) * 3.0) * 0.5 + 0.5; }\n' +
            'float wood3(vec3 p) { float r = length(p.xy - vec2(0.5)) * 10.0; return sin(r + fbmNoise3(p) * 3.0) * 0.5 + 0.5; }\n' +
            'float plasma(vec2 p) { return sin(p.x * 10.0) * 0.25 + sin(p.y * 10.0) * 0.25 + sin((p.x + p.y) * 7.0) * 0.25 + sin(length(p) * 10.0) * 0.25 + 0.5; }\n' +
            'float plasma3(vec3 p) { return sin(p.x * 10.0) * 0.2 + sin(p.y * 10.0) * 0.2 + sin(p.z * 10.0) * 0.2 + sin((p.x + p.y) * 7.0) * 0.2 + sin(length(p) * 10.0) * 0.2 + 0.5; }\n' +
            'float hexagons(vec2 p) {\n' +
            '    vec2 h = vec2(1.0, 1.732); vec2 a = mod(p, h) - h * 0.5; vec2 b = mod(p - h * 0.5, h) - h * 0.5;\n' +
            '    return smoothstep(0.0, 0.1, min(dot(a, a), dot(b, b)));\n' +
            '}\n' +
            'float hexagons3(vec3 p) { return hexagons(p.xy + p.z * 0.5); }\n' +
            'float dots(vec2 p) { vec2 i = floor(p); vec2 f = fract(p); return 1.0 - smoothstep(0.0, 0.35, length(f - 0.5)); }\n' +
            'float dots3(vec3 p) { return dots(p.xy + p.z * 0.3); }\n' +
            'float crosshatch(vec2 p) {\n' +
            '    float l1 = smoothstep(0.0, 0.1, abs(fract(p.x + p.y) - 0.5));\n' +
            '    float l2 = smoothstep(0.0, 0.1, abs(fract(p.x - p.y) - 0.5));\n' +
            '    return l1 * l2;\n' +
            '}\n' +
            'float crosshatch3(vec3 p) { return crosshatch(p.xy + p.z * 0.3); }\n' +
            'float caustics(vec2 p) {\n' +
            '    float c = 0.0;\n' +
            '    for (int i = 0; i < 3; i++) {\n' +
            '        float f = float(i + 1) * 2.0;\n' +
            '        c += sin(p.x * f + sin(p.y * f * 0.5)) * sin(p.y * f + sin(p.x * f * 0.5));\n' +
            '    }\n' +
            '    return c * 0.16 + 0.5;\n' +
            '}\n' +
            'float caustics3(vec3 p) { return caustics(p.xy + vec2(sin(p.z), cos(p.z)) * 0.5); }\n' +
            'float voronoi(vec2 p) {\n' +
            '    vec2 i = floor(p); vec2 f = fract(p); float md = 10.0; float md2 = 10.0;\n' +
            '    for (int y = -1; y <= 1; y++) {\n' +
            '        for (int x = -1; x <= 1; x++) {\n' +
            '            vec2 n = vec2(float(x), float(y)); vec2 h = hash2(i + n);\n' +
            '            float d = length(n + h - f); if (d < md) { md2 = md; md = d; } else if (d < md2) { md2 = d; }\n' +
            '        }\n' +
            '    }\n' +
            '    return clamp(md2 - md, 0.0, 1.0);\n' +
            '}\n' +
            'float voronoi3(vec3 p) {\n' +
            '    vec3 i = floor(p); vec3 f = fract(p); float md = 10.0; float md2 = 10.0;\n' +
            '    for (int z = -1; z <= 1; z++) {\n' +
            '        for (int y = -1; y <= 1; y++) {\n' +
            '            for (int x = -1; x <= 1; x++) {\n' +
            '                vec3 n = vec3(float(x), float(y), float(z)); vec3 h = hash23(i + n);\n' +
            '                float d = length(n + h - f); if (d < md) { md2 = md; md = d; } else if (d < md2) { md2 = d; }\n' +
            '            }\n' +
            '        }\n' +
            '    }\n' +
            '    return clamp(md2 - md, 0.0, 1.0);\n' +
            '}\n' +
            '\n' +
            'float selectNoise(vec2 p, float t){\n' +
            '    if (t < 0.5) return noise(p);\n' +
            '    if (t < 1.5) return sineNoise(p);\n' +
            '    if (t < 2.5) return triNoise(p);\n' +
            '    if (t < 3.5) return stripeNoise(p);\n' +
            '    if (t < 4.5) return checkerNoise(p);\n' +
            '    if (t < 5.5) return rippleNoise(p);\n' +
            '    if (t < 6.5) return spiralNoise(p);\n' +
            '    if (t < 7.5) return fbmNoise(p);\n' +
            '    if (t < 8.5) return ridgedNoise(p);\n' +
            '    if (t < 9.5) return cellularNoise(p);\n' +
            '    if (t < 10.5) return turbulence(p);\n' +
            '    if (t < 11.5) return marble(p);\n' +
            '    if (t < 12.5) return wood(p);\n' +
            '    if (t < 13.5) return plasma(p);\n' +
            '    if (t < 14.5) return hexagons(p);\n' +
            '    if (t < 15.5) return dots(p);\n' +
            '    if (t < 16.5) return crosshatch(p);\n' +
            '    if (t < 17.5) return caustics(p);\n' +
            '    return voronoi(p);\n' +
            '}\n' +
            'float selectNoise3(vec3 p, float t){\n' +
            '    if (t < 0.5) return noise3(p);\n' +
            '    if (t < 1.5) return 0.5 + 0.5 * sin((p.x + p.y + p.z) * 3.14159);\n' +
            '    if (t < 2.5) return abs(fract(p.x + abs(fract(p.y) - 0.5) * 2.0 + abs(fract(p.z) - 0.5) * 2.0) - 0.5) * 2.0;\n' +
            '    if (t < 3.5) return abs(fract(p.x) - 0.5) * 2.0;\n' +
            '    if (t < 4.5) { vec3 c = floor(p); return mod(c.x + c.y + c.z, 2.0); }\n' +
            '    if (t < 5.5) { vec3 c = p - vec3(0.5); float r = length(c); return sin(r * 12.56636) * 0.5 + 0.5; }\n' +
            '    if (t < 6.5) { vec3 c = p - vec3(0.5); float a = atan(c.y, c.x); float r = length(c.xy); return sin(a * 4.0 + r * 10.0 + p.z * 5.0) * 0.5 + 0.5; }\n' +
            '    if (t < 7.5) return fbmNoise3(p);\n' +
            '    if (t < 8.5) return ridgedNoise3(p);\n' +
            '    if (t < 9.5) return cellularNoise3(p);\n' +
            '    if (t < 10.5) return turbulence3(p);\n' +
            '    if (t < 11.5) return marble3(p);\n' +
            '    if (t < 12.5) return wood3(p);\n' +
            '    if (t < 13.5) return plasma3(p);\n' +
            '    if (t < 14.5) return hexagons3(p);\n' +
            '    if (t < 15.5) return dots3(p);\n' +
            '    if (t < 16.5) return crosshatch3(p);\n' +
            '    if (t < 17.5) return caustics3(p);\n' +
            '    return voronoi3(p);\n' +
            '}\n' +
            '\n' +
            'void main(){\n' +
            '    vec2 uv = v_uv;\n' +
            '    vec2 tuv = vec2(uv.x, 1.0 - uv.y);\n' +
            '    vec2 pn = uv * noiseFreq + time * vec2(noiseDriftX, noiseDriftY);\n' +
            '    float tn = time * noiseSpeed;\n' +
            '    float n = selectNoise3(vec3(pn, tn), noiseType);\n' +
            '    vec2 warp = vec2(n - 0.5) * noiseScale;\n' +
            '    tuv += warp;\n' +
            '    vec4 cur = texture2D(uTarget, uv);\n' +
            '    float baseMask = texture2D(uText, tuv).a;\n' +
            '    float scale = max(maskNoiseScale, 0.0001);\n' +
            '    vec2 pm = (uv - 0.5) * scale + 0.5;\n' +
            '    pm = pm * maskNoiseFreq + time * vec2(maskDriftX, maskDriftY);\n' +
            '    float tz = time * maskNoiseSpeed;\n' +
            '    float nm = selectNoise3(vec3(pm, tz), maskNoiseType);\n' +
            '    float textY = clamp((uv.y - maskTextBottom) / max(1e-4, (maskTextTop - maskTextBottom)), 0.0, 1.0);\n' +
            '    float brightness = mix(maskNoiseBrightnessBottom, maskNoiseBrightnessTop, textY);\n' +
            '    float nmAdj = clamp((nm - 0.5) * maskNoiseContrast + 0.5 + brightness, 0.0, 1.0);\n' +
            '    float mask = baseMask * nmAdj;\n' +
            '    vec3 add = vec3(mask * strength);\n' +
            '    cur.rgb = clamp(cur.rgb + add, 0.0, 1.0);\n' +
            '    gl_FragColor = cur;\n' +
            '}\n';

        // Final display shader: map density to palette, add glow, output color.
        var fs_display = common +
            'uniform sampler2D uDensity;\n' +
            'uniform sampler2D uPalette;\n' +
            'uniform vec2 resolution;\n' +
            'uniform float glow;\n' +
            'uniform vec2 vortexScale;\n' +
            'uniform vec2 texelSize;\n' +
            '\n' +
            'void main(){\n' +
            '    vec2 uv = v_uv;\n' +
            '    float d = clamp(texture2D(uDensity, uv).r, 0.0, 1.0);\n' +
            '    vec3 col = texture2D(uPalette, vec2(d, 0.5)).rgb;\n' +
            '    vec2 px = 1.0 / resolution * (1.0 + glow * 1.5);\n' +
            '    float g = 0.0;\n' +
            '    for(int i=-2;i<=2;i++){\n' +
            '        for(int j=-2;j<=2;j++){\n' +
            '            g += texture2D(uDensity, uv + vec2(float(i),float(j)) * px).r;\n' +
            '        }\n' +
            '    }\n' +
            '    g *= 0.02 * glow;\n' +
            '    col += vec3(1.0, 0.6, 0.2) * g;\n' +
            '    col = pow(col, vec3(0.85));\n' +
            '    gl_FragColor = vec4(col, 1.0);\n' +
            '}\n';

        // Spark line shaders.
        var vs_spark =
            'attribute vec2 aPos;\n' +
            'attribute float aAlpha;\n' +
            'varying float vAlpha;\n' +
            'void main(){\n' +
            '    vAlpha = aAlpha;\n' +
            '    gl_Position = vec4(aPos*2.0-1.0, 0.0, 1.0);\n' +
            '}\n';
        var fs_spark =
            'precision highp float;\n' +
            'varying float vAlpha;\n' +
            'void main(){\n' +
            '    vec3 col = vec3(1.0, 0.8, 0.4);\n' +
            '    gl_FragColor = vec4(col, vAlpha);\n' +
            '}\n';

        // Noise shader code shared between preview shaders
        var noiseShaderCode =
            'float hash(vec2 p){ return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }\n' +
            'float hash3(vec3 p){ return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453); }\n' +
            'vec2 hash2(vec2 p){\n' +
            '    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)))) * 43758.5453);\n' +
            '}\n' +
            'vec3 hash23(vec3 p){\n' +
            '    return fract(sin(vec3(dot(p, vec3(127.1, 311.7, 74.7)), dot(p, vec3(269.5, 183.3, 246.1)), dot(p, vec3(113.5, 271.9, 124.6)))) * 43758.5453);\n' +
            '}\n' +
            'float noise3(vec3 p){\n' +
            '    vec3 i = floor(p);\n' +
            '    vec3 f = fract(p);\n' +
            '    vec3 u = f * f * (3.0 - 2.0 * f);\n' +
            '    float a = hash3(i);\n' +
            '    float b = hash3(i + vec3(1.0, 0.0, 0.0));\n' +
            '    float c = hash3(i + vec3(0.0, 1.0, 0.0));\n' +
            '    float d = hash3(i + vec3(1.0, 1.0, 0.0));\n' +
            '    float e = hash3(i + vec3(0.0, 0.0, 1.0));\n' +
            '    float f1 = hash3(i + vec3(1.0, 0.0, 1.0));\n' +
            '    float g = hash3(i + vec3(0.0, 1.0, 1.0));\n' +
            '    float h = hash3(i + vec3(1.0, 1.0, 1.0));\n' +
            '    float k0 = mix(a, b, u.x);\n' +
            '    float k1 = mix(c, d, u.x);\n' +
            '    float k2 = mix(e, f1, u.x);\n' +
            '    float k3 = mix(g, h, u.x);\n' +
            '    float k4 = mix(k0, k1, u.y);\n' +
            '    float k5 = mix(k2, k3, u.y);\n' +
            '    return mix(k4, k5, u.z);\n' +
            '}\n' +
            'float fbmNoise3(vec3 p){\n' +
            '    float v = 0.0;\n' +
            '    float a = 0.5;\n' +
            '    for (int i = 0; i < 4; i++) {\n' +
            '        v += a * noise3(p);\n' +
            '        p *= 2.0;\n' +
            '        a *= 0.5;\n' +
            '    }\n' +
            '    return v;\n' +
            '}\n' +
            'float ridgedNoise3(vec3 p){ float n = noise3(p); return 1.0 - abs(2.0 * n - 1.0); }\n' +
            'float cellularNoise3(vec3 p){\n' +
            '    vec3 i = floor(p);\n' +
            '    vec3 f = fract(p);\n' +
            '    float minDist = 10.0;\n' +
            '    for (int z = -1; z <= 1; z++) {\n' +
            '        for (int y = -1; y <= 1; y++) {\n' +
            '            for (int x = -1; x <= 1; x++) {\n' +
            '                vec3 n = vec3(float(x), float(y), float(z));\n' +
            '                vec3 h = hash23(i + n);\n' +
            '                vec3 diff = n + h - f;\n' +
            '                minDist = min(minDist, dot(diff, diff));\n' +
            '            }\n' +
            '        }\n' +
            '    }\n' +
            '    return 1.0 - clamp(minDist, 0.0, 1.0);\n' +
            '}\n' +
            '// Additional noise functions\n' +
            'float turbulence3(vec3 p) {\n' +
            '    float v = 0.0; float a = 0.5; float f = 1.0;\n' +
            '    for (int i = 0; i < 4; i++) { v += abs(noise3(p * f) - 0.5) * a; a *= 0.5; f *= 2.0; }\n' +
            '    return clamp(v * 2.0, 0.0, 1.0);\n' +
            '}\n' +
            'float marble3(vec3 p) { return sin(p.x * 3.0 + fbmNoise3(p) * 5.0) * 0.5 + 0.5; }\n' +
            'float wood3(vec3 p) { float r = length(p.xy - vec2(0.5)) * 10.0; return sin(r + fbmNoise3(p) * 3.0) * 0.5 + 0.5; }\n' +
            'float plasma3(vec3 p) { return sin(p.x * 10.0) * 0.2 + sin(p.y * 10.0) * 0.2 + sin(p.z * 10.0) * 0.2 + sin((p.x + p.y) * 7.0) * 0.2 + sin(length(p) * 10.0) * 0.2 + 0.5; }\n' +
            'float hexagons3(vec3 p) {\n' +
            '    vec2 h = vec2(1.0, 1.732); vec2 a = mod(p.xy + p.z * 0.5, h) - h * 0.5; vec2 b = mod(p.xy + p.z * 0.5 - h * 0.5, h) - h * 0.5;\n' +
            '    return smoothstep(0.0, 0.1, min(dot(a, a), dot(b, b)));\n' +
            '}\n' +
            'float dots3(vec3 p) { vec2 uv = p.xy + p.z * 0.3; vec2 f = fract(uv); return 1.0 - smoothstep(0.0, 0.35, length(f - 0.5)); }\n' +
            'float crosshatch3(vec3 p) {\n' +
            '    vec2 uv = p.xy + p.z * 0.3;\n' +
            '    float l1 = smoothstep(0.0, 0.1, abs(fract(uv.x + uv.y) - 0.5));\n' +
            '    float l2 = smoothstep(0.0, 0.1, abs(fract(uv.x - uv.y) - 0.5));\n' +
            '    return l1 * l2;\n' +
            '}\n' +
            'float caustics3(vec3 p) {\n' +
            '    vec2 uv = p.xy + vec2(sin(p.z), cos(p.z)) * 0.5;\n' +
            '    float c = 0.0;\n' +
            '    for (int i = 0; i < 3; i++) {\n' +
            '        float f = float(i + 1) * 2.0;\n' +
            '        c += sin(uv.x * f + sin(uv.y * f * 0.5)) * sin(uv.y * f + sin(uv.x * f * 0.5));\n' +
            '    }\n' +
            '    return c * 0.16 + 0.5;\n' +
            '}\n' +
            'float voronoi3(vec3 p) {\n' +
            '    vec3 i = floor(p); vec3 f = fract(p); float md = 10.0; float md2 = 10.0;\n' +
            '    for (int z = -1; z <= 1; z++) {\n' +
            '        for (int y = -1; y <= 1; y++) {\n' +
            '            for (int x = -1; x <= 1; x++) {\n' +
            '                vec3 n = vec3(float(x), float(y), float(z)); vec3 h = hash23(i + n);\n' +
            '                float d = length(n + h - f); if (d < md) { md2 = md; md = d; } else if (d < md2) { md2 = d; }\n' +
            '            }\n' +
            '        }\n' +
            '    }\n' +
            '    return clamp(md2 - md, 0.0, 1.0);\n' +
            '}\n' +
            'float selectNoise3(vec3 p, float t){\n' +
            '    if (t < 0.5) return noise3(p);\n' +
            '    if (t < 1.5) return 0.5 + 0.5 * sin((p.x + p.y + p.z) * 3.14159);\n' +
            '    if (t < 2.5) return abs(fract(p.x + abs(fract(p.y) - 0.5) * 2.0 + abs(fract(p.z) - 0.5) * 2.0) - 0.5) * 2.0;\n' +
            '    if (t < 3.5) return abs(fract(p.x) - 0.5) * 2.0;\n' +
            '    if (t < 4.5) { vec3 c = floor(p); return mod(c.x + c.y + c.z, 2.0); }\n' +
            '    if (t < 5.5) { vec3 c = p - vec3(0.5); float r = length(c); return sin(r * 12.56636) * 0.5 + 0.5; }\n' +
            '    if (t < 6.5) { vec3 c = p - vec3(0.5); float a = atan(c.y, c.x); float r = length(c.xy); return sin(a * 4.0 + r * 10.0 + p.z * 5.0) * 0.5 + 0.5; }\n' +
            '    if (t < 7.5) return fbmNoise3(p);\n' +
            '    if (t < 8.5) return ridgedNoise3(p);\n' +
            '    if (t < 9.5) return cellularNoise3(p);\n' +
            '    if (t < 10.5) return turbulence3(p);\n' +
            '    if (t < 11.5) return marble3(p);\n' +
            '    if (t < 12.5) return wood3(p);\n' +
            '    if (t < 13.5) return plasma3(p);\n' +
            '    if (t < 14.5) return hexagons3(p);\n' +
            '    if (t < 15.5) return dots3(p);\n' +
            '    if (t < 16.5) return crosshatch3(p);\n' +
            '    if (t < 17.5) return caustics3(p);\n' +
            '    return voronoi3(p);\n' +
            '}\n';

        // Text noise preview shader
        var fs_preview_noise = common + noiseShaderCode +
            'uniform float time;\n' +
            'uniform float noiseFreq;\n' +
            'uniform float noiseSpeed;\n' +
            'uniform float noiseType;\n' +
            'uniform float noiseAmplitude;\n' +
            'uniform float noiseDriftX;\n' +
            'uniform float noiseDriftY;\n' +
            'void main(){\n' +
            '    vec2 uv = v_uv;\n' +
            '    vec2 pn = uv * noiseFreq + time * vec2(noiseDriftX, noiseDriftY);\n' +
            '    float tn = time * noiseSpeed;\n' +
            '    float n = selectNoise3(vec3(pn, tn), noiseType);\n' +
            '    float warp = (n - 0.5) * noiseAmplitude * 10.0;\n' +
            '    float v = clamp(0.5 + warp, 0.0, 1.0);\n' +
            '    gl_FragColor = vec4(vec3(v), 1.0);\n' +
            '}\n';

        // Text mask preview shader
        var fs_preview_mask = common + noiseShaderCode +
            'uniform float time;\n' +
            'uniform float maskScale;\n' +
            'uniform float maskFreq;\n' +
            'uniform float maskSpeed;\n' +
            'uniform float maskType;\n' +
            'uniform float maskContrast;\n' +
            'uniform float maskBrightnessTop;\n' +
            'uniform float maskBrightnessBottom;\n' +
            'uniform float maskDriftX;\n' +
            'uniform float maskDriftY;\n' +
            'void main(){\n' +
            '    vec2 uv = v_uv;\n' +
            '    float scale = max(maskScale, 0.0001);\n' +
            '    vec2 pm = (uv - 0.5) * scale + 0.5;\n' +
            '    pm = pm * maskFreq + time * vec2(maskDriftX, maskDriftY);\n' +
            '    float tz = time * maskSpeed;\n' +
            '    float n = selectNoise3(vec3(pm, tz), maskType);\n' +
            '    float brightness = mix(maskBrightnessBottom, maskBrightnessTop, uv.y);\n' +
            '    float v = clamp((n - 0.5) * maskContrast + 0.5 + brightness, 0.0, 1.0);\n' +
            '    gl_FragColor = vec4(vec3(v), 1.0);\n' +
            '}\n';

        // Compile a shader from GLSL source.
        function createShader(type, src) {
            var s = gl.createShader(type);
            gl.shaderSource(s, src);
            gl.compileShader(s);
            if (!gl.getShaderParameter(s, gl.COMPILE_STATUS)) {
                console.error(gl.getShaderInfoLog(s));
                gl.deleteShader(s);
                return null;
            }
            return s;
        }

        var vsObj = createShader(gl.VERTEX_SHADER, vs);
        // Link a program using the shared vertex shader and a fragment shader.
        function createProgram(fsSrc) {
            var fsObj = createShader(gl.FRAGMENT_SHADER, fsSrc);
            var p = gl.createProgram();
            gl.attachShader(p, vsObj);
            gl.attachShader(p, fsObj);
            gl.linkProgram(p);
            if (!gl.getProgramParameter(p, gl.LINK_STATUS)) {
                console.error(gl.getProgramInfoLog(p));
                return null;
            }
            return p;
        }

        var progAdvect = createProgram(fs_advect);
        var progAdvectVel = createProgram(fs_advect_vel);
        var progDiv = createProgram(fs_div);
        var progJac = createProgram(fs_jacobi);
        var progGrad = createProgram(fs_grad);
        var progVort = createProgram(fs_vort);
        var progBuoy = createProgram(fs_buoy);
        var progSplat = createProgram(fs_splat);
        var progSplatVel = createProgram(fs_splat_vel);
        var progText = createProgram(fs_text);
        var progDisp = createProgram(fs_display);
        var progPreviewNoise = createProgram(fs_preview_noise);
        var progPreviewMask = createProgram(fs_preview_mask);

        var vsSparkObj = createShader(gl.VERTEX_SHADER, vs_spark);
        var fsSparkObj = createShader(gl.FRAGMENT_SHADER, fs_spark);
        var progSpark = gl.createProgram();
        gl.attachShader(progSpark, vsSparkObj);
        gl.attachShader(progSpark, fsSparkObj);
        gl.linkProgram(progSpark);
        var sparkLineBuf = gl.createBuffer();

        // Create a texture-backed framebuffer object.
        function createFBO(w, h) {
            var tex = gl.createTexture();
            gl.bindTexture(gl.TEXTURE_2D, tex);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, floatLinear ? gl.LINEAR : gl.NEAREST);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, floatLinear ? gl.LINEAR : gl.NEAREST);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, w, h, 0, gl.RGBA, textureType, null);
            var fb = gl.createFramebuffer();
            gl.bindFramebuffer(gl.FRAMEBUFFER, fb);
            gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, tex, 0);
            return { texture: tex, framebuffer: fb, width: w, height: h };
        }

        // Create ping-pong framebuffers for simulation steps.
        function createDouble(w, h) {
            return { read: createFBO(w, h), write: createFBO(w, h), swap: function () { var t = this.read; this.read = this.write; this.write = t; } };
        }

        // Text rendering settings.
        var textEnabled = true;
        var textValue = 'Fire!';
        var textFont = 'Arial';
        var textSize = 96;
        var textOffsetX = 0.0; // -0.5..0.5 (fraction of width)
        var textOffsetY = 0.0; // -0.5..0.5 (fraction of height)
        var textStrength = 1.5;
        var textNoiseType = 0; // 0 = Value noise, 1 = Sine warp
        var textNoiseScale = 0.03; // Warp amplitude
        var textNoiseFreq = 3.0;   // Warp frequency
        var textNoiseSpeed = 0.4;
        var textMaskNoiseType = 0;
        var textMaskNoiseScale = 1.0;
        var textMaskNoiseFreq = 3.0;
        var textMaskNoiseSpeed = 0.4;
        var textMaskNoiseContrast = 1.0;
        var textMaskNoiseBrightnessTop = 0.0;
        var textMaskNoiseBrightnessBottom = 0.0;
        var textNoiseDriftX = 0.0;
        var textNoiseDriftY = 0.0;
        var textMaskDriftX = 0.0;
        var textMaskDriftY = 0.0;
        var textMaskTopUv = 1.0;
        var textMaskBottomUv = 0.0;
        var textCanvas = document.createElement('canvas');
        var textCtx = textCanvas.getContext('2d');
        var textTexture = gl.createTexture();
        var textDirty = true;

        var simScale = 0.3;
        var simW = 0, simH = 0;
        var velocity, density, pressure, divergenceFBO;

        // Clear a framebuffer to a constant value.
        function clearFBO(fb, r, g, b, a) {
            gl.bindFramebuffer(gl.FRAMEBUFFER, fb.framebuffer);
            gl.clearColor(r, g, b, a);
            gl.clear(gl.COLOR_BUFFER_BIT);
        }

        // Allocate or reallocate all simulation buffers based on simScale.
        function allocFBOs() {
            simW = Math.max(64, Math.round(canvas.width * simScale));
            simH = Math.max(64, Math.round(canvas.height * simScale));
            velocity = createDouble(simW, simH);
            density = createDouble(simW, simH);
            pressure = createDouble(simW, simH);
            divergenceFBO = createFBO(simW, simH);
            clearFBO(velocity.read, 0.5, 0.5, 0.0, 1.0);
            clearFBO(velocity.write, 0.5, 0.5, 0.0, 1.0);
            clearFBO(density.read, 0.0, 0.0, 0.0, 0.0);
            clearFBO(density.write, 0.0, 0.0, 0.0, 0.0);
            clearFBO(pressure.read, 0.0, 0.0, 0.0, 0.0);
            clearFBO(pressure.write, 0.0, 0.0, 0.0, 0.0);
            clearFBO(divergenceFBO, 0.0, 0.0, 0.0, 0.0);
            ensureTextResources();
        }

        // Ensure text canvas/texture match simulation resolution.
        function ensureTextResources() {
            if (textCanvas.width !== simW || textCanvas.height !== simH) {
                textCanvas.width = simW;
                textCanvas.height = simH;
                textDirty = true;
            }
            gl.bindTexture(gl.TEXTURE_2D, textTexture);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        }

        // GPU-based noise preview rendering
        var previewSize = 128;
        function renderNoisePreviewGPU(time) {
            // Text noise preview
            gl.viewport(0, 0, previewSize, previewSize);
            gl.useProgram(progPreviewNoise);
            bindQuad(progPreviewNoise);
            gl.uniform1f(gl.getUniformLocation(progPreviewNoise, 'time'), time);
            gl.uniform1f(gl.getUniformLocation(progPreviewNoise, 'noiseFreq'), textNoiseFreq);
            gl.uniform1f(gl.getUniformLocation(progPreviewNoise, 'noiseSpeed'), textNoiseSpeed);
            gl.uniform1f(gl.getUniformLocation(progPreviewNoise, 'noiseType'), textNoiseType);
            gl.uniform1f(gl.getUniformLocation(progPreviewNoise, 'noiseAmplitude'), textNoiseScale);
            gl.uniform1f(gl.getUniformLocation(progPreviewNoise, 'noiseDriftX'), textNoiseDriftX);
            gl.uniform1f(gl.getUniformLocation(progPreviewNoise, 'noiseDriftY'), textNoiseDriftY);
            gl.bindFramebuffer(gl.FRAMEBUFFER, previewNoiseFBO.framebuffer);
            gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

            // Text mask preview
            gl.useProgram(progPreviewMask);
            bindQuad(progPreviewMask);
            gl.uniform1f(gl.getUniformLocation(progPreviewMask, 'time'), time);
            gl.uniform1f(gl.getUniformLocation(progPreviewMask, 'maskScale'), textMaskNoiseScale);
            gl.uniform1f(gl.getUniformLocation(progPreviewMask, 'maskFreq'), textMaskNoiseFreq);
            gl.uniform1f(gl.getUniformLocation(progPreviewMask, 'maskSpeed'), textMaskNoiseSpeed);
            gl.uniform1f(gl.getUniformLocation(progPreviewMask, 'maskType'), textMaskNoiseType);
            gl.uniform1f(gl.getUniformLocation(progPreviewMask, 'maskContrast'), textMaskNoiseContrast);
            gl.uniform1f(gl.getUniformLocation(progPreviewMask, 'maskBrightnessTop'), textMaskNoiseBrightnessTop);
            gl.uniform1f(gl.getUniformLocation(progPreviewMask, 'maskBrightnessBottom'), textMaskNoiseBrightnessBottom);
            gl.uniform1f(gl.getUniformLocation(progPreviewMask, 'maskDriftX'), textMaskDriftX);
            gl.uniform1f(gl.getUniformLocation(progPreviewMask, 'maskDriftY'), textMaskDriftY);
            gl.bindFramebuffer(gl.FRAMEBUFFER, previewMaskFBO.framebuffer);
            gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

            // Restore main viewport
            gl.bindFramebuffer(gl.FRAMEBUFFER, null);
            gl.viewport(0, 0, canvas.width, canvas.height);
        }

        // Copy FBO texture to 2D canvas for display
        function copyFBOToCanvas(fbo, targetCanvas) {
            var pixels = new Uint8Array(fbo.width * fbo.height * 4);
            gl.bindFramebuffer(gl.FRAMEBUFFER, fbo.framebuffer);
            gl.readPixels(0, 0, fbo.width, fbo.height, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
            gl.bindFramebuffer(gl.FRAMEBUFFER, null);
            var ctx = targetCanvas.getContext('2d');
            var img = ctx.createImageData(fbo.width, fbo.height);
            // Flip Y axis when copying
            for (var y = 0; y < fbo.height; y++) {
                var srcRow = (fbo.height - 1 - y) * fbo.width * 4;
                var dstRow = y * fbo.width * 4;
                for (var x = 0; x < fbo.width * 4; x++) {
                    img.data[dstRow + x] = pixels[srcRow + x];
                }
            }
            ctx.putImageData(img, 0, 0);
        }

        // Preview FBOs (created after allocFBOs is defined)
        var previewNoiseFBO, previewMaskFBO;
        function createPreviewFBOs() {
            // Create simple RGBA FBOs for preview rendering
            function createPreviewFBO(w, h) {
                var tex = gl.createTexture();
                gl.bindTexture(gl.TEXTURE_2D, tex);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
                gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, w, h, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
                var fb = gl.createFramebuffer();
                gl.bindFramebuffer(gl.FRAMEBUFFER, fb);
                gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, tex, 0);
                return { texture: tex, framebuffer: fb, width: w, height: h };
            }
            previewNoiseFBO = createPreviewFBO(previewSize, previewSize);
            previewMaskFBO = createPreviewFBO(previewSize, previewSize);
        }

        // Render text into the offscreen canvas and upload to GPU.
        function updateTextTexture() {
            if (!textDirty) return;
            textDirty = false;
            textCtx.clearRect(0, 0, textCanvas.width, textCanvas.height);
            textCtx.save();
            textCtx.fillStyle = 'white';
            textCtx.textAlign = 'center';
            textCtx.textBaseline = 'middle';
            textCtx.font = textSize + 'px ' + textFont;
            var cx = textCanvas.width * (0.5 + textOffsetX);
            var cy = textCanvas.height * (0.5 + textOffsetY);
            textCtx.fillText(textValue, cx, cy);
            var halfH = textSize * 0.5;
            var topPx = cy - halfH;
            var bottomPx = cy + halfH;
            var topUv = 1.0 - (topPx / textCanvas.height);
            var bottomUv = 1.0 - (bottomPx / textCanvas.height);
            textMaskTopUv = Math.min(1.0, Math.max(0.0, Math.max(topUv, bottomUv)));
            textMaskBottomUv = Math.min(1.0, Math.max(0.0, Math.min(topUv, bottomUv)));
            textCtx.restore();

            gl.bindTexture(gl.TEXTURE_2D, textTexture);
            gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, textCanvas);
        }

        var quadBuf = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, quadBuf);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 1, -1, -1, 1, 1, 1]), gl.STATIC_DRAW);

        // Bind fullscreen quad to a program's position attribute.
        function bindQuad(prog) {
            var loc = gl.getAttribLocation(prog, 'position');
            gl.bindBuffer(gl.ARRAY_BUFFER, quadBuf);
            if (loc >= 0) { gl.enableVertexAttribArray(loc); gl.vertexAttribPointer(loc, 2, gl.FLOAT, false, 0, 0); }
        }

        // Set viewport to render target size or screen size.
        function setViewport(target) {
            if (target) { gl.bindFramebuffer(gl.FRAMEBUFFER, target.framebuffer); gl.viewport(0, 0, target.width, target.height); }
            else { gl.bindFramebuffer(gl.FRAMEBUFFER, null); gl.viewport(0, 0, canvas.width, canvas.height); }
        }

        // Render a full-screen pass to a target with optional uniform setup.
        function renderTo(target, prog, setUniforms) {
            if (!prog) return;
            gl.useProgram(prog);
            bindQuad(prog);
            if (setUniforms) setUniforms(prog);
            setViewport(target);
            gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
        }

        // Resize canvas and simulation buffers when window size changes.
        function resize() {
            canvas.width = window.innerWidth;
            canvas.height = window.innerHeight;
            gl.viewport(0, 0, canvas.width, canvas.height);
            allocFBOs();
        }
        window.addEventListener('resize', resize);
        resize();
        createPreviewFBOs();

        // Defaults
        var dt = 1 / 60;
        var jacobi = 32;        // Pressure iterations
        var curl = 77;          // Vorticity
        var vortexScaleX = 15.0;
        var vortexScaleY = 31.0;
        var buoyancy = 15.0;
        var maxHeat = 4.0;
        var heatPower = 3.2;
        var minHeat = 0.15;
        var glow = 1.2;
        var emberRate = 6;      // Embers spawned per frame
        var emberHeight = 0.02; // Vertical spawn range near bottom
        var emberOffset = 0.0;  // Vertical spawn offset from bottom
        var emberWidth = 0.6;   // Screen width covered by embers
        var emberSize = 0.008;  // Base ember radius
        var emberUniformity = 0.7; // 0 = uniform size, 1 = edges tiny
        var emberLifeFrames = 1; // Lifespan in frames (1-60)
        var velDissipation = 0.3;
        var denDissipation = 0.91;
        var sparkChance = 0.275;
        var sparkMax = 64;     // Max number of sparks
        var rdx = 130.0;
        var velRange = 3.0;
        var pointer = { down: false, x: 0, y: 0, px: 0, py: 0 };
        var splats = [];
        var embers = [];
        var pointerEmber = null;
        var sparks = [];

        var paletteSize = 256;
        var paletteTexture = gl.createTexture();

        // When defining palettes, pos: 1.0 is the start colour and 0.0 is the end colour (effect blends from 1.0 to 0.0)
        var palettes = [
            {
                name: 'Fire', stops: [
                    { pos: 0.0, color: [0.05, 0.0, 0.0] },
                    { pos: 0.4, color: [0.8, 0.15, 0.02] },
                    { pos: 0.8, color: [1.0, 0.6, 0.1] },
                    { pos: 1.0, color: [1.0, 0.95, 0.8] }
                ]
            },
            {
                name: 'Fire (reversed)', stops: [
                    { pos: 0.0, color: [1.0, 0.95, 0.8] },
                    { pos: 0.4, color: [1.0, 0.6, 0.1] },
                    { pos: 0.8, color: [0.8, 0.15, 0.02] },
                    { pos: 1.0, color: [0.05, 0.0, 0.0] }
                ]
            },
            {
                name: 'Rainbow', stops: [
                    { pos: 0.0, color: [0.0, 0.0, 0.0] },
                    { pos: 0.1, color: [1.0, 0.0, 0.0] },
                    { pos: 0.17, color: [1.0, 0.5, 0.0] },
                    { pos: 0.33, color: [1.0, 1.0, 0.0] },
                    { pos: 0.5, color: [0.0, 1.0, 0.0] },
                    { pos: 0.67, color: [0.0, 1.0, 1.0] },
                    { pos: 0.83, color: [0.0, 0.3, 1.0] },
                    { pos: 1.0, color: [0.6, 0.0, 1.0] }
                ]
            },
            {
                name: 'Rainbow (reversed)', stops: [
                    { pos: 0.0, color: [0.6, 0.0, 1.0] },
                    { pos: 0.17, color: [0.0, 0.3, 1.0] },
                    { pos: 0.33, color: [0.0, 1.0, 1.0] },
                    { pos: 0.5, color: [0.0, 1.0, 0.0] },
                    { pos: 0.67, color: [1.0, 1.0, 0.0] },
                    { pos: 0.83, color: [1.0, 0.5, 0.0] },
                    { pos: 0.9, color: [1.0, 0.0, 0.0] },
                    { pos: 1.0, color: [0.0, 0.0, 0.0] }
                ]
            },
            {
                name: 'Fire + Smoke', stops: [
                    { pos: 0.0, color: [0.05, 0.0, 0.0] },
                    { pos: 0.4, color: [0.8, 0.15, 0.02] },
                    { pos: 0.8, color: [1.0, 0.6, 0.1] },
                    { pos: 0.9, color: [1.0, 0.95, 0.8] },
                    { pos: 0.97, color: [0.55, 0.55, 0.55] },
                    { pos: 1.0, color: [0.0, 0.0, 0.0] }
                ]
            },
            {
                name: 'Smoke', stops: [
                    { pos: 0.0, color: [0.0, 0.0, 0.0] },
                    { pos: 0.1, color: [0.1, 0.1, 0.1] },
                    { pos: 0.2, color: [0.35, 0.35, 0.35] },
                    { pos: 0.4, color: [0.8, 0.15, 0.02] },
                    { pos: 0.8, color: [1.0, 0.6, 0.1] },
                    { pos: 1.0, color: [1.0, 0.95, 0.8] }
                ]
            },
            {
                name: 'Smoke 2', stops: [
                    { pos: 0.0, color: [0.0, 0.0, 0.0] },
                    { pos: 0.2, color: [0.0, 0.0, 0.0] },
                    { pos: 0.5, color: [0.35, 0.35, 0.35] },
                    { pos: 0.6, color: [0.8, 0.15, 0.02] },
                    { pos: 0.85, color: [1.0, 0.6, 0.1] },
                    { pos: 1.0, color: [1.0, 0.95, 0.8] }
                ]
            },
            {
                name: 'Smoke 3', stops: [
                    { pos: 0.0, color: [0.0, 0.0, 0.0] },
                    { pos: 0.2, color: [0.0, 0.0, 0.0] },
                    { pos: 0.24, color: [0.3, 0.3, 0.3] },
                    { pos: 0.25, color: [0.4, 0.4, 0.45] },
                    { pos: 0.3, color: [0.8, 0.15, 0.02] },
                    { pos: 0.8, color: [1.0, 0.6, 0.1] },
                    { pos: 1.0, color: [1.0, 0.95, 0.8] }
                ]
            },
        ];
        var currentPalette = palettes[0];

        // Convert palette stop list to a CSS gradient for UI preview.
        function paletteToCss(palette) {
            var parts = [];
            for (var i = 0; i < palette.stops.length; i++) {
                var s = palette.stops[i];
                var r = Math.round(s.color[0] * 255);
                var g = Math.round(s.color[1] * 255);
                var b = Math.round(s.color[2] * 255);
                parts.push('rgb(' + r + ',' + g + ',' + b + ') ' + Math.round(s.pos * 100) + '%');
            }
            return 'linear-gradient(270deg,' + parts.join(',') + ')';
        }

        // Build a 1D RGBA palette texture from stops.
        function buildPaletteData(palette) {
            var stops = palette.stops.slice().sort(function (a, b) { return a.pos - b.pos; });
            if (stops.length === 1) {
                stops = [stops[0], { pos: 1.0, color: stops[0].color.slice(0) }];
            }
            var data = new Uint8Array(paletteSize * 4);
            var si = 0;
            for (var i = 0; i < paletteSize; i++) {
                var t = i / (paletteSize - 1);
                while (si < stops.length - 2 && t > stops[si + 1].pos) { si++; }
                var a = stops[si];
                var b = stops[Math.min(si + 1, stops.length - 1)];
                var span = b.pos - a.pos;
                var tt = span > 0.0 ? (t - a.pos) / span : 0.0;
                var r = a.color[0] + (b.color[0] - a.color[0]) * tt;
                var g = a.color[1] + (b.color[1] - a.color[1]) * tt;
                var bcol = a.color[2] + (b.color[2] - a.color[2]) * tt;
                var idx = i * 4;
                data[idx] = Math.round(r * 255);
                data[idx + 1] = Math.round(g * 255);
                data[idx + 2] = Math.round(bcol * 255);
                data[idx + 3] = 255;
            }
            return data;
        }

        // Upload palette data to the GPU texture.
        function uploadPaletteTexture(palette) {
            var data = buildPaletteData(palette);
            gl.bindTexture(gl.TEXTURE_2D, paletteTexture);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, paletteSize, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE, data);
        }

        uploadPaletteTexture(currentPalette);

        // Todo - ember size so towards edges of the bottom embers, they are smaller and more fleeting for better fire edges, and there can be more including higher up in the middle (like a pile)
        // If we do ember points as an array that can be filled by new random embers each frame - they can have a lifespan
        // I think the splat framework is already set up for this - we just need to account for the mouse pointer and adding it as a new point and making sure that point's timer is reset each frame if the mouse pointer isnt moving and is still held down
        // and the mouse click embers can be added to that with a different lifespan so they hang on screen for a moment
        // me nice for the ember splat to also randomise splat size a bit and also misshape it so there not all perfect circles
        // also have sliders for width and height of ember pile and how far up the screen it sits
        // Does splats with velocity actually do anything?
        // Splatting text
        // Turn on borders so fluid is constrained to screen

        // UI controls
        var uiContainer = document.createElement('div');
        uiContainer.className = 'fire-ui-container';
        var ctrl = document.createElement('div');
        ctrl.className = 'fire-ui';
        var textCtrl = document.createElement('div');
        textCtrl.className = 'fire-ui';
        // Create a labeled slider row.
        function addSlider(label, min, max, step, value, onChange, tooltip, target) {
            var row = document.createElement('div');
            row.className = 'fire-ui-row';
            if (tooltip) { row.title = tooltip; }
            var l = document.createElement('label');
            l.textContent = label;
            l.className = 'fire-ui-label';
            if (tooltip) { l.title = tooltip; }
            var input = document.createElement('input');
            input.type = 'range';
            input.min = min; input.max = max; input.step = step; input.value = value;
            input.className = 'fire-ui-range';
            if (tooltip) { input.title = tooltip; }
            var valueSpan = document.createElement('span');
            valueSpan.textContent = value;
            valueSpan.className = 'fire-ui-value';
            if (tooltip) { valueSpan.title = tooltip; }
            input.oninput = function () { valueSpan.textContent = input.value; onChange(parseFloat(input.value)); };
            row.appendChild(l);
            row.appendChild(input);
            row.appendChild(valueSpan);
            (target || ctrl).appendChild(row);
        }

        // Create a palette dropdown with preview swatches.
        function addPaletteDropdown(label, paletteList, onChange, tooltip, target) {
            var row = document.createElement('div');
            row.className = 'fire-ui-row';
            if (tooltip) { row.title = tooltip; }
            var l = document.createElement('label');
            l.textContent = label;
            l.className = 'fire-ui-label';
            if (tooltip) { l.title = tooltip; }
            var wrap = document.createElement('div');
            wrap.className = 'fire-ui-dropdown';
            var button = document.createElement('div');
            button.className = 'fire-ui-button';
            if (tooltip) { button.title = tooltip; }
            var nameSpan = document.createElement('span');
            var swatch = document.createElement('div');
            swatch.className = 'fire-ui-swatch';
            var list = document.createElement('div');
            list.className = 'fire-ui-list';

            function setPalette(p) {
                nameSpan.textContent = p.name;
                swatch.style.background = paletteToCss(p);
                onChange(p);
            }

            for (var i = 0; i < paletteList.length; i++) {
                (function (p) {
                    var item = document.createElement('div');
                    item.className = 'fire-ui-item';
                    var itemName = document.createElement('span');
                    itemName.textContent = p.name;
                    var itemSwatch = document.createElement('div');
                    itemSwatch.className = 'fire-ui-swatch';
                    itemSwatch.style.background = paletteToCss(p);
                    item.appendChild(itemName);
                    item.appendChild(itemSwatch);
                    item.onclick = function () { setPalette(p); list.style.display = 'none'; };
                    list.appendChild(item);
                })(paletteList[i]);
            }

            button.onclick = function (e) {
                e.stopPropagation();
                list.style.display = (list.style.display === 'none') ? 'block' : 'none';
            };
            document.addEventListener('click', function (e) {
                if (!wrap.contains(e.target)) list.style.display = 'none';
            });

            button.appendChild(nameSpan);
            button.appendChild(swatch);
            wrap.appendChild(button);
            wrap.appendChild(list);
            row.appendChild(l);
            row.appendChild(wrap);
            (target || ctrl).appendChild(row);

            setPalette(paletteList[0]);
        }

        // Create a font dropdown with font previews.
        function addFontDropdown(label, fontList, onChange, tooltip, target) {
            var row = document.createElement('div');
            row.className = 'fire-ui-row';
            if (tooltip) { row.title = tooltip; }
            var l = document.createElement('label');
            l.textContent = label;
            l.className = 'fire-ui-label';
            if (tooltip) { l.title = tooltip; }
            var wrap = document.createElement('div');
            wrap.className = 'fire-ui-dropdown';
            var button = document.createElement('div');
            button.className = 'fire-ui-button';
            if (tooltip) { button.title = tooltip; }
            var nameSpan = document.createElement('span');
            var list = document.createElement('div');
            list.className = 'fire-ui-list';

            function setFont(f) {
                nameSpan.textContent = f;
                nameSpan.style.fontFamily = f;
                onChange(f);
            }

            for (var i = 0; i < fontList.length; i++) {
                (function (f) {
                    var item = document.createElement('div');
                    item.className = 'fire-ui-item';
                    var itemName = document.createElement('span');
                    itemName.textContent = f;
                    itemName.style.fontFamily = f;
                    item.appendChild(itemName);
                    item.onclick = function () { setFont(f); list.style.display = 'none'; };
                    list.appendChild(item);
                })(fontList[i]);
            }

            button.onclick = function (e) {
                e.stopPropagation();
                list.style.display = (list.style.display === 'none') ? 'block' : 'none';
            };
            document.addEventListener('click', function (e) {
                if (!wrap.contains(e.target)) list.style.display = 'none';
            });

            button.appendChild(nameSpan);
            wrap.appendChild(button);
            wrap.appendChild(list);
            row.appendChild(l);
            row.appendChild(wrap);
            (target || ctrl).appendChild(row);

            setFont(fontList[0]);
        }

        // Create a noise dropdown for text distortion options.
        function addNoiseDropdown(label, items, onChange, tooltip, target) {
            var row = document.createElement('div');
            row.className = 'fire-ui-row';
            if (tooltip) { row.title = tooltip; }
            var l = document.createElement('label');
            l.textContent = label;
            l.className = 'fire-ui-label';
            if (tooltip) { l.title = tooltip; }
            var wrap = document.createElement('div');
            wrap.className = 'fire-ui-dropdown';
            var button = document.createElement('div');
            button.className = 'fire-ui-button';
            if (tooltip) { button.title = tooltip; }
            var nameSpan = document.createElement('span');
            var list = document.createElement('div');
            list.className = 'fire-ui-list';

            function setNoise(item) {
                nameSpan.textContent = item.label;
                onChange(item.value);
            }

            for (var i = 0; i < items.length; i++) {
                (function (item) {
                    var entry = document.createElement('div');
                    entry.className = 'fire-ui-item';
                    var entryName = document.createElement('span');
                    entryName.textContent = item.label;
                    entry.appendChild(entryName);
                    entry.onclick = function () { setNoise(item); list.style.display = 'none'; };
                    list.appendChild(entry);
                })(items[i]);
            }

            button.onclick = function (e) {
                e.stopPropagation();
                list.style.display = (list.style.display === 'none') ? 'block' : 'none';
            };
            document.addEventListener('click', function (e) {
                if (!wrap.contains(e.target)) list.style.display = 'none';
            });

            button.appendChild(nameSpan);
            wrap.appendChild(button);
            wrap.appendChild(list);
            row.appendChild(l);
            row.appendChild(wrap);
            (target || ctrl).appendChild(row);

            setNoise(items[0]);
        }

        // Create a labeled text input row.
        function addTextInput(label, value, onChange, tooltip, target) {
            var row = document.createElement('div');
            row.className = 'fire-ui-row';
            if (tooltip) { row.title = tooltip; }
            var l = document.createElement('label');
            l.textContent = label;
            l.className = 'fire-ui-label';
            if (tooltip) { l.title = tooltip; }
            var input = document.createElement('input');
            input.type = 'text';
            input.value = value;
            input.className = 'fire-ui-text';
            if (tooltip) { input.title = tooltip; }
            input.oninput = function () { onChange(input.value); };
            row.appendChild(l);
            row.appendChild(input);
            (target || ctrl).appendChild(row);
        }

        // Create a labeled checkbox row.
        function addCheckbox(label, value, onChange, tooltip, target) {
            var row = document.createElement('div');
            row.className = 'fire-ui-row';
            if (tooltip) { row.title = tooltip; }
            var l = document.createElement('label');
            l.textContent = label;
            l.className = 'fire-ui-label';
            if (tooltip) { l.title = tooltip; }
            var input = document.createElement('input');
            input.type = 'checkbox';
            input.checked = value;
            input.className = 'fire-ui-check';
            if (tooltip) { input.title = tooltip; }
            input.onchange = function () { onChange(input.checked); };
            row.appendChild(l);
            row.appendChild(input);
            (target || ctrl).appendChild(row);
        }

        // Add a section heading in the UI panel.
        function addHeading(text, target) {
            var h = document.createElement('div');
            h.className = 'fire-ui-heading';
            h.textContent = text;
            (target || ctrl).appendChild(h);
        }
        var textFonts = [
            'Arial',
            'Georgia',
            'Times New Roman',
            'Trebuchet MS',
            'Verdana',
            'Courier New',
            'Impact',
            'Comic Sans MS',
            'Tahoma',
            'Palatino Linotype',
            'Garamond',
            'Book Antiqua',
            'Century Gothic',
            'Lucida Console',
            'Lucida Sans Unicode',
            'Copperplate Gothic Bold',
            'Brush Script MT',
            'Segoe UI',
            'Cambria',
            'Consolas'
        ];
        var textNoiseOptions = [
            { label: 'Value noise', value: 0 },
            { label: 'Sine warp', value: 1 },
            { label: 'Triangle noise', value: 2 },
            { label: 'Stripes', value: 3 },
            { label: 'Checker', value: 4 },
            { label: 'Ripple', value: 5 },
            { label: 'Spiral', value: 6 },
            { label: 'FBM', value: 7 },
            { label: 'Ridged', value: 8 },
            { label: 'Cellular', value: 9 },
            { label: 'Turbulence', value: 10 },
            { label: 'Marble', value: 11 },
            { label: 'Wood', value: 12 },
            { label: 'Plasma', value: 13 },
            { label: 'Hexagons', value: 14 },
            { label: 'Dots', value: 15 },
            { label: 'Crosshatch', value: 16 },
            { label: 'Caustics', value: 17 },
            { label: 'Voronoi', value: 18 }
        ];
        content.appendChild(uiContainer);
        uiContainer.appendChild(ctrl);
        uiContainer.appendChild(textCtrl);
        addPaletteDropdown('palette', palettes, function (p) { currentPalette = p; uploadPaletteTexture(currentPalette); }, 'Selects the color gradient used to map density values to flame colors. This only affects rendering, not the simulation.', ctrl);
        addHeading('Simulation', ctrl);
        addSlider('quality (sim scale)', 0.1, 2.0, 0.05, simScale, function (v) { simScale = v; allocFBOs(); }, 'Simulation resolution scale. Increase for sharper detail but higher GPU cost; decrease for faster performance.', ctrl);
        addSlider('pressure iterations', 1, 48, 1, jacobi, function (v) { jacobi = Math.floor(v); }, 'Number of pressure-solve iterations. More iterations reduce volume changes and make flow look less “squishy.”', ctrl);
        addSlider('vorticity', 0.0, 100.0, 1.0, curl, function (v) { curl = v; }, 'Adds swirl via vorticity confinement. Higher values increase curl and turbulence in the flame.', ctrl);
        addSlider('vortex scale H', 0.0, 50.0, 1.0, vortexScaleX, function (v) { vortexScaleX = v; }, 'Horizontal sampling distance for curl detection. Larger values produce broader swirls.', ctrl);
        addSlider('vortex scale V', 0.0, 50.0, 1.0, vortexScaleY, function (v) { vortexScaleY = v; }, 'Vertical sampling distance for curl detection. Larger values produce taller swirls.', ctrl);
        addSlider('velocity range', 1.0, 5.0, 0.5, velRange, function (v) { velRange = v; }, 'Velocity encoding range in textures. Higher allows faster motion but reduces precision for subtle flow.', ctrl);
        addSlider('velocity dissipation', 0.0, 2.0, 0.01, velDissipation, function (v) { velDissipation = v; }, 'How quickly velocity decays each frame. Higher values calm the flow faster.', ctrl);
        addSlider('density dissipation', 0.8, 1.0, 0.001, denDissipation, function (v) { denDissipation = v; }, 'How quickly density fades each frame. Lower values keep smoke/fire lingering longer.', ctrl);
        addSlider('advection speed', 10.0, 500.0, 5.0, rdx, function (v) { rdx = v; }, 'Scales how far fields are pushed by velocity each frame. Higher values increase motion speed.', ctrl);

        addHeading('Fire', ctrl);
        addSlider('buoyancy (rise)', 10.0, 100.0, 0.5, buoyancy, function (v) { buoyancy = v; }, 'Strength of upward lift applied from heat. Higher values make the flame rise faster overall.', ctrl);
        addSlider('max heat', 0.5, 10.0, 0.5, maxHeat, function (v) { maxHeat = v; }, 'Density value treated as “full heat.” Lower values make the same density hotter, increasing lift.', ctrl);
        addSlider('heat power', -5.0, 5.0, 0.1, heatPower, function (v) { heatPower = v; }, 'Shape of the heat curve. Positive makes hot cores rise much faster; negative boosts cooler edges instead.', ctrl);
        addSlider('min heat', 0.0, 0.5, 0.01, minHeat, function (v) { minHeat = v; }, 'Minimum heat contribution so fading areas still move upward.', ctrl);

        addHeading('Embers', ctrl);
        addSlider('embers per frame', 1, 100, 1, emberRate, function (v) { emberRate = Math.floor(v); }, 'How many new ember splats are created each frame. Higher values make the base denser and brighter.', ctrl);
        addSlider('ember life (frames)', 1, 60, 1, emberLifeFrames, function (v) { emberLifeFrames = Math.floor(v); }, 'How long embers persist. Higher values leave longer-lasting trails before they fade out.', ctrl);
        addSlider('ember width', 0.1, 1.2, 0.01, emberWidth, function (v) { emberWidth = v; }, 'Horizontal span of the ember spawn band (as a fraction of the screen width).', ctrl);
        addSlider('ember height', 0.0, 0.2, 0.005, emberHeight, function (v) { emberHeight = v; }, 'Vertical thickness of the ember spawn band near the bottom.', ctrl);
        addSlider('ember offset', 0.0, 0.5, 0.005, emberOffset, function (v) { emberOffset = v; }, 'Moves the ember spawn band upward from the bottom of the screen.', ctrl);
        addSlider('ember size', 0.001, 0.1, 0.001, emberSize, function (v) { emberSize = v; }, 'Base radius for ember splats; larger values create thicker, softer embers.', ctrl);
        addSlider('ember uniformity', 0.0, 1.0, 0.01, emberUniformity, function (v) { emberUniformity = v; }, 'Center weighting for ember size. 0 = all equal; 1 = edges shrink and center stays larger.', ctrl);

        addHeading('Misc.', ctrl);
        addSlider('spark chance', 0.0, 1.0, 0.01, sparkChance, function (v) { sparkChance = v; }, 'Chance of a spark spawning each frame. Higher values produce more frequent sparks.', ctrl);
        addSlider('glow', 0.0, 2.0, 0.01, glow, function (v) { glow = v; }, 'Strength of the soft glow around bright areas. Higher values give a hotter, blooming look.', ctrl);

        addHeading('Text', textCtrl);
        addCheckbox('text enabled', textEnabled, function (v) { textEnabled = v; textDirty = true; }, 'Toggle drawing of the text mask into the fire.', textCtrl);
        addTextInput('text', textValue, function (v) { textValue = v; textDirty = true; }, 'Text string that will be drawn into the fire.', textCtrl);
        addFontDropdown('font', textFonts, function (v) { textFont = v; textDirty = true; }, 'Font family used to render the text.', textCtrl);
        addSlider('font size', 10, 600, 2, textSize, function (v) { textSize = v; textDirty = true; }, 'Font size in pixels. Larger values produce bigger text.', textCtrl);
        addSlider('text offset X', -0.5, 0.5, 0.01, textOffsetX, function (v) { textOffsetX = v; textDirty = true; }, 'Horizontal offset of the text as a fraction of screen width (0 = center).', textCtrl);
        addSlider('text offset Y', -0.5, 0.5, 0.01, textOffsetY, function (v) { textOffsetY = v; textDirty = true; }, 'Vertical offset of the text as a fraction of screen height (0 = center).', textCtrl);
        addNoiseDropdown('text noise', textNoiseOptions, function (v) { textNoiseType = v; }, 'Select the distortion pattern used to warp the text.', textCtrl);
        addSlider('noise amplitude', 0.0, 0.1, 0.001, textNoiseScale, function (v) { textNoiseScale = v; }, 'Strength of the warp in UV space. Higher values distort the text more.', textCtrl);
        addSlider('noise frequency', 0.5, 100.0, 0.1, textNoiseFreq, function (v) { textNoiseFreq = v; }, 'Density of the warp pattern. Higher values create finer ripples.', textCtrl);
        addSlider('noise speed', 0.0, 10.0, 0.05, textNoiseSpeed, function (v) { textNoiseSpeed = v; }, 'Animation speed of the distortion over time.', textCtrl);
        addSlider('noise drift H', -2.0, 2.0, 0.05, textNoiseDriftX, function (v) { textNoiseDriftX = v; }, 'Horizontal drift velocity for text warp noise.', textCtrl);
        addSlider('noise drift V', -2.0, 2.0, 0.05, textNoiseDriftY, function (v) { textNoiseDriftY = v; }, 'Vertical drift velocity for text warp noise.', textCtrl);

        var showTextPreviews = false;
        addCheckbox('show text previews', showTextPreviews, function (v) { showTextPreviews = v; updatePreviewVisibility(); }, 'Show live grayscale previews of the text noise and mask.', textCtrl);
        var textNoisePreviewWrap = document.createElement('div');
        textNoisePreviewWrap.className = 'fire-ui-preview';
        var textNoisePreviewLabel = document.createElement('div');
        textNoisePreviewLabel.className = 'fire-ui-preview-label';
        textNoisePreviewLabel.textContent = 'Text noise preview';
        var textNoisePreviewCanvas = document.createElement('canvas');
        textNoisePreviewCanvas.className = 'fire-ui-preview-canvas';
        textNoisePreviewCanvas.width = 128;
        textNoisePreviewCanvas.height = 128;
        textNoisePreviewWrap.appendChild(textNoisePreviewLabel);
        textNoisePreviewWrap.appendChild(textNoisePreviewCanvas);
        textCtrl.appendChild(textNoisePreviewWrap);

        addHeading('Text mask', textCtrl);
        addNoiseDropdown('mask noise', textNoiseOptions, function (v) { textMaskNoiseType = v; }, 'Select the noise pattern that masks the text intensity.', textCtrl);
        addSlider('mask noise scale', 0.0, 50.0, 0.06, textMaskNoiseScale, function (v) { textMaskNoiseScale = v; }, 'Overall scale of the mask noise pattern. Higher values create smaller details.', textCtrl);
        addSlider('mask noise frequency', 0.5, 100.0, 0.1, textMaskNoiseFreq, function (v) { textMaskNoiseFreq = v; }, 'Density of the mask noise pattern. Higher values create finer holes.', textCtrl);
        addSlider('mask noise speed', 0.0, 10.0, 0.05, textMaskNoiseSpeed, function (v) { textMaskNoiseSpeed = v; }, 'Animation speed of the mask noise.', textCtrl);
        addSlider('mask drift H', -2.0, 2.0, 0.05, textMaskDriftX, function (v) { textMaskDriftX = v; }, 'Horizontal drift velocity for mask noise.', textCtrl);
        addSlider('mask drift V', -2.0, 2.0, 0.05, textMaskDriftY, function (v) { textMaskDriftY = v; }, 'Vertical drift velocity for mask noise.', textCtrl);
        addSlider('mask noise contrast', 0.0, 4.0, 0.05, textMaskNoiseContrast, function (v) { textMaskNoiseContrast = v; }, 'Contrast applied to the mask noise. Higher values create crisper holes.', textCtrl);
        addSlider('mask brightness top', -1.0, 1.0, 0.02, textMaskNoiseBrightnessTop, function (v) { textMaskNoiseBrightnessTop = v; }, 'Brightness offset at the top of the text mask. Lower values reduce intensity.', textCtrl);
        addSlider('mask brightness bottom', -1.0, 1.0, 0.02, textMaskNoiseBrightnessBottom, function (v) { textMaskNoiseBrightnessBottom = v; }, 'Brightness offset at the bottom of the text mask. Lower values reduce intensity.', textCtrl);

        var textMaskPreviewWrap = document.createElement('div');
        textMaskPreviewWrap.className = 'fire-ui-preview';
        var textMaskPreviewLabel = document.createElement('div');
        textMaskPreviewLabel.className = 'fire-ui-preview-label';
        textMaskPreviewLabel.textContent = 'Text mask preview';
        var textMaskPreviewCanvas = document.createElement('canvas');
        textMaskPreviewCanvas.className = 'fire-ui-preview-canvas';
        textMaskPreviewCanvas.width = 128;
        textMaskPreviewCanvas.height = 128;
        textMaskPreviewWrap.appendChild(textMaskPreviewLabel);
        textMaskPreviewWrap.appendChild(textMaskPreviewCanvas);
        textCtrl.appendChild(textMaskPreviewWrap);

        function updatePreviewVisibility() {
            var display = showTextPreviews ? 'block' : 'none';
            textNoisePreviewWrap.style.display = display;
            textMaskPreviewWrap.style.display = display;
        }
        updatePreviewVisibility();

        // Queue a density/velocity splat to apply this frame.
        function addSplat(x, y, dx, dy, radius, densityAmount) {
            splats.push({ x: x, y: y, dx: dx, dy: dy, r: radius, d: densityAmount });
        }

        // Add a persistent ember that re-splats for its lifespan.
        function addEmber(x, y, dx, dy, radius, densityAmount, lifeFrames) {
            var life = Math.max(1, Math.floor(lifeFrames || 1));
            embers.push({ x: x, y: y, dx: dx, dy: dy, r: radius, d: densityAmount, life: life, maxLife: life });
        }

        // Spawn new embers across the bottom band.
        function spawnEmbers() {
            for (var i = 0; i < emberRate; i++) {
                var width = Math.max(0.001, emberWidth);
                var x = 0.5 + (Math.random() - 0.5) * width;
                var y = emberOffset + Math.random() * emberHeight;
                var dx = (Math.random() - 0.5) * 0.1;
                var dy = 0.0;
                var edge = Math.abs(x - 0.5) / (width * 0.5);
                edge = Math.min(1.0, Math.max(0.0, edge));
                var edgeFactor = 1.0 - edge;
                var sizeScale = (1.0 - emberUniformity) + emberUniformity * edgeFactor;
                var radius = emberSize * sizeScale;
                var life = emberLifeFrames <= 1 ? 1 : (1 + Math.floor(Math.random() * emberLifeFrames));
                addEmber(x, y, dx, dy, radius, 1.5, life);
            }
        }

        // Re-apply embers each frame and decrement lifespan.
        function updateEmbers() {
            for (var i = embers.length - 1; i >= 0; i--) {
                var e = embers[i];
                addSplat(e.x, e.y, e.dx, e.dy, e.r, e.d);
                e.life -= 1;
                if (e.life <= 0) { embers.splice(i, 1); }
            }
        }

        // Spawn a spark line that rises and fades.
        function spawnSpark() {
            if (sparks.length > sparkMax) return;
            var width = Math.max(0.001, emberWidth);
            var innerWidth = width * 0.65;
            var x = 0.5 + (Math.random() - 0.5) * innerWidth;
            var midY = emberOffset + emberHeight * 0.5;
            var y = midY + (Math.random() - 0.5) * emberHeight;
            var baseVy = 1.5 + Math.random() * 1.0;
            var baseMax = 0.25 + Math.random() * 0.55;
            var maxY = Math.min(1.0, emberOffset + baseMax);
            var range = Math.max(0.05, maxY - emberOffset);
            var s = {
                x: x,
                y: y,
                px: x,
                py: y,
                startY: y,
                vx: (Math.random() - 0.5) * 0.3,
                vy: baseVy * (0.25 + (Math.random() * 0.65)),
                maxY: maxY,
                fadeStart: emberOffset + range * 0.7,
                alpha: 0.0,
                fading: false
            };
            sparks.push(s);
        }

        // Update spark positions, fading in/out over lifetime.
        function updateSparks(dtSec) {
            for (var i = sparks.length - 1; i >= 0; i--) {
                var s = sparks[i];
                s.px = s.x;
                s.py = s.y;
                s.vx += (Math.random() - 0.5) * 8.0 * dtSec;
                s.vx *= 0.95;
                s.x += s.vx * dtSec;
                s.y += s.vy * dtSec;
                s.vy -= 0.2 * dtSec;
                if (s.y >= s.maxY || s.y < emberOffset || s.x < 0.0 || s.x > 1.0) { sparks.splice(i, 1); continue; }
                var alpha = 1.0;
                var denom = Math.max(1e-4, (s.maxY - s.startY));
                var rise = (s.y - s.startY) / denom;
                if (rise < 0.25) { alpha = rise / 0.25; }
                var isFading = (s.vy < 0.0) || (s.y > s.fadeStart);
                if (s.vy < 0.0) {
                    // If the spark starts falling, fade out earlier as it drops.
                    var downStart = Math.max(s.fadeStart, s.y);
                    var downSpan = Math.max(1e-4, (downStart - emberOffset) * 0.25);
                    var fadeDown = 1.0 - (downStart - s.y) / downSpan;
                    alpha = Math.min(alpha, fadeDown);
                } else if (s.y > s.fadeStart) {
                    var fade = 1.0 - (s.y - s.fadeStart) / (s.maxY - s.fadeStart);
                    alpha = Math.min(alpha, fade);
                }
                alpha = Math.max(0.0, Math.min(1.0, alpha));
                if (!s.fading && isFading) { s.fading = true; }
                s.alpha = s.fading ? Math.min(s.alpha, alpha) : alpha;
            }
        }

        // Draw sparks as line segments with additive blending.
        function renderSparks() {
            if (sparks.length === 0) return;
            var verts = [];
            for (var i = 0; i < sparks.length; i++) {
                var s = sparks[i];
                verts.push(s.px, s.py, s.alpha);
                verts.push(s.x, s.y, s.alpha);
            }
            gl.bindBuffer(gl.ARRAY_BUFFER, sparkLineBuf);
            gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(verts), gl.DYNAMIC_DRAW);
            gl.useProgram(progSpark);
            var aPos = gl.getAttribLocation(progSpark, 'aPos');
            var aAlpha = gl.getAttribLocation(progSpark, 'aAlpha');
            gl.enableVertexAttribArray(aPos);
            gl.enableVertexAttribArray(aAlpha);
            gl.vertexAttribPointer(aPos, 2, gl.FLOAT, false, 12, 0);
            gl.vertexAttribPointer(aAlpha, 1, gl.FLOAT, false, 12, 8);
            gl.enable(gl.BLEND);
            gl.blendFunc(gl.SRC_ALPHA, gl.ONE);
            gl.drawArrays(gl.LINES, 0, sparks.length * 2);
            gl.disable(gl.BLEND);
            gl.disableVertexAttribArray(aPos);
            gl.disableVertexAttribArray(aAlpha);
        }

        canvas.addEventListener('mousedown', function (e) { pointer.down = true; pointer.x = e.clientX; pointer.y = e.clientY; pointer.px = pointer.x; pointer.py = pointer.y; });
        window.addEventListener('mouseup', function () { pointer.down = false; pointerEmber = null; });
        window.addEventListener('mousemove', function (e) { pointer.x = e.clientX; pointer.y = e.clientY; });
        canvas.addEventListener('touchstart', function (e) { if (e.touches.length) { var t = e.touches[0]; pointer.down = true; pointer.x = t.clientX; pointer.y = t.clientY; pointer.px = pointer.x; pointer.py = pointer.y; } });
        window.addEventListener('touchend', function () { pointer.down = false; pointerEmber = null; });
        window.addEventListener('touchmove', function (e) { if (e.touches.length) { var t = e.touches[0]; pointer.x = t.clientX; pointer.y = t.clientY; } });

        // Apply queued splats to density and velocity buffers.
        function applySplats() {
            while (splats.length) {
                var s = splats.shift();
                renderTo(density.write, progSplat, function (p) {
                    gl.activeTexture(gl.TEXTURE0);
                    gl.bindTexture(gl.TEXTURE_2D, density.read.texture);
                    gl.uniform1i(gl.getUniformLocation(p, 'uTarget'), 0);
                    gl.uniform2f(gl.getUniformLocation(p, 'point'), s.x, s.y);
                    gl.uniform1f(gl.getUniformLocation(p, 'radius'), s.r);
                    gl.uniform3f(gl.getUniformLocation(p, 'color'), s.d, s.d * 0.4, 0.0);
                });
                density.swap();

                renderTo(velocity.write, progSplatVel, function (p) {
                    gl.activeTexture(gl.TEXTURE0);
                    gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture);
                    gl.uniform1i(gl.getUniformLocation(p, 'uTarget'), 0);
                    gl.uniform2f(gl.getUniformLocation(p, 'point'), s.x, s.y);
                    gl.uniform1f(gl.getUniformLocation(p, 'radius'), s.r);
                    gl.uniform2f(gl.getUniformLocation(p, 'force'), s.dx, s.dy);
                    gl.uniform1f(gl.getUniformLocation(p, 'velRange'), velRange);
                });
                velocity.swap();
            }
        }

        // Advance simulation by one frame.
        function step(t) {
            var time = t * 0.001;
            var dtSec = dt;

            spawnEmbers();
            updateEmbers();
            if (Math.random() < sparkChance) { spawnSpark(); }
            updateSparks(dtSec);

            if (pointer.down) {
                var x = pointer.x / canvas.width;
                var y = 1.0 - pointer.y / canvas.height;
                var dx = (pointer.x - pointer.px) / canvas.width * 5.0;
                var dy = (pointer.py - pointer.y) / canvas.height * 5.0;
                var radius = emberSize * 0.75;
                var threshold = 0.003;
                if (pointerEmber) {
                    var dist = Math.abs(pointerEmber.x - x) + Math.abs(pointerEmber.y - y);
                    if (dist > threshold) {
                        pointerEmber = { x: x, y: y, dx: dx, dy: dy, r: radius, d: 3.0, life: emberLifeFrames, maxLife: emberLifeFrames };
                        embers.push(pointerEmber);
                    } else {
                        pointerEmber.x = x; pointerEmber.y = y;
                        pointerEmber.dx = dx; pointerEmber.dy = dy;
                        pointerEmber.life = Math.max(1, emberLifeFrames);
                    }
                } else {
                    pointerEmber = { x: x, y: y, dx: dx, dy: dy, r: radius, d: 3.0, life: emberLifeFrames, maxLife: emberLifeFrames };
                    embers.push(pointerEmber);
                }
                pointer.px = pointer.x; pointer.py = pointer.y;
            }

            applySplats();

            renderTo(velocity.write, progAdvectVel, function (p) {
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p, 'uVelocity'), 0);
                gl.uniform1f(gl.getUniformLocation(p, 'dt'), dtSec);
                gl.uniform1f(gl.getUniformLocation(p, 'dissipation'), velDissipation);
                gl.uniform2f(gl.getUniformLocation(p, 'texelSize'), 1.0 / simW, 1.0 / simH);
                gl.uniform1f(gl.getUniformLocation(p, 'rdx'), rdx);
                gl.uniform1f(gl.getUniformLocation(p, 'velRange'), velRange);
            }); velocity.swap();

            renderTo(velocity.write, progVort, function (p) {
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p, 'uVelocity'), 0);
                gl.uniform2f(gl.getUniformLocation(p, 'texelSize'), 1.0 / simW, 1.0 / simH);
                gl.uniform1f(gl.getUniformLocation(p, 'curl'), curl);
                gl.uniform1f(gl.getUniformLocation(p, 'dt'), dtSec);
                gl.uniform2f(gl.getUniformLocation(p, 'scale'), vortexScaleX, vortexScaleY);
                gl.uniform1f(gl.getUniformLocation(p, 'velRange'), velRange);
            }); velocity.swap();

            renderTo(velocity.write, progBuoy, function (p) {
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p, 'uVelocity'), 0);
                gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, density.read.texture); gl.uniform1i(gl.getUniformLocation(p, 'uDensity'), 1);
                gl.uniform1f(gl.getUniformLocation(p, 'strength'), buoyancy);
                gl.uniform1f(gl.getUniformLocation(p, 'maxHeat'), maxHeat);
                gl.uniform1f(gl.getUniformLocation(p, 'heatPower'), heatPower);
                gl.uniform1f(gl.getUniformLocation(p, 'minHeat'), minHeat);
                gl.uniform1f(gl.getUniformLocation(p, 'velRange'), velRange);
            }); velocity.swap();

            renderTo(divergenceFBO, progDiv, function (p) {
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p, 'uVelocity'), 0);
                gl.uniform2f(gl.getUniformLocation(p, 'texelSize'), 1.0 / simW, 1.0 / simH);
                gl.uniform1f(gl.getUniformLocation(p, 'velRange'), velRange);
            });

            gl.bindFramebuffer(gl.FRAMEBUFFER, pressure.read.framebuffer);
            gl.clearColor(0, 0, 0, 0); gl.clear(gl.COLOR_BUFFER_BIT);
            for (var i = 0; i < jacobi; i++) {
                renderTo(pressure.write, progJac, function (p) {
                    gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, pressure.read.texture); gl.uniform1i(gl.getUniformLocation(p, 'uPressure'), 0);
                    gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, divergenceFBO.texture); gl.uniform1i(gl.getUniformLocation(p, 'uDivergence'), 1);
                    gl.uniform2f(gl.getUniformLocation(p, 'texelSize'), 1.0 / simW, 1.0 / simH);
                });
                pressure.swap();
            }

            renderTo(velocity.write, progGrad, function (p) {
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, pressure.read.texture); gl.uniform1i(gl.getUniformLocation(p, 'uPressure'), 0);
                gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p, 'uVelocity'), 1);
                gl.uniform2f(gl.getUniformLocation(p, 'texelSize'), 1.0 / simW, 1.0 / simH);
                gl.uniform1f(gl.getUniformLocation(p, 'velRange'), velRange);
            }); velocity.swap();

            renderTo(density.write, progAdvect, function (p) {
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p, 'uVelocity'), 0);
                gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, density.read.texture); gl.uniform1i(gl.getUniformLocation(p, 'uSource'), 1);
                gl.uniform1f(gl.getUniformLocation(p, 'dt'), dtSec);
                gl.uniform1f(gl.getUniformLocation(p, 'dissipation'), denDissipation);
                gl.uniform2f(gl.getUniformLocation(p, 'texelSize'), 1.0 / simW, 1.0 / simH);
                gl.uniform1f(gl.getUniformLocation(p, 'rdx'), rdx);
                gl.uniform1f(gl.getUniformLocation(p, 'velRange'), velRange);
            }); density.swap();

            // Apply text after physics so it only affects visuals, not buoyancy
            if (textEnabled) {
                updateTextTexture();
                renderTo(density.write, progText, function (p) {
                    gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, density.read.texture); gl.uniform1i(gl.getUniformLocation(p, 'uTarget'), 0);
                    gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, textTexture); gl.uniform1i(gl.getUniformLocation(p, 'uText'), 1);
                    gl.uniform1f(gl.getUniformLocation(p, 'strength'), textStrength);
                    gl.uniform1f(gl.getUniformLocation(p, 'time'), time);
                    gl.uniform1f(gl.getUniformLocation(p, 'noiseScale'), textNoiseScale);
                    gl.uniform1f(gl.getUniformLocation(p, 'noiseFreq'), textNoiseFreq);
                    gl.uniform1f(gl.getUniformLocation(p, 'noiseSpeed'), textNoiseSpeed);
                    gl.uniform1f(gl.getUniformLocation(p, 'noiseType'), textNoiseType);
                    gl.uniform1f(gl.getUniformLocation(p, 'noiseDriftX'), textNoiseDriftX);
                    gl.uniform1f(gl.getUniformLocation(p, 'noiseDriftY'), textNoiseDriftY);
                    gl.uniform1f(gl.getUniformLocation(p, 'maskNoiseScale'), textMaskNoiseScale);
                    gl.uniform1f(gl.getUniformLocation(p, 'maskNoiseFreq'), textMaskNoiseFreq);
                    gl.uniform1f(gl.getUniformLocation(p, 'maskNoiseSpeed'), textMaskNoiseSpeed);
                    gl.uniform1f(gl.getUniformLocation(p, 'maskNoiseType'), textMaskNoiseType);
                    gl.uniform1f(gl.getUniformLocation(p, 'maskDriftX'), textMaskDriftX);
                    gl.uniform1f(gl.getUniformLocation(p, 'maskDriftY'), textMaskDriftY);
                    gl.uniform1f(gl.getUniformLocation(p, 'maskNoiseContrast'), textMaskNoiseContrast);
                    gl.uniform1f(gl.getUniformLocation(p, 'maskNoiseBrightnessTop'), textMaskNoiseBrightnessTop);
                    gl.uniform1f(gl.getUniformLocation(p, 'maskNoiseBrightnessBottom'), textMaskNoiseBrightnessBottom);
                    gl.uniform1f(gl.getUniformLocation(p, 'maskTextTop'), textMaskTopUv);
                    gl.uniform1f(gl.getUniformLocation(p, 'maskTextBottom'), textMaskBottomUv);
                });
                density.swap();
            }

            if (showTextPreviews) {
                renderNoisePreviewGPU(time);
                copyFBOToCanvas(previewNoiseFBO, textNoisePreviewCanvas);
                copyFBOToCanvas(previewMaskFBO, textMaskPreviewCanvas);
            }

            renderTo(null, progDisp, function (p) {
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, density.read.texture); gl.uniform1i(gl.getUniformLocation(p, 'uDensity'), 0);
                gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, paletteTexture); gl.uniform1i(gl.getUniformLocation(p, 'uPalette'), 1);
                gl.uniform2f(gl.getUniformLocation(p, 'resolution'), canvas.width, canvas.height);
                gl.uniform1f(gl.getUniformLocation(p, 'glow'), glow);
                gl.uniform2f(gl.getUniformLocation(p, 'vortexScale'), vortexScaleX, vortexScaleY);
                gl.uniform2f(gl.getUniformLocation(p, 'texelSize'), 1.0 / simW, 1.0 / simH);
            });

            renderSparks();
        }

        // Animation loop.
        function frame(t) { step(t); requestAnimationFrame(frame); }
        requestAnimationFrame(frame);
    }

    return { start: start };
})();

TestFire.start();
