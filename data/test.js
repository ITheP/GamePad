/* Clean replacement: single coherent TestFire implementation */
var TestFire = (function(){
    function $(id){ return document.getElementById(id); }

    function start(){
        var content = $('content') || document.body;
        var canvas = document.createElement('canvas');
        canvas.style.position='fixed'; canvas.style.top='0'; canvas.style.left='0'; canvas.style.width='100%'; canvas.style.height='100%'; canvas.style.zIndex='-1';
        content.appendChild(canvas);
        var gl = canvas.getContext('webgl'); if(!gl){ console.error('WebGL not available'); return; }

        // shaders (vertex + fragments)
        var vs = '\nattribute vec2 position;\nvarying vec2 v_texCoord;\nvoid main(){ v_texCoord = position*0.5+0.5; gl_Position = vec4(position,0.0,1.0); }\n';
        var common = 'precision highp float;\nvarying vec2 v_texCoord;\n';

        var fs_advect = common + '\n' +
            'uniform sampler2D uVelocity; uniform sampler2D uSource; uniform float dt; uniform float dissipation; uniform vec2 texelSize;\n' +
            'void main(){ vec2 uv=v_texCoord; vec2 vel=texture2D(uVelocity,uv).xy*2.0-1.0; vec2 prev=uv-vel*dt; gl_FragColor = texture2D(uSource, prev) * dissipation; }\n';

        var fs_div = common + '\n' +
            'uniform sampler2D uVelocity; uniform vec2 texelSize; void main(){ vec2 uv=v_texCoord; float vl=texture2D(uVelocity,uv-vec2(texelSize.x,0)).x; float vr=texture2D(uVelocity,uv+vec2(texelSize.x,0)).x; float vb=texture2D(uVelocity,uv-vec2(0,texelSize.y)).y; float vt=texture2D(uVelocity,uv+vec2(0,texelSize.y)).y; float d=(vr-vl+vt-vb)*0.5; gl_FragColor=vec4(d,0,0,1); }\n';

        var fs_jacobi = common + '\n' +
            'uniform sampler2D uPressure; uniform sampler2D uDivergence; uniform vec2 texelSize; void main(){ vec2 uv=v_texCoord; float pl=texture2D(uPressure,uv-vec2(texelSize.x,0)).x; float pr=texture2D(uPressure,uv+vec2(texelSize.x,0)).x; float pb=texture2D(uPressure,uv-vec2(0,texelSize.y)).x; float pt=texture2D(uPressure,uv+vec2(0,texelSize.y)).x; float b=texture2D(uDivergence,uv).x; float p=(pl+pr+pb+pt-b)*0.25; gl_FragColor=vec4(p,0,0,1); }\n';

        var fs_grad = common + '\n' +
            'uniform sampler2D uPressure; uniform sampler2D uVelocity; uniform vec2 texelSize; void main(){ vec2 uv=v_texCoord; float pl=texture2D(uPressure,uv-vec2(texelSize.x,0)).x; float pr=texture2D(uPressure,uv+vec2(texelSize.x,0)).x; float pb=texture2D(uPressure,uv-vec2(0,texelSize.y)).x; float pt=texture2D(uPressure,uv+vec2(0,texelSize.y)).x; vec2 g=vec2(pr-pl,pt-pb)*0.5; vec2 vel=texture2D(uVelocity,uv).xy*2.0-1.0; vel-=g; vec2 enc=vel*0.5+0.5; gl_FragColor=vec4(enc,0,1); }\n';

        var fs_vort = common + '\n' +
            'uniform sampler2D uVelocity; uniform vec2 texelSize; uniform float curl; void main(){ vec2 uv=v_texCoord; vec2 vl=texture2D(uVelocity,uv-vec2(texelSize.x,0)).xy*2.0-1.0; vec2 vr=texture2D(uVelocity,uv+vec2(texelSize.x,0)).xy*2.0-1.0; vec2 vb=texture2D(uVelocity,uv-vec2(0,texelSize.y)).xy*2.0-1.0; vec2 vt=texture2D(uVelocity,uv+vec2(0,texelSize.y)).xy*2.0-1.0; float c=(vr.y-vl.y)-(vt.x-vb.x); vec2 grad=vec2(abs(vr.y-vl.y),abs(vt.x-vb.x)); vec2 N=normalize(grad+1e-6); vec2 f=vec2(N.y,-N.x)*c*curl; vec2 vel=texture2D(uVelocity,uv).xy*2.0-1.0; vel+=f; vec2 enc=vel*0.5+0.5; gl_FragColor=vec4(enc,0,1); }\n';

        // density splat (particles generate density)
        // Sparks spawn only near the bottom of the mesh (y in [0, 0.08])
        var fs_splat = common + '\n' +
            'uniform sampler2D uTarget; uniform float time; uniform int sparkCount; uniform vec2 resolution; float rand(vec2 co){ return fract(sin(dot(co,vec2(12.9898,78.233)))*43758.5453); } void main(){ vec2 uv=v_texCoord; vec4 cur=texture2D(uTarget,uv); for(int i=0;i<256;i++){ if(i>=sparkCount) break; float s=rand(vec2(float(i),time)); float x=0.5+(s-0.5)*0.3; float y=fract(time*0.3 + s*1.37)*0.25; vec2 pos=vec2(x, y); float life=1.0-pos.y; float r=mix(0.02,0.005,life); float spl=smoothstep(r,0.0,length(uv-pos)); cur.r += spl * life * 0.9; } gl_FragColor=cur; }\n';

        // velocity splat: note Y is flipped on encode so we keep advect decode canonical
        var fs_splat_vel = common + '\n' +
            'uniform sampler2D uTarget; uniform float time; uniform int sparkCount; uniform vec2 resolution; uniform float fireSpeed; float rand(vec2 co){ return fract(sin(dot(co,vec2(12.9898,78.233)))*43758.5453); } void main(){ vec2 uv=v_texCoord; vec4 cur=texture2D(uTarget,uv); for(int i=0;i<256;i++){ if(i>=sparkCount) break; float s=rand(vec2(float(i),time)); float x=0.5+(s-0.5)*0.3; float y=fract(time*0.3 + s*1.37)*0.25; vec2 pos=vec2(x, y); float life=1.0-pos.y; float r=mix(0.02,0.005,life); float spl=smoothstep(r,0.0,length(uv-pos)); vec2 vel=vec2((s-0.5)*0.05, -mix(fireSpeed,fireSpeed*2.0,life)); vec2 enc=vec2(vel.x*0.5+0.5, vel.y*0.5+0.5); cur.xy += enc * spl * 0.9; } gl_FragColor=cur; }\n';

        var fs_display = common + '\n' +
            'uniform sampler2D uDensity; uniform float glowLevel; uniform vec2 resolution; void main(){ vec2 uv=v_texCoord; float d=texture2D(uDensity,uv).r; vec3 col=vec3(1.0,0.45,0.12)*smoothstep(0.0,1.0,d); float g=0.0; vec2 px=1.0/resolution*(1.0+glowLevel*2.0); for(int i=-2;i<=2;i++){ for(int j=-2;j<=2;j++){ g += texture2D(uDensity, uv + vec2(float(i),float(j))*px).r; } } g *= 0.02 * glowLevel; col += vec3(1.0,0.6,0.2) * g; gl_FragColor=vec4(pow(col,vec3(0.9)),1.0); }\n';

        function createShader(type, src){ var s=gl.createShader(type); gl.shaderSource(s,src); gl.compileShader(s); if(!gl.getShaderParameter(s,gl.COMPILE_STATUS)){ console.error(gl.getShaderInfoLog(s)); gl.deleteShader(s); return null;} return s; }
        var vsObj = createShader(gl.VERTEX_SHADER, vs);
        function createProgram(fsSrc){ var fsObj=createShader(gl.FRAGMENT_SHADER, fsSrc); var p=gl.createProgram(); gl.attachShader(p, vsObj); gl.attachShader(p, fsObj); gl.linkProgram(p); if(!gl.getProgramParameter(p,gl.LINK_STATUS)){ console.error(gl.getProgramInfoLog(p)); return null;} return p; }

        var progAdvect = createProgram(fs_advect);
        var progDiv = createProgram(fs_div);
        var progJac = createProgram(fs_jacobi);
        var progGrad = createProgram(fs_grad);
        var progVort = createProgram(fs_vort);
        var progSplat = createProgram(fs_splat);
        var progSplatVel = createProgram(fs_splat_vel);
        var progDisp = createProgram(fs_display);

        function createFBO(w,h){ var tex=gl.createTexture(); gl.bindTexture(gl.TEXTURE_2D, tex); gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,w,h,0,gl.RGBA,gl.UNSIGNED_BYTE,null); gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.LINEAR); gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.LINEAR); gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_S,gl.CLAMP_TO_EDGE); gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_T,gl.CLAMP_TO_EDGE); var fb=gl.createFramebuffer(); gl.bindFramebuffer(gl.FRAMEBUFFER, fb); gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, tex, 0); return {texture:tex, framebuffer:fb, width:w, height:h}; }
        function createDouble(w,h){ return { read: createFBO(w,h), write: createFBO(w,h), swap: function(){var t = this.read; this.read = this.write; this.write = t;} }; }

        var simScale = 1.0, simW=0, simH=0; var velocity, density, pressure, divergenceFBO;
        function allocFBOs(){ simW=Math.max(32,Math.round(canvas.width*simScale)); simH=Math.max(32,Math.round(canvas.height*simScale)); function delF(f){ try{ if(f){ gl.deleteTexture(f.texture); gl.deleteFramebuffer(f.framebuffer);} }catch(e){} } if(velocity){ delF(velocity.read); delF(velocity.write);} if(density){ delF(density.read); delF(density.write);} if(pressure){ delF(pressure.read); delF(pressure.write);} if(divergenceFBO){ delF(divergenceFBO);} velocity=createDouble(simW,simH); density=createDouble(simW,simH); pressure=createDouble(simW,simH); divergenceFBO=createFBO(simW,simH); function clearF(fb,r,g,b,a){ gl.bindFramebuffer(gl.FRAMEBUFFER, fb.framebuffer); gl.clearColor(r,g,b,a); gl.clear(gl.COLOR_BUFFER_BIT);} clearF(velocity.read,0.5,0.5,0,1); clearF(velocity.write,0.5,0.5,0,1); clearF(density.read,0,0,0,0); clearF(density.write,0,0,0,0); clearF(pressure.read,0,0,0,0); clearF(pressure.write,0,0,0,0); clearF(divergenceFBO,0,0,0,0); }

        var quadBuf = gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER, quadBuf); gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1,-1,1,-1,-1,1,1,1]), gl.STATIC_DRAW);
        function bindQuad(prog){ var loc=gl.getAttribLocation(prog,'position'); gl.bindBuffer(gl.ARRAY_BUFFER, quadBuf); if(loc>=0){ gl.enableVertexAttribArray(loc); gl.vertexAttribPointer(loc,2,gl.FLOAT,false,0,0);} }
        function setVP(fb){ if(fb){ gl.bindFramebuffer(gl.FRAMEBUFFER, fb.framebuffer); gl.viewport(0,0,fb.width,fb.height);} else { gl.bindFramebuffer(gl.FRAMEBUFFER,null); gl.viewport(0,0,canvas.width,canvas.height);} }
        function renderTo(target, prog, setUniforms){ if(!prog) return; gl.useProgram(prog); bindQuad(prog); if(setUniforms) setUniforms(prog); setVP(target); gl.drawArrays(gl.TRIANGLE_STRIP,0,4); }

        function resize(){ canvas.width=window.innerWidth; canvas.height=window.innerHeight; gl.viewport(0,0,canvas.width,canvas.height); allocFBOs(); }
    window.addEventListener('resize', resize);
    simScale = 2.0; // Start with higher mesh resolution
    resize();

    var dt=1/60, jacobi=24, curl=10, sparks=256, glow=1.0;
    var fireSpeed = 0.18; // Controls upward velocity of fire
        // UI
        var ctrl=document.createElement('div'); ctrl.style.position='fixed'; ctrl.style.right='8px'; ctrl.style.top='8px'; ctrl.style.zIndex=9999; ctrl.style.background='rgba(0,0,0,0.4)'; ctrl.style.color='#fff'; ctrl.style.padding='8px'; ctrl.style.fontFamily='sans-serif'; ctrl.style.fontSize='12px'; ctrl.style.borderRadius='6px';
        function add(label,min,max,step,val,cb){ var row=document.createElement('div'); var l=document.createElement('label'); l.textContent=label; l.style.display='block'; l.style.marginBottom='4px'; var i=document.createElement('input'); i.type='range'; i.min=min; i.max=max; i.step=step; i.value=val; i.style.width='160px'; var v=document.createElement('span'); v.textContent=val; v.style.marginLeft='8px'; i.oninput=function(){ v.textContent=i.value; cb(parseFloat(i.value)); }; row.appendChild(l); row.appendChild(i); row.appendChild(v); ctrl.appendChild(row); return i; }
        content.appendChild(ctrl);
    add('quality (res scale)',0.25,2.0,0.25,2.0,function(v){ simScale=v; allocFBOs(); });
    // Add mesh resolution slider (linked to simScale)
    add('mesh resolution',0.25,2.0,0.01,simScale,function(v){ simScale=v; allocFBOs(); });
    // Add vorticity strength slider (linked to curl)
    add('vorticity strength',0.0,20.0,0.1,curl,function(v){ curl=v; });
    // Add fire speed slider (linked to fireSpeed)
    add('fire speed',0.05,0.5,0.01,fireSpeed,function(v){ fireSpeed=v; });
        add('sparks',1,512,1,sparks,function(v){ sparks=Math.floor(v); });
        add('glow',0,2,0.01,glow,function(v){ glow=v; });

        function step(t){ var time=t*0.001;
            // splat density
            renderTo(density.write, progSplat, function(prog){
                gl.activeTexture(gl.TEXTURE0);
                gl.bindTexture(gl.TEXTURE_2D, density.read.texture);
                gl.uniform1i(gl.getUniformLocation(prog,'uTarget'),0);
                gl.uniform1f(gl.getUniformLocation(prog,'time'),time);
                gl.uniform1i(gl.getUniformLocation(prog,'sparkCount'),sparks);
                gl.uniform2f(gl.getUniformLocation(prog,'resolution'), simW, simH);
            }); density.swap();
            // splat velocity (encodes Y flipped)
            renderTo(velocity.write, progSplatVel, function(prog){
                gl.activeTexture(gl.TEXTURE0);
                gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture);
                gl.uniform1i(gl.getUniformLocation(prog,'uTarget'),0);
                gl.uniform1f(gl.getUniformLocation(prog,'time'),time);
                gl.uniform1i(gl.getUniformLocation(prog,'sparkCount'),sparks);
                gl.uniform2f(gl.getUniformLocation(prog,'resolution'), simW, simH);
                gl.uniform1f(gl.getUniformLocation(prog,'fireSpeed'), fireSpeed);
            }); velocity.swap();
            // advect vel
            renderTo(velocity.write, progAdvect, function(prog){ gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(prog,'uVelocity'),0); gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(prog,'uSource'),1); gl.uniform1f(gl.getUniformLocation(prog,'dt'),dt); gl.uniform1f(gl.getUniformLocation(prog,'dissipation'),0.99); gl.uniform2f(gl.getUniformLocation(prog,'texelSize'),1.0/simW,1.0/simH); }); velocity.swap();
            // vorticity
            renderTo(velocity.write, progVort, function(prog){ gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(prog,'uVelocity'),0); gl.uniform2f(gl.getUniformLocation(prog,'texelSize'),1.0/simW,1.0/simH); gl.uniform1f(gl.getUniformLocation(prog,'curl'), curl); }); velocity.swap();
            // divergence
            renderTo(divergenceFBO, progDiv, function(prog){ gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(prog,'uVelocity'),0); gl.uniform2f(gl.getUniformLocation(prog,'texelSize'),1.0/simW,1.0/simH); });
            // pressure solve
            gl.bindFramebuffer(gl.FRAMEBUFFER, pressure.read.framebuffer); gl.clearColor(0,0,0,0); gl.clear(gl.COLOR_BUFFER_BIT);
            for(var i=0;i<jacobi;i++){ renderTo(pressure.write, progJac, function(prog){ gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, pressure.read.texture); gl.uniform1i(gl.getUniformLocation(prog,'uPressure'),0); gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, divergenceFBO.texture); gl.uniform1i(gl.getUniformLocation(prog,'uDivergence'),1); gl.uniform2f(gl.getUniformLocation(prog,'texelSize'),1.0/simW,1.0/simH); }); pressure.swap(); }
            // project
            renderTo(velocity.write, progGrad, function(prog){ gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, pressure.read.texture); gl.uniform1i(gl.getUniformLocation(prog,'uPressure'),0); gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(prog,'uVelocity'),1); gl.uniform2f(gl.getUniformLocation(prog,'texelSize'),1.0/simW,1.0/simH); }); velocity.swap();
            // advect density
            renderTo(density.write, progAdvect, function(prog){ gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, velocity.read.texture); gl.uniform1i(gl.getUniformLocation(prog,'uVelocity'),0); gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, density.read.texture); gl.uniform1i(gl.getUniformLocation(prog,'uSource'),1); gl.uniform1f(gl.getUniformLocation(prog,'dt'),dt); gl.uniform1f(gl.getUniformLocation(prog,'dissipation'),0.995); gl.uniform2f(gl.getUniformLocation(prog,'texelSize'),1.0/simW,1.0/simH); }); density.swap();
            // display
            renderTo(null, progDisp, function(prog){ gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, density.read.texture); gl.uniform1i(gl.getUniformLocation(prog,'uDensity'),0); gl.uniform1f(gl.getUniformLocation(prog,'glowLevel'), glow); gl.uniform2f(gl.getUniformLocation(prog,'resolution'), canvas.width, canvas.height); });
        }

        function frame(t){ step(t); requestAnimationFrame(frame); }
        requestAnimationFrame(frame);
    }

    return { start:start };
})();

TestFire.start();
