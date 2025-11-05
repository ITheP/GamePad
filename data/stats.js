var countdown = 5;
var refreshInterval;
function updateTime() {
    let now = new Date();
    let timeString = now.getHours().toString().padStart(2, '0') + ':'
        + now.getMinutes().toString().padStart(2, '0') + ':'
        + now.getSeconds().toString().padStart(2, '0');
    document.getElementById('currentTime').innerText = timeString;
}

function toggleRefresh() {
    let checkbox = document.getElementById('refreshBox');
    localStorage.setItem('autoRefresh', checkbox.checked);
    if (checkbox.checked) {
        startAutoRefresh();
    } else {
        IntervalManager.clearAll();
        document.getElementById('countdownDisplay').innerHTML = '';
    }
}

var statsStatus = document.getElementById('countdownDisplay');
var fetchingStats = false;

function startAutoRefresh() {
    countdown = 5;

    IntervalManager.add(() => {
        if (fetchingStats == false) {
            try {
                if (countdown > 1) {
                    countdown--;
                    if (countdown < 6)
                        statsStatus.innerHTML = countdown + 's until refresh';
                }
                else if (countdown == 1) {
                    countdown--;    // Make sure this doesn't trigger while fetchStats is running
                    fetchStats();
                }
            }
            catch (error) {
                IntervalManager.clearAll();
                statsStatus.innerText = 'Error fetching stats: ' + (error?.message || error);
            }
        }
    }, 1000);
}

function fetchStats() {
    fetchingStats = true;

    statsStatus.innerText = 'Fetching latest stats...';

    fetch('/component/stats')
        .then(response => {
            if (!response.ok) { throw new Error('Network response was not ok'); }
            return response.text();
        })
        .then(html => {
            statsTable.innerHTML = html;
            countdown = 6;
            statsStatus.innerText = 'Stats updated';
            updateTime();

            fetchingStats = false;
        })
        .catch(() => {
            statsStatus.innerText = 'Fetching latest stats failed, will try again in a few seconds.';
            countdown = 8;

            fetchingStats = false;
        });
}

var savedState = localStorage.getItem('autoRefresh') !== 'false';
fetchStats();
document.getElementById('refreshBox').checked = savedState;
if (savedState) {
    startAutoRefresh();
}

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

    var vertexShaderSource = `
            attribute vec2 position;
            varying vec2 v_texCoord;
            void main() {
                v_texCoord =  position * 0.5 + 0.5;
                gl_Position = vec4(position, 0.0, 1.0);
            }
        `;

    var fragmentShaderSource = `
            precision mediump float;
            uniform sampler2D u_image;
            uniform float time;
            varying vec2 v_texCoord;

            void main() {
                // Create varying zoom based on position
                float zoomFactor = (sin(time * 0.17) + 1.0) + sin(v_texCoord.x * 5.0 + v_texCoord.y * 5.0 + time) * 0.1;

                // Adjust texture coordinates based on zoom level
                vec2 zoomedCoord = (v_texCoord - 0.5) * zoomFactor + 0.5;

                float angle = sin(time * 0.01) * 20.0; // Rotation speed
                float cosA = cos(angle);
                float sinA = sin(angle);
                mat2 rotationMatrix = mat2(cosA, -sinA, sinA, cosA);

                vec2 rotatedCoord = rotationMatrix * (zoomedCoord - 0.5) + 0.5;

                // Apply distortion waves
                float wave = sin(v_texCoord.y * 17.0 + time) * 0.02;
                float wave2 = sin(v_texCoord.x * 21.17 + (time * 1.517)) * 0.02;
                vec2 displacedCoord = rotatedCoord + vec2(wave, wave2);
                displacedCoord.y = 1.0 - displacedCoord.y;

                float offsetP = 0.3;
                float offsetM = -0.3;
                float offsetP2 = 0.6;
                float offsetM2 = -0.6;

                vec4 glowColor = vec4(0.0);
                float period = sin(v_texCoord.x + time * 0.1) * 15.0 + sin(v_texCoord.y + time * 0.17) * 15.0;
                float glowIntensity = ((sin(v_texCoord.x * period + v_texCoord.y * period + time) + 1.0) * 0.1) + 0.1;

                glowColor += texture2D(u_image, displacedCoord + vec2( offsetP, offsetP)) * glowIntensity;
                glowColor += texture2D(u_image, displacedCoord + vec2( offsetM, offsetP)) * glowIntensity;
                glowColor += texture2D(u_image, displacedCoord + vec2( offsetP, offsetM)) * glowIntensity;
                glowColor += texture2D(u_image, displacedCoord + vec2( offsetM, offsetM)) * glowIntensity;

                glowColor += texture2D(u_image, displacedCoord + vec2( offsetP2, offsetP2)) * glowIntensity;
                glowColor += texture2D(u_image, displacedCoord + vec2( offsetM2, offsetP2)) * glowIntensity;
                glowColor += texture2D(u_image, displacedCoord + vec2( offsetP2, offsetM2)) * glowIntensity;
                glowColor += texture2D(u_image, displacedCoord + vec2( offsetM2, offsetM2)) * glowIntensity;

                vec4 texColor = texture2D(u_image, displacedCoord);

                glowColor = max(texColor, glowColor);
                glowColor = glowColor - texColor;

                vec3 bgColor = vec3(0.0); // Black background
                vec3 finalColor = mix(bgColor.rgb, glowColor.rgb, texColor.a);

                // // Round vignette
                // vec2 center = vec2(0.5);
                // float dist = distance(v_texCoord, center);
                // float radius = 0.35;     // circle radius before fade starts
                // float softness = 0.15;    // controls the fade gradient
                // float vignette = smoothstep(radius, radius + softness, dist);
                // finalColor = mix(finalColor, vec3(0.0), vignette);

                // // Square vignette
                // vec2 edgeFade = smoothstep(vec2(0.0), vec2(0.2), v_texCoord) * smoothstep(vec2(1.0), vec2(0.8), v_texCoord);
                // float vignette = edgeFade.x * edgeFade.y;
                // finalColor = mix(vec3(0.0), finalColor, vignette);

                //gl_FragColor = vec4(v_texCoord.x, v_texCoord.y, 1.0, 1.0);
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

    // Throttle animation a bit, doesn't need to be mad
    var lastFrameTime = 0;
    var targetFPS = 60;
    var frameInterval = 1000 / targetFPS;

    var image = new Image();

    image.onload = () => {
        var buffer = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
        gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);
        var positionLocation = gl.getAttribLocation(program, "position");
        gl.enableVertexAttribArray(positionLocation);
        gl.vertexAttribPointer(positionLocation, 2, gl.FLOAT, false, 0, 0);
        // gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
        gl.activeTexture(gl.TEXTURE0);

        var texture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, texture);

        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);

        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image);
        //gl.generateMipmap(gl.TEXTURE_2D); // Ensure mipmaps are generated

        var textureLocation = gl.getUniformLocation(program, "u_image");
        gl.uniform1i(textureLocation, 0);

        function animate(now) {
            if (now - lastFrameTime >= frameInterval) {
                lastFrameTime = now;

                gl.uniform1f(timeLocation, now * 0.001);
                gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
            }

            animationFrameId = requestAnimationFrame(animate);
        }

        animate();
    }

    image.src = 'img/logo.' + Math.floor(Math.random() * 6) + '.png';   // logo.0-5.png

}

main();