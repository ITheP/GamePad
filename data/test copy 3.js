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
            'uniform sampler2D uVelocity; uniform sampler2D uSource; uniform float dt; uniform float dissipation; uniform vec2 texelSize; uniform float rdx; uniform float velRange;\n' +
            'vec2 decode(vec2 v, float r){ return v*r*2.0-r; }\n' +
            'void main(){ vec2 uv=v_uv; vec2 vel=decode(texture2D(uVelocity,uv).xy, velRange); float scale=min(texelSize.x,texelSize.y)*rdx; vec2 prev=uv-dt*vel*scale; gl_FragColor = texture2D(uSource, prev) * dissipation; }\n';
        var fs_advect_vel = common +
            'uniform sampler2D uVelocity; uniform float dt; uniform float dissipation; uniform vec2 texelSize; uniform float rdx; uniform float velRange;\n' +
            'vec2 decode(vec2 v, float r){ return v*r*2.0-r; }\n' +
            'vec2 encode(vec2 v, float r){ return clamp(v/r*0.5+0.5, 0.0, 1.0); }\n' +
            'void main(){ vec2 uv=v_uv; vec2 vel=decode(texture2D(uVelocity,uv).xy, velRange); float scale=min(texelSize.x,texelSize.y)*rdx; vec2 prev=uv-dt*vel*scale; vec2 sampled=decode(texture2D(uVelocity,prev).xy, velRange); sampled *= dissipation; gl_FragColor = vec4(encode(sampled, velRange),0.0,1.0); }\n';
        var fs_div = common +
            'uniform sampler2D uVelocity; uniform vec2 texelSize; uniform float velRange;\n' +
            'vec2 decode(vec2 v, float r){ return v*r*2.0-r; }\n' +
            'void main(){ vec2 uv=v_uv; float vl=decode(texture2D(uVelocity,uv-vec2(texelSize.x,0.0)).xy, velRange).x; float vr=decode(texture2D(uVelocity,uv+vec2(texelSize.x,0.0)).xy, velRange).x; float vb=decode(texture2D(uVelocity,uv-vec2(0.0,texelSize.y)).xy, velRange).y; float vt=decode(texture2D(uVelocity,uv+vec2(0.0,texelSize.y)).xy, velRange).y; float d=0.5*(vr-vl+vt-vb); gl_FragColor=vec4(d,0.0,0.0,1.0); }\n';

        var fs_jacobi = common +
            'uniform sampler2D uPressure; uniform sampler2D uDivergence; uniform vec2 texelSize;\n' +
            'void main(){ vec2 uv=v_uv; float pl=texture2D(uPressure,uv-vec2(texelSize.x,0.0)).x; float pr=texture2D(uPressure,uv+vec2(texelSize.x,0.0)).x; float pb=texture2D(uPressure,uv-vec2(0.0,texelSize.y)).x; float pt=texture2D(uPressure,uv+vec2(0.0,texelSize.y)).x; float b=texture2D(uDivergence,uv).x; float p=(pl+pr+pb+pt-b)*0.25; gl_FragColor=vec4(p,0.0,0.0,1.0); }\n';

        var fs_grad = common +
            'uniform sampler2D uPressure; uniform sampler2D uVelocity; uniform vec2 texelSize; uniform float velRange;\n' +
            'vec2 decode(vec2 v, float r){ return v*r*2.0-r; }\n' +
            'vec2 encode(vec2 v, float r){ return clamp(v/r*0.5+0.5, 0.0, 1.0); }\n' +
            'void main(){ vec2 uv=v_uv; float pl=texture2D(uPressure,uv-vec2(texelSize.x,0.0)).x; float pr=texture2D(uPressure,uv+vec2(texelSize.x,0.0)).x; float pb=texture2D(uPressure,uv-vec2(0.0,texelSize.y)).x; float pt=texture2D(uPressure,uv+vec2(0.0,texelSize.y)).x; vec2 g=vec2(pr-pl,pt-pb)*0.5; vec2 vel=decode(texture2D(uVelocity,uv).xy, velRange); vel-=g; gl_FragColor=vec4(encode(vel, velRange),0.0,1.0); }\n';

        var fs_vort = common +
            'uniform sampler2D uVelocity; uniform vec2 texelSize; uniform float curl; uniform float dt; uniform vec2 scale; uniform float velRange;\n' +
            'vec2 decode(vec2 v, float r){ return v*r*2.0-r; }\n' +
            'vec2 encode(vec2 v, float r){ return clamp(v/r*0.5+0.5, 0.0, 1.0); }\n' +
            'float getCurl(vec2 uv, vec2 dx){ vec2 vl=decode(texture2D(uVelocity,uv-vec2(dx.x,0.0)).xy, velRange); vec2 vr=decode(texture2D(uVelocity,uv+vec2(dx.x,0.0)).xy, velRange); vec2 vb=decode(texture2D(uVelocity,uv-vec2(0.0,dx.y)).xy, velRange); vec2 vt=decode(texture2D(uVelocity,uv+vec2(0.0,dx.y)).xy, velRange); return ((vr.y-vl.y)-(vt.x-vb.x))*0.5; }\n' +
            'void main(){ vec2 uv=v_uv; vec2 dx=texelSize*scale; float cL=getCurl(uv-vec2(dx.x,0.0),dx); float cR=getCurl(uv+vec2(dx.x,0.0),dx); float cB=getCurl(uv-vec2(0.0,dx.y),dx); float cT=getCurl(uv+vec2(0.0,dx.y),dx); float cC=getCurl(uv,dx); vec2 grad=vec2(abs(cR)-abs(cL), abs(cT)-abs(cB)); float len=length(grad)+1e-5; vec2 n=grad/len; float scaleAvg=(scale.x+scale.y)*0.5; vec2 f=vec2(n.y,-n.x)*cC*curl*dt*scaleAvg*0.1; vec2 vel=decode(texture2D(uVelocity,uv).xy, velRange); vel+=f; gl_FragColor=vec4(encode(vel, velRange),0.0,1.0); }\n';

        var fs_buoy = common +
            'uniform sampler2D uVelocity; uniform sampler2D uDensity; uniform float strength; uniform float maxHeat; uniform float heatPower; uniform float minHeat; uniform float velRange;\n' +
            'vec2 decode(vec2 v, float r){ return v*r*2.0-r; }\n' +
            'vec2 encode(vec2 v, float r){ return clamp(v/r*0.5+0.5, 0.0, 1.0); }\n' +
            'void main(){ vec2 uv=v_uv; float d=texture2D(uDensity,uv).r; float normalized=clamp(d/maxHeat, 0.0, 1.0); float absHP=abs(heatPower); float curve=pow(normalized, absHP); float heatNorm = heatPower >= 0.0 ? curve : 1.0 - curve; float heat=mix(minHeat, 1.0, heatNorm) * d; vec2 vel=decode(texture2D(uVelocity,uv).xy, velRange); vel.y += heat * strength; gl_FragColor=vec4(encode(vel, velRange),0.0,1.0); }\n';

        var fs_splat = common +
            'uniform sampler2D uTarget; uniform vec2 point; uniform float radius; uniform vec3 color;\n' +
            'void main(){ vec2 uv=v_uv; vec4 cur=texture2D(uTarget,uv); float d=distance(uv,point); float a=exp(-d*d/(radius*radius)); cur.rgb += color * a; gl_FragColor=cur; }\n';

        var fs_splat_vel = common +
            'uniform sampler2D uTarget; uniform vec2 point; uniform float radius; uniform vec2 force; uniform float velRange;\n' +
            'vec2 encode(vec2 v, float r){ return clamp(v/r*0.5+0.5, 0.0, 1.0); }\n' +
            'vec2 decode(vec2 v, float r){ return v*r*2.0-r; }\n' +
            'void main(){ vec2 uv=v_uv; vec4 cur=texture2D(uTarget,uv); float d=distance(uv,point); float a=exp(-d*d/(radius*radius)); vec2 vel=decode(cur.xy, velRange); vel += force * a; gl_FragColor=vec4(encode(vel, velRange),0.0,1.0); }\n';

        var fs_display = common +
            'uniform sampler2D uDensity; uniform vec2 resolution; uniform float glow; uniform vec2 vortexScale; uniform vec2 texelSize;\n' +
            'vec3 palette(float x){ vec3 c1=vec3(0.05,0.0,0.0); vec3 c2=vec3(0.8,0.15,0.02); vec3 c3=vec3(1.0,0.6,0.1); vec3 c4=vec3(1.0,0.95,0.8); vec3 col=mix(c1,c2,smoothstep(0.0,0.4,x)); col=mix(col,c3,smoothstep(0.3,0.8,x)); col=mix(col,c4,smoothstep(0.6,1.0,x)); return col; }\n' +
            'void main(){ vec2 uv=v_uv; float d=texture2D(uDensity,uv).r; vec3 col=palette(d); vec2 px=1.0/resolution * (1.0 + glow*1.5); float g=0.0; for(int i=-2;i<=2;i++){ for(int j=-2;j<=2;j++){ g += texture2D(uDensity, uv + vec2(float(i),float(j))*px).r; } } g *= 0.02*glow; col += vec3(1.0,0.6,0.2)*g; col = pow(col, vec3(0.85)); vec2 boxSize=vortexScale*texelSize; vec2 boxMin=vec2(0.02); vec2 boxMax=boxMin+boxSize; float lineW=1.5/resolution.x; float edge=step(boxMin.x-lineW,uv.x)*step(uv.x,boxMax.x+lineW)*step(boxMin.y-lineW,uv.y)*step(uv.y,boxMax.y+lineW); float inner=step(boxMin.x+lineW,uv.x)*step(uv.x,boxMax.x-lineW)*step(boxMin.y+lineW,uv.y)*step(uv.y,boxMax.y-lineW); float box=edge*(1.0-inner); col=mix(col,vec3(1.0),box*0.8); gl_FragColor=vec4(col,1.0); }\n';

        var vs_spark = 'attribute vec2 aPos; attribute float aAlpha; varying float vAlpha; void main(){ vAlpha=aAlpha; gl_Position=vec4(aPos*2.0-1.0, 0.0, 1.0); }';
        var fs_spark = 'precision highp float; varying float vAlpha; void main(){ vec3 col=vec3(1.0, 0.8, 0.4); gl_FragColor=vec4(col, vAlpha); }';

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
        var progAdvectVel = createProgram(fs_advect_vel);
        var progDiv = createProgram(fs_div);
        var progJac = createProgram(fs_jacobi);
        var progGrad = createProgram(fs_grad);
        var progVort = createProgram(fs_vort);
        var progBuoy = createProgram(fs_buoy);
        var progSplat = createProgram(fs_splat);
        var progSplatVel = createProgram(fs_splat_vel);
        var progDisp = createProgram(fs_display);

        var vsSparkObj = createShader(gl.VERTEX_SHADER, vs_spark);
        var fsSparkObj = createShader(gl.FRAGMENT_SHADER, fs_spark);
        var progSpark = gl.createProgram();
        gl.attachShader(progSpark, vsSparkObj);
        gl.attachShader(progSpark, fsSparkObj);
        gl.linkProgram(progSpark);
        var sparkLineBuf = gl.createBuffer();

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

        var simScale = 0.3;
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
        var curl = 80;
        var vortexScaleX = 5.0;
        var vortexScaleY = 10.0;
        var buoyancy = 15.0;
        var maxHeat = 3.5;
        var heatPower = 3.6;
        var minHeat = 0.15;
        var glow = 1.2;
        var fireSpeed = 1.2;
        var emberRate = 3;
        var velDissipation = 0.84;
        var denDissipation = 0.961;
        var sparkChance = 0.275;
        var sparkMax = 64;
        var rdx = 130.0;
        var velRange = 2.5;
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
        addSlider('quality (sim scale)', 0.1, 2.0, 0.05, simScale, function(v){ simScale = v; allocFBOs(); });
        addSlider('pressure iterations', 1, 48, 1, jacobi, function(v){ jacobi = Math.floor(v); });
        addSlider('vorticity', 0.0, 100.0, 1.0, curl, function(v){ curl = v; });
        addSlider('vortex scale H', 0.0, 50.0, 1.0, vortexScaleX, function(v){ vortexScaleX = v; });
        addSlider('vortex scale V', 0.0, 50.0, 1.0, vortexScaleY, function(v){ vortexScaleY = v; });
        addSlider('buoyancy (rise)', 10.0, 40.0, 0.5, buoyancy, function(v){ buoyancy = v; });
        addSlider('max heat', 0.5, 10.0, 0.5, maxHeat, function(v){ maxHeat = v; });
        addSlider('heat power', -5.0, 5.0, 0.1, heatPower, function(v){ heatPower = v; });
        addSlider('min heat', 0.0, 0.5, 0.01, minHeat, function(v){ minHeat = v; });
        addSlider('advection speed', 10.0, 500.0, 5.0, rdx, function(v){ rdx = v; });
        addSlider('fire speed', 0.2, 3.0, 0.05, fireSpeed, function(v){ fireSpeed = v; });
        addSlider('embers per frame', 1, 100, 1, emberRate, function(v){ emberRate = Math.floor(v); });
        addSlider('velocity dissipation', 0.0, 2.0, 0.01, velDissipation, function(v){ velDissipation = v; });
        addSlider('density dissipation', 0.8, 1.0, 0.001, denDissipation, function(v){ denDissipation = v; });
        addSlider('spark chance', 0.0, 0.3, 0.005, sparkChance, function(v){ sparkChance = v; });
        addSlider('glow', 0.0, 2.0, 0.01, glow, function(v){ glow = v; });
        addSlider('velocity range', 1.0, 5.0, 0.5, velRange, function(v){ velRange = v; });

        function addSplat(x, y, dx, dy, radius, densityAmount){
            splats.push({ x:x, y:y, dx:dx, dy:dy, r:radius, d:densityAmount });
        }

        function spawnEmbers(){
            for(var i=0;i<emberRate;i++){
                var x = 0.5 + (Math.random()-0.5)*0.6;
                var y = Math.random()*0.02;
                var dx = (Math.random()-0.5)*0.1;
                var dy = fireSpeed * (0.8 + Math.random()*0.4);
                addSplat(x, y, dx, dy, 0.008, 1.5);
            }
        }

        function spawnSpark(){
            if(sparks.length > sparkMax) return;
            var x = 0.5 + (Math.random()-0.5)*0.4;
            var y = 0.01 + Math.random()*0.02;
            var s = {
                x: x,
                y: y,
                px: x,
                py: y,
                vx: (Math.random()-0.5)*0.3,
                vy: 1.5 + Math.random()*1.0,
                maxY: 0.25 + Math.random()*0.55,
                fadeStart: 0.0
            };
            s.fadeStart = s.maxY * 0.7;
            sparks.push(s);
        }

        function updateSparks(dtSec){
            for(var i=sparks.length-1;i>=0;i--){
                var s = sparks[i];
                s.px = s.x;
                s.py = s.y;
                s.vx += (Math.random()-0.5) * 8.0 * dtSec;
                s.vx *= 0.95;
                s.x += s.vx * dtSec;
                s.y += s.vy * dtSec;
                s.vy -= 0.2 * dtSec;
                if(s.y >= s.maxY){ sparks.splice(i,1); continue; }
                s.alpha = 1.0;
                if(s.y > s.fadeStart){
                    s.alpha = 1.0 - (s.y - s.fadeStart) / (s.maxY - s.fadeStart);
                }
            }
        }

        function renderSparks(){
            if(sparks.length === 0) return;
            var verts = [];
            for(var i=0; i<sparks.length; i++){
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
                    gl.uniform1f(gl.getUniformLocation(p,'velRange'), velRange);
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
                addSplat(x, y, dx, dy + fireSpeed * 1.5, 0.006, 3.0);
                pointer.px = pointer.x; pointer.py = pointer.y;
            }

            applySplats();

            renderTo(velocity.write, progAdvectVel, function(p){
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uVelocity'),0);
                gl.uniform1f(gl.getUniformLocation(p,'dt'), dtSec);
                gl.uniform1f(gl.getUniformLocation(p,'dissipation'), velDissipation);
                gl.uniform2f(gl.getUniformLocation(p,'texelSize'), 1.0/simW, 1.0/simH);
                gl.uniform1f(gl.getUniformLocation(p,'rdx'), rdx);
                gl.uniform1f(gl.getUniformLocation(p,'velRange'), velRange);
            }); velocity.swap();

            renderTo(velocity.write, progVort, function(p){
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uVelocity'),0);
                gl.uniform2f(gl.getUniformLocation(p,'texelSize'), 1.0/simW, 1.0/simH);
                gl.uniform1f(gl.getUniformLocation(p,'curl'), curl);
                gl.uniform1f(gl.getUniformLocation(p,'dt'), dtSec);
                gl.uniform2f(gl.getUniformLocation(p,'scale'), vortexScaleX, vortexScaleY);
                gl.uniform1f(gl.getUniformLocation(p,'velRange'), velRange);
            }); velocity.swap();

            renderTo(velocity.write, progBuoy, function(p){
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uVelocity'),0);
                gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, density.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uDensity'),1);
                gl.uniform1f(gl.getUniformLocation(p,'strength'), buoyancy);
                gl.uniform1f(gl.getUniformLocation(p,'maxHeat'), maxHeat);
                gl.uniform1f(gl.getUniformLocation(p,'heatPower'), heatPower);
                gl.uniform1f(gl.getUniformLocation(p,'minHeat'), minHeat);
                gl.uniform1f(gl.getUniformLocation(p,'velRange'), velRange);
            }); velocity.swap();

            renderTo(divergenceFBO, progDiv, function(p){
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uVelocity'),0);
                gl.uniform2f(gl.getUniformLocation(p,'texelSize'), 1.0/simW, 1.0/simH);
                gl.uniform1f(gl.getUniformLocation(p,'velRange'), velRange);
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
                gl.uniform1f(gl.getUniformLocation(p,'velRange'), velRange);
            }); velocity.swap();

            renderTo(density.write, progAdvect, function(p){
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uVelocity'),0);
                gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, density.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uSource'),1);
                gl.uniform1f(gl.getUniformLocation(p,'dt'), dtSec);
                gl.uniform1f(gl.getUniformLocation(p,'dissipation'), denDissipation);
                gl.uniform2f(gl.getUniformLocation(p,'texelSize'), 1.0/simW, 1.0/simH);
                gl.uniform1f(gl.getUniformLocation(p,'rdx'), rdx);
                gl.uniform1f(gl.getUniformLocation(p,'velRange'), velRange);
            }); density.swap();

            renderTo(null, progDisp, function(p){
                gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, density.read.texture); gl.uniform1i(gl.getUniformLocation(p,'uDensity'),0);
                gl.uniform2f(gl.getUniformLocation(p,'resolution'), canvas.width, canvas.height);
                gl.uniform1f(gl.getUniformLocation(p,'glow'), glow);
                gl.uniform2f(gl.getUniformLocation(p,'vortexScale'), vortexScaleX, vortexScaleY);
                gl.uniform2f(gl.getUniformLocation(p,'texelSize'), 1.0/simW, 1.0/simH);
            });

            renderSparks();
        }

        function frame(t){ step(t); requestAnimationFrame(frame); }
        requestAnimationFrame(frame);
    }

    return { start: start };
})();

TestFire.start();
