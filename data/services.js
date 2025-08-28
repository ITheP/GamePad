function main() {
    // Vaguely clean up any existing effect
    cancelAnimationFrame(animationFrameId);
    document.body.querySelectorAll("canvas").forEach(canvas => canvas.remove());

    var canvas = document.createElement("canvas");
    var content = document.getElementById("content");
    canvas.style.position = 'fixed';
    canvas.style.zIndex = -1;
    canvas.style.top = 0;
    canvas.style.left = 0;
    content.appendChild(canvas);

    var gl = canvas.getContext('webgl');

    if (!gl) {
        console.log('WebGL not supported');
        return;
    }

    // Resize canvas
    function resizeCanvas() {
        canvas.width = window.innerWidth;
        canvas.height = window.innerHeight;
        gl.viewport(0, 0, canvas.width, canvas.height);
    }

    resizeCanvas();
    ResizeManager.attach(resizeCanvas);

    // Vertex Shader Source
    var vertexShaderSource = `
            attribute vec2 position;
            varying vec2 v_texCoord;
            void main() {
                v_texCoord = position * 0.5 + 0.5;
                gl_Position = vec4(position, 0.0, 1.0);
            }
        `;

    // Fragment Shader Source (Generates Color Wave Effect)
    var fragmentShaderSource = `
            precision mediump float;
            varying vec2 v_texCoord;
            uniform float time;

            void main() {
                float contrast = 1.0; // 0.3;
                
                // Apply zoom-out effect
                vec2 zoomUvR = v_texCoord * 8.0;
                float wave = sin(zoomUvR.x) + sin(time * 1.17);
                float offsetR = sin(zoomUvR.x + sin(time)) + sin(zoomUvR.y + wave) * (cos(wave + time));
                float r = (sin((time + zoomUvR.y) * 0.5) * offsetR) + 0.5;
                r *= contrast;

                float time2 = time + 1.141;
                vec2 zoomUvG = v_texCoord * 6.0;
                wave = sin(zoomUvG.x) + sin(time2 * 0.49);
                float offsetG = sin(zoomUvG.x + sin(time2)) + sin(zoomUvG.y + wave) * (cos(wave + time2));
                float g = (sin((time2 + zoomUvG.y) * 0.5) * offsetG) + 0.5;
                g *= contrast;

                float time3 = time + 2.141;
                vec2 zoomUvB = v_texCoord * 4.0;
                wave = sin(zoomUvB.x) + sin(time3 * 0.78);
                float offsetB = sin(zoomUvB.x + sin(time3)) + sin(zoomUvB.y + wave) * (cos(wave + time3));
                float b = (sin((time3 + zoomUvB.y) * 0.5) * offsetB) + 0.5;
                b *= contrast;

                vec3 color = vec3(r, g, b);

                // Round vignette
                vec2 center = vec2(0.5);
                float dist = distance(v_texCoord, center);
                float radius = 0.35;     // circle radius before fade starts
                float softness = 0.15;    // controls the fade gradient
                float vignette = smoothstep(radius, radius + softness, dist);
                color = mix(color, vec3(0.0), vignette);

                // // Square vignette
                // vec2 edgeFade = smoothstep(vec2(0.0), vec2(0.2), v_texCoord) * smoothstep(vec2(1.0), vec2(0.8), v_texCoord);
                // float vignette = edgeFade.x * edgeFade.y;
                // color = mix(vec3(0.0), color, vignette);

                gl_FragColor = vec4(color, 1.0);
            }
        `;

    // Define Quad Geometry (Full-Screen Triangle Strip)
    var vertices = new Float32Array([
        -1, -1, 1, -1, -1, 1, 1, 1
    ]);

    // Compile Shader Function
    function createShader(type, source) {
        var shader = gl.createShader(type);
        gl.shaderSource(shader, source);
        gl.compileShader(shader);
        if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
            console.error('Shader compilation failed:', gl.getShaderInfoLog(shader));
            gl.deleteShader(shader);
            return null;
        }
        return shader;
    }

    // Create Program & Attach Shaders
    var vertexShader = createShader(gl.VERTEX_SHADER, vertexShaderSource);
    var fragmentShader = createShader(gl.FRAGMENT_SHADER, fragmentShaderSource);

    var program = gl.createProgram();
    gl.attachShader(program, vertexShader);
    gl.attachShader(program, fragmentShader);
    gl.linkProgram(program);
    gl.useProgram(program);

    // Handle Time Uniform
    var timeLocation = gl.getUniformLocation(program, "time");

    // Throttle animation a bit, doesn't need to be mad
    var lastFrameTime = 0;
    var targetFPS = 60;
    var frameInterval = 1000 / targetFPS;  // ~33.3ms per frame

    var buffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
    gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);
    var positionLocation = gl.getAttribLocation(program, "position");
    gl.enableVertexAttribArray(positionLocation);
    gl.vertexAttribPointer(positionLocation, 2, gl.FLOAT, false, 0, 0);


    function animate(now) {
        if (now - lastFrameTime >= frameInterval) {
            lastFrameTime = now;

            // Update shader time uniform
            gl.uniform1f(timeLocation, now * 0.001);
            gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
        }

        animationFrameId = requestAnimationFrame(animate);
    }

    animate();
}

main();