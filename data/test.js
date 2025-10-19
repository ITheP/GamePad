function main() {
    // Vaguely clean up any existing effect
   // cancelAnimationFrame(animationFrameId);
   // document.body.querySelectorAll("canvas").forEach(canvas => canvas.remove());

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
    //////ResizeManager.attach(resizeCanvas);
    window.addEventListener('resize', resizeCanvas);

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

uniform float time;
uniform vec2 resolution;
uniform vec2 mouse;
uniform sampler2D prevFrame;

varying vec2 v_texCoord;

void main() {
    vec2 uv = v_texCoord;
    float t = time;

    // Read previous frame and fade it slightly
    vec4 prev = texture2D(prevFrame, uv);
    vec3 faded = prev.rgb * 0.096;

    // Flame rising from bottom
    float y = uv.y - t * 0.2;
    float baseFlame = exp(-10.0 * abs(uv.x - 0.5)) * pow(1.0 - y, 3.0);
    baseFlame *= smoothstep(0.0, 0.3, uv.y); // only near bottom

    // Mouse-injected flame
    float dist = distance(uv, mouse);
    float mouseFlame = exp(-dist * 40.0);

    // Total flame intensity
    float f = baseFlame + mouseFlame;

    // Fire color gradient
    vec3 fire = vec3(0.0);
    fire.r = smoothstep(0.0, 1.0, f * 2.0);
    fire.g = smoothstep(0.2, 0.8, f);
    fire.b = smoothstep(0.5, 0.8, f * 0.5);

    // Combine with faded previous frame
    vec3 finalColor = clamp(faded + fire, 0.0, 1.0);

    gl_FragColor = vec4(finalColor, 1.0);
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
    var resolutionLocation = gl.getUniformLocation(program, "resolution");
    var mouseLocation = gl.getUniformLocation(program, "mouse");

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

    var mouse = [0.5, 0.0];
    canvas.addEventListener('mousemove', function (e) {
        mouse[0] = e.clientX / canvas.width;
        mouse[1] = 1.0 - e.clientY / canvas.height;
    });


    function createFramebufferTexture() {
        const tex = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, tex);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, canvas.width, canvas.height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

        const fb = gl.createFramebuffer();
        gl.bindFramebuffer(gl.FRAMEBUFFER, fb);
        gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, tex, 0);

        return { texture: tex, framebuffer: fb };
    }

    let bufferA = createFramebufferTexture();
    let bufferB = createFramebufferTexture();

var prevFrameLocation = gl.getUniformLocation(program, "prevFrame");

    function animate(now) {
    if (now - lastFrameTime >= frameInterval) {
        lastFrameTime = now;

        gl.useProgram(program);

        // Update uniforms
        gl.uniform1f(timeLocation, now * 0.001);
        gl.uniform2f(resolutionLocation, canvas.width, canvas.height);
        gl.uniform2f(mouseLocation, mouse[0], mouse[1]);

        // Bind bufferA as input texture
        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, bufferA.texture);
        gl.uniform1i(prevFrameLocation, 0);

        // Render to bufferB
        gl.bindFramebuffer(gl.FRAMEBUFFER, bufferB.framebuffer);
        gl.viewport(0, 0, canvas.width, canvas.height);
        gl.clearColor(0.0, 0.0, 0.0, 1.0); // clear to black
        gl.clear(gl.COLOR_BUFFER_BIT);
        gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

        // Render bufferB to screen
        gl.bindFramebuffer(gl.FRAMEBUFFER, null);
        gl.viewport(0, 0, canvas.width, canvas.height);
        gl.bindTexture(gl.TEXTURE_2D, bufferB.texture);
        gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

        // Swap buffers
        let temp = bufferA;
        bufferA = bufferB;
        bufferB = temp;
    }



        //animationFrameId = requestAnimationFrame(animate);
        requestAnimationFrame(animate);
    }

    animate();
}

main();


