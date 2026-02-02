/* GPU fire simulation with WebGL-based fluid dynamics */
var TestFire = (function(){
    function $(id){ return document.getElementById(id); }

    function start(){
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
        if(!gl){ console.error('WebGL not available'); return; }

        var floatTex = gl.getExtension('OES_texture_float');
        var floatLinear = gl.getExtension('OES_texture_float_linear');
        var floatColor = gl.getExtension('WEBGL_color_buffer_float');

        var textureType = gl.UNSIGNED_BYTE;
        if(floatTex && floatColor){ textureType = gl.FLOAT; }

        var vs = '\nattribute vec2 position;\nvarying vec2 v_uv;\nvoid main(){ v_uv = position * 0.5 + 0.5; gl_Position = vec4(position, 0.0, 1.0); }\n';
        var common = 'precision highp float;\nvarying vec2 v_uv;\n';

        var fs_advect = common +
            'uniform sampler2D uVelocity; uniform sampler2D uSource; uniform float dt; uniform float dissipation; uniform vec2 texelSize; uniform float rdx;\n' +
            'vec2 decode(vec2 v){ return v*2.0-1.0; }\n' +
            'void main(){ vec2 uv=v_uv; vec2 vel=decode(texture2D(uVelocity,uv).xy); vec2 prev=uv-dt*rdx*vel; gl_FragColor = texture2D(uSource, prev) * dissipation; }\n';

        var fs_div = common +
            'uniform sampler2D uVelocity; uniform vec2 texelSize;\n' +
            'vec2 decode(vec2 v){ return v*2.0-1.0; }\n' +
            'void main(){ vec2 uv=v_uv; float vl=decode(texture2D(uVelocity,uv-vec2(texelSize.x,0.0)).xy).x; float vr=decode(texture2D(uVelocity,uv+vec2(texelSize.x,0.0)).xy).x; float vb=decode(texture2D(uVelocity,uv-vec2(0.0,texelSize.y)).xy).y; float vt=decode(texture2D(uVelocity,uv+vec2(0.0,texelSize.y)).xy).y; float d=0.5*(vr-vl+vt-vb); gl_FragColor=vec4(d,0.0,0.0,1.0); }\n';

        var fs_jacobi = common +
            'uniform sampler2D uPressure; uniform sampler2D uDivergence; uniform vec2 texelSize;\n' +
            'void main(){ vec2 uv=v_uv; float pl=texture2D(uPressure,uv-vec2(texelSize.x,0.0)).x; float pr=texture2D(uPressure,uv+vec2(texelSize.x,0.0)).x; float pb=texture2D(uPressure,uv-vec2(0.0,texelSize.y)).x; float pt=texture2D(uPressure,uv+vec2(0.0,texelSize.y)).x; float b=texture2D(uDivergence,uv).x; float p=(pl+pr+pb+pt-b)*0.25; gl_FragColor=vec4(p,0.0,0.0,1.0); }\n';

        var fs_grad = common +
            'uniform sampler2D uPressure; uniform sampler2D uVelocity; uniform vec2 texelSize;\n' +
            'vec2 decode(vec2 v){ return v*2.0-1.0; }\n' +
            'vec2 encode(vec2 v){ return clamp(v*0.5+0.5, 0.0, 1.0); }\n' +
            'void main(){ vec2 uv=v_uv; float pl=texture2D(uPressure,uv-vec2(texelSize.x,0.0)).x; float pr=texture2D(uPressure,uv+vec2(texelSize.x,0.0)).x; float pb=texture2D(uPressure,uv-vec2(0.0,texelSize.y)).x; float pt=texture2D(uPressure,uv+vec2(0.0,texelSize.y)).x; vec2 g=vec2(pr-pl,pt-pb)*0.5; vec2 vel=decode(texture2D(uVelocity,uv).xy); vel-=g; gl_FragColor=vec4(encode(vel),0.0,1.0); }\n';

        var fs_vort = common +
            'uniform sampler2D uVelocity; uniform vec2 texelSize; uniform float curl; uniform float dt; uniform float scale;\n' +
            'vec2 decode(vec2 v){ return v*2.0-1.0; }\n' +
            'vec2 encode(vec2 v){ return clamp(v*0.5+0.5, 0.0, 1.0); }\n' +
            'float getCurl(vec2 uv, vec2 dx){ vec2 vl=decode(texture2D(uVelocity,uv-vec2(dx.x,0.0)).xy); vec2 vr=decode(texture2D(uVelocity,uv+vec2(dx.x,0.0)).xy); vec2 vb=decode(texture2D(uVelocity,uv-vec2(0.0,dx.y)).xy); vec2 vt=decode(texture2D(uVelocity,uv+vec2(0.0,dx.y)).xy); return ((vr.y-vl.y)-(vt.x-vb.x))*0.5; }\n' +
            'void main(){ vec2 uv=v_uv; vec2 dx=texelSize*scale; float cL=getCurl(uv-vec2(dx.x,0.0),dx); float cR=getCurl(uv+vec2(dx.x,0.0),dx); float cB=getCurl(uv-vec2(0.0,dx.y),dx); float cT=getCurl(uv+vec2(0.0,dx.y),dx); float cC=getCurl(uv,dx); vec2 grad=vec2(abs(cR)-abs(cL), abs(cT)-abs(cB)); float len=length(grad)+1e-5; vec2 n=grad/len; vec2 f=vec2(n.y,-n.x)*cC*curl*dt; vec2 vel=decode(texture2D(uVelocity,uv).xy); vel+=f; gl_FragColor=vec4(encode(vel),0.0,1.0); }\n';

        var fs_buoy = common +
            'uniform sampler2D uVelocity; uniform sampler2D uDensity; uniform float strength; uniform float dt;\n' +
            'vec2 decode(vec2 v){ return v*2.0-1.0; }\n' +
            'vec2 encode(vec2 v){ return clamp(v*0.5+0.5, 0.0, 1.0); }\n' +
            'void main(){ vec2 uv=v_uv; float d=texture2D(uDensity,uv).r; vec2 vel=decode(texture2D(uVelocity,uv).xy); float lift=clamp(d,0.0,1.0)*strength*dt; vel.y += lift; gl_FragColor=vec4(encode(vel),0.0,1.0); }\n';

        var fs_splat = common +
            'uniform sampler2D uTarget; uniform vec2 point; uniform float radius; uniform vec3 color;\n' +
            'void main(){ vec2 uv=v_uv; vec4 cur=texture2D(uTarget,uv); float d=distance(uv,point); float a=exp(-d*d/(radius*radius)); cur.rgb += color * a; gl_FragColor=cur; }\n';

        var fs_splat_vel = common +
            'uniform sampler2D uTarget; uniform vec2 point; uniform float radius; uniform vec2 force;\n' +
            'vec2 encode(vec2 v){ return clamp(v*0.5+0.5, 0.0, 1.0); }\n' +
            'vec2 decode(vec2 v){ return v*2.0-1.0; }\n' +
            'void main(){ vec2 uv=v_uv; vec4 cur=texture2D(uTarget,uv); float d=distance(uv,point); float a=exp(-d*d/(radius*radius)); vec2 vel=decode(cur.xy); vel += force * a; gl_FragColor=vec4(encode(vel),0.0,1.0); }\n';

        var fs_display = common +
            'uniform sampler2D uDensity; uniform vec2 resolution; uniform float glow;\n' +
            'vec3 palette(float x){ vec3 c1=vec3(0.05,0.0,0.0); vec3 c2=vec3(0.8,0.15,0.02); vec3 c3=vec3(1.0,0.6,0.1); vec3 c4=vec3(1.0,0.95,0.8); vec3 col=mix(c1,c2,smoothstep(0.0,0.4,x)); col=mix(col,c3,smoothstep(0.3,0.8,x)); col=mix(col,c4,smoothstep(0.6,1.0,x)); return col; }\n' +
            'void main(){ vec2 uv=v_uv; float d=texture2D(uDensity,uv).r; vec3 col=palette(d); vec2 px=1.0/resolution * (1.0 + glow*1.5); float g=0.0; for(int i=-2;i<=2;i++){ for(int j=-2;j<=2;j++){ g += texture2D(uDensity, uv + vec2(float(i),float(j))*px).r; } } g *= 0.02*glow; col += vec3(1.0,0.6,0.2)*g; col = pow(col, vec3(0.85)); gl_FragColor=vec4(col,1.0); }\n';

        function createShader(type, src){
            var s = gl.createShader(type);
            gl.shaderSource(s, src);
            gl.compileShader(s);
            if(!gl.getShaderParameter(s, gl.COMPILE_STATUS)){
                console.error(gl.getShaderInfoLog(s));
                gl.deleteShader(s);
                return null;
            }
            return s;
        }

        var vsObj = createShader(gl.VERTEX_SHADER, vs);
        function createProgram(fsSrc){
            var fsObj = createShader(gl.FRAGMENT_SHADER, fsSrc);
            var p = gl.createProgram();
            gl.attachShader(p, vsObj);
            gl.attachShader(p, fsObj);
            gl.linkProgram(p);
            if(!gl.getProgramParameter(p, gl.LINK_STATUS)){
                console.error(gl.getProgramInfoLog(p));
                return null;
            }
            return p;
        }

        var progAdvect = createProgram(fs_advect);
        var progDiv = createProgram(fs_div);
        var progJac = createProgram(fs_jacobi);
        var progGrad = createProgram(fs_grad);
        var progVort = createProgram(fs_vort);
        var progBuoy = createProgram(fs_buoy);
        var progSplat = createProgram(fs_splat);
        var progSplatVel = createProgram(fs_splat_vel);
        var progDisp = createProgram(fs_display);

        function createFBO(w, h){
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

        function createDouble(w, h){
            return { read: createFBO(w,h), write: createFBO(w,h), swap: function(){ var t=this.read; this.read=this.write; this.write=t; } };
        }

        var simScale = 1.25;
        var simW = 0, simH = 0;
        var velocity, density, pressure, divergenceFBO;

        function clearFBO(fb, r, g, b, a){
            gl.bindFramebuffer(gl.FRAMEBUFFER, fb.framebuffer);
            gl.clearColor(r, g, b, a);
            gl.clear(gl.COLOR_BUFFER_BIT);
        }

        function allocFBOs(){
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
        }

        var quadBuf = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, quadBuf);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1,-1, 1,-1, -1,1, 1,1]), gl.STATIC_DRAW);

        function bindQuad(prog){
            var loc = gl.getAttribLocation(prog, 'position');
            gl.bindBuffer(gl.ARRAY_BUFFER, quadBuf);
            if(loc >= 0){ gl.enableVertexAttribArray(loc); gl.vertexAttribPointer(loc, 2, gl.FLOAT, false, 0, 0); }
        }

        function setViewport(target){
            if(target){ gl.bindFramebuffer(gl.FRAMEBUFFER, target.framebuffer); gl.viewport(0,0,target.width,target.height); }
            else { gl.bindFramebuffer(gl.FRAMEBUFFER, null); gl.viewport(0,0,canvas.width,canvas.height); }
        }

        function renderTo(target, prog, setUniforms){
            if(!prog) return;
            gl.useProgram(prog);
            bindQuad(prog);
            if(setUniforms) setUniforms(prog);
            setViewport(target);
            gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
        }

        function resize(){
            canvas.width = window.innerWidth;
            canvas.height = window.innerHeight;
            gl.viewport(0,0,canvas.width,canvas.height);
            allocFBOs();
        }
        window.addEventListener('resize', resize);
        resize();

        var dt = 1/60;
        var jacobi = 32;
        var curl = 30;
        var vortexScale = 8.0;
        var buoyancy = 20;
        var glow = 1.2;
        var fireSpeed = 1.2;
        var emberRate = 24;
        var velDissipation = 0.99;
        var denDissipation = 0.985;
        var sparkChance = 0.12;
        var sparkMax = 64;
        var rdx = 0.012;
        var pointer = { down:false, x:0, y:0, px:0, py:0 };
        var splats = [];
        var sparks = [];

        // UI controls
        var ctrl = document.createElement('div');
        ctrl.style.position = 'fixed';
        ctrl.style.right = '8px';
        ctrl.style.top = '8px';
        ctrl.style.zIndex = 9999;
        ctrl.style.background = 'rgba(0,0,0,0.45)';
        ctrl.style.color = '#fff';
        ctrl.style.padding = '8px 10px';
        ctrl.style.fontFamily = 'sans-serif';
        ctrl.style.fontSize = '12px';
        ctrl.style.borderRadius = '6px';
        ctrl.style.userSelect = 'none';
        function addSlider(label, min, max, step, value, onChange){
            var row = document.createElement('div');
            row.style.marginBottom = '6px';
            var l = document.createElement('label');
            l.textContent = label;
            l.style.display = 'block';
            l.style.marginBottom = '2px';
            var input = document.createElement('input');
            input.type = 'range';
            input.min = min; input.max = max; input.step = step; input.value = value;
            input.style.width = '170px';
            var valueSpan = document.createElement('span');
            valueSpan.textContent = value;
            valueSpan.style.marginLeft = '8px';
            input.oninput = function(){ valueSpan.textContent = input.value; onChange(parseFloat(input.value)); };
            row.appendChild(l);
            row.appendChild(input);
            row.appendChild(valueSpan);
            ctrl.appendChild(row);
        }
        content.appendChild(ctrl);
        addSlider('quality (sim scale)', 0.5, 2.0, 0.05, simScale, function(v){ simScale = v; allocFBOs(); });
        addSlider('pressure iterations', 6, 48, 1, jacobi, function(v){ jacobi = Math.floor(v); });
        addSlider('vorticity', 0.0, 100.0, 1.0, curl, function(v){ curl = v; });
        addSlider('vortex scale', 1.0, 20.0, 0.5, vortexScale, function(v){ vortexScale = v; });
        addSlider('buoyancy (rise)', 0.0, 80.0, 1.0, buoyancy, function(v){ buoyancy = v; });
        addSlider('advection speed', 0.001, 0.05, 0.001, rdx, function(v){ rdx = v; });
        addSlider('fire speed', 0.2, 3.0, 0.05, fireSpeed, function(v){ fireSpeed = v; });
        addSlider('embers per frame', 1, 40, 1, emberRate, function(v){ emberRate = Math.floor(v); });
        addSlider('velocity dissipation', 0.95, 0.999, 0.001, velDissipation, function(v){ velDissipation = v; });
        addSlider('density dissipation', 0.95, 0.999, 0.001, denDissipation, function(v){ denDissipation = v; });
        addSlider('spark chance', 0.0, 0.3, 0.005, sparkChance, function(v){ sparkChance = v; });
        addSlider('glow', 0.0, 2.0, 0.01, glow, function(v){ glow = v; });

        function addSplat(x, y, dx, dy, radius, densityAmount){
            splats.push({ x:x, y:y, dx:dx, dy:dy, r:radius, d:densityAmount });
        }

        function spawnEmbers(){
            for(var i=0;i<emberRate;i++){
                var x = 0.5 + (Math.random()-0.5)*0.5;
                var y = Math.random()*0.03;
                var dx = (Math.random()-0.5)*0.15;
                var dy = fireSpeed * (1.0 + Math.random()*0.5);
                addSplat(x, y, dx, dy, 0.025, 2.5);
            }
        }

        function spawnSpark(){
            if(sparks.length > sparkMax) return;
            var s = {
                x: 0.5 + (Math.random()-0.5)*0.4,
                y: 0.01 + Math.random()*0.02,
                vx: (Math.random()-0.5)*0.25,
                vy: 1.2 + Math.random()*0.8,
                life: 1.0
            };
            sparks.push(s);
        }

        function updateSparks(dtSec){
            for(var i=sparks.length-1;i>=0;i--){
                var s = sparks[i];
                s.x += s.vx * dtSec;
                s.y += s.vy * dtSec;
                s.vy -= 0.15 * dtSec;
                s.life -= dtSec*0.5;
                if(s.life <= 0 || s.y > 1.1){ sparks.splice(i,1); continue; }
                addSplat(s.x, s.y, s.vx*0.3, s.vy*0.3, 0.02, 2.2*s.life);
            }
        }

        canvas.addEventListener('mousedown', function(e){ pointer.down=true; pointer.x=e.clientX; pointer.y=e.clientY; pointer.px=pointer.x; pointer.py=pointer.y; });
        window.addEventListener('mouseup', function(){ pointer.down=false; });
        window.addEventListener('mousemove', function(e){ pointer.x=e.clientX; pointer.y=e.clientY; });
        canvas.addEventListener('touchstart', function(e){ if(e.touches.length){ var t=e.touches[0]; pointer.down=true; pointer.x=t.clientX; pointer.y=t.clientY; pointer.px=pointer.x; pointer.py=pointer.y; } });
        window.addEventListener('touchend', function(){ pointer.down=false; });
        window.addEventListener('touchmove', function(e){ if(e.touches.length){ var t=e.touches[0]; pointer.x=t.clientX; pointer.y=t.clientY; } });

        function applySplats(){
            while(splats.length){
                var s = splats.shift();
                renderTo(density.write, progSplat, function(p){
                    gl.activeTexture(gl.TEXTURE0);
                    gl.bindTexture(gl.TEXTURE_2D, density.read.texture);
                    gl.uniform1i(gl.getUniformLocation(p,'uTarget'),0);
                    gl.uniform2f(gl.getUniformLocation(p,'point'), s.x, s.y);
                    gl.uniform1f(gl.getUniformLocation(p,'radius'), s.r);
                    gl.uniform3f(gl.getUniformLocation(p,'color'), s.d, s.d*0.4, 0.0);
                });
                density.swap();

                renderTo(velocity.write, progSplatVel, function(p){
                    gl.activeTexture(gl.TEXTURE0);
                    gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture);
                    gl.uniform1i(gl.getUniformLocation(p,'uTarget'),0);
                    gl.uniform2f(gl.getUniformLocation(p,'point'), s.x, s.y);
                    gl.uniform1f(gl.getUniformLocation(p,'radius'), s.r);
                    gl.uniform2f(gl.getUniformLocation(p,'force'), s.dx, s.dy);
                });
                velocity.swap();
            }
        }

        function step(t){
            var time = t*0.001;
            var dtSec = dt;

            spawnEmbers();
            if(Math.random() < sparkChance){ spawnSpark(); }
            updateSparks(dtSec);

            if(pointer.down){
                var x = pointer.x / canvas.width;
                var y = 1.0 - pointer.y / canvas.height;
                var dx = (pointer.x - pointer.px) / canvas.width * 5.0;
                var dy = (pointer.py - pointer.y) / canvas.height * 5.0;
                addSplat(x, y, dx, dy + fireSpeed * 1.5, 0.05, 3.0);
                pointer.px = pointer.x; pointer.py = pointer.y;
            }

            applySplats();

            renderTo(velocity.write, progAdvect, function(p){
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uVelocity'),0);
                gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uSource'),1);
                gl.uniform1f(gl.getUniformLocation(p,'dt'), dtSec);
                gl.uniform1f(gl.getUniformLocation(p,'dissipation'), velDissipation);
                gl.uniform2f(gl.getUniformLocation(p,'texelSize'), 1.0/simW, 1.0/simH);
                gl.uniform1f(gl.getUniformLocation(p,'rdx'), rdx);
            }); velocity.swap();

            renderTo(velocity.write, progVort, function(p){
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uVelocity'),0);
                gl.uniform2f(gl.getUniformLocation(p,'texelSize'), 1.0/simW, 1.0/simH);
                gl.uniform1f(gl.getUniformLocation(p,'curl'), curl);
                gl.uniform1f(gl.getUniformLocation(p,'dt'), dtSec);
                gl.uniform1f(gl.getUniformLocation(p,'scale'), vortexScale);
            }); velocity.swap();

            renderTo(velocity.write, progBuoy, function(p){
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uVelocity'),0);
                gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, density.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uDensity'),1);
                gl.uniform1f(gl.getUniformLocation(p,'strength'), buoyancy);
                gl.uniform1f(gl.getUniformLocation(p,'dt'), dtSec);
            }); velocity.swap();

            renderTo(divergenceFBO, progDiv, function(p){
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uVelocity'),0);
                gl.uniform2f(gl.getUniformLocation(p,'texelSize'), 1.0/simW, 1.0/simH);
            });

            gl.bindFramebuffer(gl.FRAMEBUFFER, pressure.read.framebuffer);
            gl.clearColor(0,0,0,0); gl.clear(gl.COLOR_BUFFER_BIT);
            for(var i=0;i<jacobi;i++){
                renderTo(pressure.write, progJac, function(p){
                    gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, pressure.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uPressure'),0);
                    gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, divergenceFBO.texture); gl.uniform1i(gl.getUniformLocation(p,'uDivergence'),1);
                    gl.uniform2f(gl.getUniformLocation(p,'texelSize'), 1.0/simW, 1.0/simH);
                });
                pressure.swap();
            }

            renderTo(velocity.write, progGrad, function(p){
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, pressure.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uPressure'),0);
                gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uVelocity'),1);
                gl.uniform2f(gl.getUniformLocation(p,'texelSize'), 1.0/simW, 1.0/simH);
            }); velocity.swap();

            renderTo(density.write, progAdvect, function(p){
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uVelocity'),0);
                gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, density.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uSource'),1);
                gl.uniform1f(gl.getUniformLocation(p,'dt'), dtSec);
                gl.uniform1f(gl.getUniformLocation(p,'dissipation'), denDissipation);
                gl.uniform2f(gl.getUniformLocation(p,'texelSize'), 1.0/simW, 1.0/simH);
                gl.uniform1f(gl.getUniformLocation(p,'rdx'), rdx);
            }); density.swap();

            renderTo(null, progDisp, function(p){
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, density.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uDensity'),0);
                gl.uniform2f(gl.getUniformLocation(p,'resolution'), canvas.width, canvas.height);
                gl.uniform1f(gl.getUniformLocation(p,'glow'), glow);
            });
        }

        function frame(t){ step(t); requestAnimationFrame(frame); }
        requestAnimationFrame(frame);
    }

    return { start: start };
})();

TestFire.start();
