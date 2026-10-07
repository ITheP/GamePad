
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
                float contrast = 0.8; // 0.3;
                
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

/* =========================================================================
   Configuration page
   Reads /json/config, renders grouped controls, and posts changes back
   to the device via /api/UpdateConfig
   ========================================================================= */
(function () {
    "use strict";

    var CONFIG_URL = "/json/config";
    var UPDATE_URL = "/api/UpdateConfig";
    var SAVE_TIMEOUT_MS = 10000;

    var ICON = {
        FAILED:  "❌",                      // "\u274C"
        SAVE:    "💾",                      // "\uD83D\uDCBE"
        SAVING:  "⏳",                      // "\u23F3",
        TICK:    "✅",                      // "\u2705"
        DEFAULT: "Default",
        RETRY:   "🔄",                      // "\uD83D\uDD04"
        WARNING: "⚠️",                      // "\u26A0\uFE0F"
        INFO:    "ℹ️"                       // "\u2139\uFE0F"
    };

    /* ---------------------------------------------------------------------
       Small helpers
       --------------------------------------------------------------------- */

    function meta(item) {
        return (item && item.Metadata) ? item.Metadata : {};
    }

    function field(item, name, fallback) {
        if (item && item[name] !== undefined && item[name] !== null) return item[name];
        var m = meta(item);
        if (m[name] !== undefined && m[name] !== null) return m[name];
        return fallback;
    }

    function typeOf(item) {
        return String(field(item, "Type", "String")).toLowerCase();
    }
    function isBool(item)   { var t = typeOf(item); return t === "bool" || t === "boolean"; }
    function isInt(item)    { var t = typeOf(item); return t === "int" || t === "integer"; }
    function isFloat(item)  { var t = typeOf(item); return t === "float" || t === "double"; }
    function isString(item) { return !isBool(item) && !isInt(item) && !isFloat(item); }

    function num(v, fallback) {
        var n = (typeof v === "number") ? v : parseFloat(v);
        return isFinite(n) ? n : fallback;
    }

    function clamp(v, a, b) {
        var lo = Math.min(a, b);
        var hi = Math.max(a, b);
        return Math.max(lo, Math.min(hi, v));
    }

    function toBool(v) {
        if (v === true || v === 1) return true;
        if (typeof v === "string") {
            var s = v.trim().toLowerCase();
            return s === "1" || s === "true" || s === "on" || s === "yes";
        }
        return false;
    }

    function decimalsForStep(step) {
        if (!(step > 0)) return 2;
        var s = String(step);
        if (s.indexOf("e") !== -1 || s.indexOf("E") !== -1) {
            return Math.min(6, Math.max(0, Math.ceil(-Math.log10(step))));
        }
        var i = s.indexOf(".");
        return i === -1 ? 0 : (s.length - i - 1);
    }

    function formatNumber(v, isIntType, step) {
        if (!isFinite(v)) return "";
        if (isIntType) return String(Math.round(v));
        var decimals = decimalsForStep(step);
        var out = v.toFixed(Math.min(6, decimals));
        if (out.indexOf(".") !== -1) {
            out = out.replace(/0+$/, "").replace(/\.$/, "");
        }
        return out;
    }

    function roundStep(raw) {
        if (!(raw > 0)) return 1;
        var mag  = Math.pow(10, Math.floor(Math.log10(raw)));
        var norm = raw / mag;
        var nice = norm <= 1 ? 1 : norm <= 2 ? 2 : norm <= 5 ? 5 : 10;
        return nice * mag;
    }

    function jsonSafe(value) {
        if (value === undefined || value === null) return null;
        if (typeof value === "number") return isFinite(value) ? value : null;
        if (typeof value === "boolean" || typeof value === "string") return value;
        try { return JSON.parse(JSON.stringify(value)); }
        catch (e) { return String(value); }
    }

    /* ---------------------------------------------------------------------
       UI state
       --------------------------------------------------------------------- */

    var ui = {
        tableBody:    null,
        refreshBtn:   null,
        globalStatus: null
    };

    var rows = Object.create(null);

    /* ---------------------------------------------------------------------
       Networking
       --------------------------------------------------------------------- */

    function loadConfig() {
        setGlobalStatus(ICON.SAVING + " Loading configuration\u2026");
        if (ui.refreshBtn) ui.refreshBtn.disabled = true;

        while (ui.tableBody.firstChild) ui.tableBody.removeChild(ui.tableBody.firstChild);

        // var TEST_CONFIG_JSON = `{
        // "config": [
        //     {"Id": 3,"Type": "Bool","Metadata": {"Group": "Inputs","Label": "Live Serial Output","Description": "Enables detailed serial output of input states, including digital, analog, virtual and battery states.","Info": "","RenderAs": "Toggle","SaveInPrefs": 0,"min": 0,"max": 0,"uiMin": 0,"uiMax": 0,"uiStep": 0,"Value": 0,"DefaultValue": 0}},
        //     {"Id": 1,"Type": "Bool","Metadata": {"Group": "Screen","Label": "White Screen","Description": "Screen will show a solid white, handy when physically aligning panel in device where visible edges are visible","Info": "","RenderAs": "Default","SaveInPrefs": 0,"min": 0,"max": 0,"uiMin": 0,"uiMax": 0,"uiStep": 0,"Value": 0,"DefaultValue": 0}},
        //     {"Id": 2,"Type": "Bool","Metadata": {"Group": "Screen","Label": "Force FPS Display","Description": "Forces the display of FPS in the top right corner of the screen","Info": "","RenderAs": "Toggle","SaveInPrefs": 1,"min": 0,"max": 0,"uiMin": 0,"uiMax": 0,"uiStep": 0,"Value": 0,"DefaultValue": 0}},
        //     {"Id": 4,"Type": "Float","Metadata": {"Group": "Idle","Label": "LED Timeout","Description": "Seconds before LED's go into idle mode.","Info": "","RenderAs": "Default","SaveInPrefs": 0,"min": 0,"max": 86400,"uiMin": 0,"uiMax": 600,"uiStep": 10,"Value": 10,"DefaultValue": 0}},
        //     {"Id": 5,"Type": "Float","Metadata": {"Group": "Idle","Label": "Screen Timeout","Description": "Seconds before screen go into idle mode.","Info": "","RenderAs": "Default","SaveInPrefs": 0,"min": 0,"max": 86400,"uiMin": 0,"uiMax": 600,"uiStep": 10,"Value": 30,"DefaultValue": 0}},
        //     {"Id": 6,"Type": "Float","Metadata": {"Group": "Idle","Label": "Screen Restart","Description": "Seconds before screen idle effect restarts.","Info": "","RenderAs": "Default","SaveInPrefs": 0,"min": 0,"max": 86400,"uiMin": 0,"uiMax": 600,"uiStep": 10,"Value": 60,"DefaultValue": 0}},
        //     {"Id": 7,"Type": "Int","Metadata": {"Group": "LED","Label": "Brightness","Description": "Global maximum brightness of LED's","Info": "Very low brightness levels may result in funny looking LED colours or fades as there isn't the resolution of brightness levels to represent subtle differences in colour","RenderAs": "Default","SaveInPrefs": 0,"min": 0,"max": 255,"uiMin": 0,"uiMax": 600,"uiStep": 10,"Value": 200,"DefaultValue": 0}}
        // ]
        // }`;

        // ---- Loader (test mode) ----
        // return Promise.resolve(JSON.parse(TEST_CONFIG_JSON))
        //     .then(function (data) {
        //         var list = (data && (data.config || data.Config)) || data;
        //         if (!Array.isArray(list)) list = [];
        //         renderConfig(list);
        //         setGlobalStatus("");
        //     });

        // ---- Loader (live mode) ----
        return fetch(CONFIG_URL, { cache: "no-store" })
            .then(function (res) {
                if (!res.ok) throw new Error("HTTP " + res.status + ": " + res.statusText);
                return res.json();
            })
            .then(function (data) {
                var list = (data && (data.config || data.Config)) || data;
                if (!Array.isArray(list)) list = [];
                renderConfig(list);
                setGlobalStatus("");
            })
            .catch(function (err) {
                console.error("Configuration: failed to load config \u2013", err);
                showMessage("Unable to load configuration from the device.", "error");
                setGlobalStatus(ICON.FAILED + " Failed to load configuration. Web service on device may not be responding.");
            })
            .finally(function () {
                if (ui.refreshBtn) ui.refreshBtn.disabled = false;
            });
    }

    function postUpdateConfig(id, value) {
        // // ---- TEST MODE: pretend the device accepted it ----
        // console.log("TEST save:", id, "=", value);
        // return Promise.resolve({ ok: true });
        // // ---- /TEST MODE ----

        if (typeof window.POST_UpdateConfig === "function") {
            return new Promise(function (resolve, reject) {
                try { resolve(window.POST_UpdateConfig(id, value)); }
                catch (err) { reject(err); }
            });
        }
        
        return fetch(UPDATE_URL, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ Id: id, Value: value })
        }).then(function (res) {
            if (!res.ok) throw new Error("HTTP " + res.status + ": " + res.statusText);
            var ct = (res.headers.get("content-type") || "").toLowerCase();
            if (ct.indexOf("json") !== -1) return res.json();
            return res.text();
        });
    }

    function interpretResponse(result) {
        if (result === undefined || result === null) return true;
        if (typeof result === "boolean") return result;
        if (typeof result === "number")  return result !== 0;

        if (typeof result === "string") {
            var s = result.trim().toLowerCase();
            if (s === "") return true;
            if (s === "ok" || s === "true" || s === "success" || s === "saved") return true;
            if (s === "false" || s === "error" || s === "fail" || s === "failed") return false;
            try { return interpretResponse(JSON.parse(result)); }
            catch (e) { return true; }
        }

        if (typeof result === "object") {
            var flagKeys = ["ok", "Ok", "OK", "success", "Success", "succeeded", "Saved", "saved"];
            for (var i = 0; i < flagKeys.length; i++) {
                if (flagKeys[i] in result) return !!result[flagKeys[i]];
            }
            if ("Status" in result || "status" in result) {
                var st = String(result.Status || result.status).toLowerCase();
                return st === "ok" || st === "success" || st === "saved" || st === "true";
            }
            if ("Error" in result || "error" in result) {
                var er = result.Error || result.error;
                if (er) return false;
            }
            return true;
        }

        return true;
    }

    /* ---------------------------------------------------------------------
       Status rendering
       --------------------------------------------------------------------- */

    function iconSpan(text, className, title) {
        var el = document.createElement("span");
        el.className = className || "";
        el.textContent = text;
        if (title) el.title = title;
        return el;
    }

    function renderStatus(row) {
        var el = row.statusEl;
        if (!el) return;

        el.innerHTML = "";
        el.className = "config-status";

        switch (row.statusState) {
            case "saving":
                el.appendChild(iconSpan(ICON.SAVING, "status-saving", "Saving\u2026"));
                break;

            case "saved": {
                var tick = iconSpan(ICON.TICK, "status-tick", "Saved");
                el.appendChild(tick);

                tick.addEventListener("animationend", function () {
                    if (row.statusState === "saved") {
                        row.statusState = "idle";
                        renderStatus(row);
                    }
                });
                break;
            }

            case "error": {
                el.appendChild(iconSpan(
                    ICON.FAILED,
                    "status-error",
                    row.statusMessage || "Save failed"
                ));
                var retry = document.createElement("button");
                retry.type = "button";
                retry.className = "btn btn--retry";
                retry.textContent = ICON.RETRY;
                retry.title = "Retry saving";
                retry.addEventListener("click", function () { saveRow(row); });
                el.appendChild(retry);
                break;
            }

            default: { // idle / dirty
                var hasWarning = !!(row.control && row.control.hasWarning && row.control.hasWarning());

                if (hasWarning) {
                    el.appendChild(iconSpan(ICON.WARNING, "warning-icon", null));
                    var text = document.createElement("span");
                    text.className = "warning-text";
                    text.textContent = (row.control.warningMessage && row.control.warningMessage())
                        || "Out of range";
                    el.appendChild(text);
                } else if (row.showSaveButton) {
                    var btn = document.createElement("button");
                    btn.type = "button";
                    btn.className = "btn btn--save" + (row.dirty ? " dirty" : "");
                    btn.textContent = ICON.SAVE;
                    btn.title = row.dirty ? "Save changes" : "No changes to save";
                    btn.disabled = !row.dirty;
                    btn.addEventListener("click", function () { saveRow(row); });
                    el.appendChild(btn);
                }
                break;
            }
        }
    }

    function setStatus(row, state, opts) {
        opts = opts || {};
        row.statusState   = state;
        row.statusMessage = opts.message || "";
        renderStatus(row);
    }

    function markDirty(row) {
        row.dirty = true;
        if (row.statusState === "saved") row.statusState = "idle";
        renderStatus(row);
    }

    /* ---------------------------------------------------------------------
       Saving
       --------------------------------------------------------------------- */

    function saveRow(row) {
        if (!row || !row.control) return;

        var validity = row.validate ? row.validate() : { ok: true };
        if (!validity.ok) {
            setStatus(row, "error", { message: validity.message || "Invalid value" });
            return;
        }

        var value = jsonSafe(row.getValue());
        var token = ++row.saveToken;

        setStatus(row, "saving");

        var timeoutId = null;
        var timeoutPromise = new Promise(function (_, reject) {
            timeoutId = setTimeout(function () {
                reject(new Error("Timed out after " + SAVE_TIMEOUT_MS + "ms"));
            }, SAVE_TIMEOUT_MS);
        });

        Promise.race([postUpdateConfig(row.id, value), timeoutPromise])
            .then(function (result) {
                if (timeoutId) clearTimeout(timeoutId);
                if (token !== row.saveToken) return;

                if (!interpretResponse(result)) {
                    throw new Error("Controller rejected the value");
                }

                row.dirty = false;
                setStatus(row, "saved");
            })
            .catch(function (err) {
                if (timeoutId) clearTimeout(timeoutId);
                if (token !== row.saveToken) return;

                console.error("Configuration: save failed for id " + row.id + " \u2013", err);
                setStatus(row, "error", {
                    message: (err && err.message) ? err.message : "Save failed"
                });
            });
    }

    /* ---------------------------------------------------------------------
       Control builders
       Each returns: { element, extra?, getValue, setValue, validate, hasWarning?, warningMessage? }
       --------------------------------------------------------------------- */

    function buildControl(item, row) {
        if (isBool(item))                   return buildBoolControl(item, row);
        if (isInt(item) || isFloat(item))   return buildNumericControl(item, row);
        return buildStringControl(item, row);
    }

    /* ----- Bool ---------------------------------------------------------- */

    function buildBoolControl(item, row) {
        var renderAs  = String(field(item, "RenderAs", "")).toLowerCase();
        var useToggle = (renderAs === "toggle");

        var input = document.createElement("input");
        var element;

        if (useToggle) {
            input.type = "checkbox";
            element = document.createElement("label");
            element.className = "toggle-switch";
            var slider = document.createElement("span");
            slider.className = "toggle-slider";
            element.appendChild(input);
            element.appendChild(slider);
        } else {
            input.type = "checkbox";
            input.className = "config-checkbox";
            element = input;
        }

        input.checked = toBool(field(item, "Value", 0));

        // Booleans save straight away \u2013 no save button required
        row.showSaveButton = false;

        input.addEventListener("change", function () {
            markDirty(row);
            saveRow(row);
        });

        return {
            element: element,
            getValue: function () { return input.checked ? 1 : 0; },
            setValue: function (v) { input.checked = toBool(v); },
            validate: function () { return { ok: true }; }
        };
    }

    /* ----- Int / Float --------------------------------------------------- */

    function buildNumericControl(item, row) {
        var isIntType = isInt(item);

        // Read each independently so we can tell what's actually present
        var rawMin   = field(item, "min",   undefined);
        var rawMax   = field(item, "max",   undefined);
        var rawUiMin = field(item, "uiMin", undefined);
        var rawUiMax = field(item, "uiMax", undefined);
        var uiStep   = num(field(item, "uiStep", 0), 0);

        // uiMin/uiMax and min/max each fall back to the other pair, then to a
        // final default only if the device provides nothing at all. This keeps
        // the hint text, the warning range, and the slider in lockstep.
        var uiMin = num(rawUiMin, num(rawMin, 0));
        var uiMax = num(rawUiMax, num(rawMax, 100));
        var min   = num(rawMin,   uiMin);
        var max   = num(rawMax,   uiMax);

        if (max   < min)   { var t1 = min;   min   = max;   max   = t1; }
        if (uiMax < uiMin) { var t2 = uiMin; uiMin = uiMax; uiMax = t2; }
        if (!(uiStep > 0)) uiStep = isIntType ? 1 : roundStep((uiMax - uiMin) / 100);

        var initialValue = num(field(item, "Value", min), min);

        var slider = document.createElement("input");
        slider.type  = "range";
        slider.min   = String(uiMin);
        slider.max   = String(uiMax);
        slider.step  = String(uiStep);
        slider.value = String(clamp(initialValue, uiMin, uiMax));

        var number = document.createElement("input");
        number.type  = "number";
        number.min   = String(min);
        number.max   = String(max);
        number.step  = String(uiStep);
        number.value = formatNumber(initialValue, isIntType, uiStep);

        var container = document.createElement("div");
        container.className = "slider-container";
        container.appendChild(slider);
        container.appendChild(number);

        var hints = document.createElement("div");
        hints.className = "range-hints";
        var hintMin = document.createElement("span");
        hintMin.textContent = "Min: " + formatNumber(min, isIntType, uiStep);
        var hintMax = document.createElement("span");
        hintMax.textContent = "Max: " + formatNumber(max, isIntType, uiStep);
        hints.appendChild(hintMin);
        hints.appendChild(hintMax);

        var warningActive  = false;
        var warningMessage = "";

        function checkRange(v) {
            warningActive = !isFinite(v) || v < min || v > max;
            warningMessage = "Value must be between " +
                formatNumber(min, isIntType, uiStep) + " and " +
                formatNumber(max, isIntType, uiStep);
            return warningActive;
        }

        checkRange(initialValue);

        slider.addEventListener("input", function () {
            var v = parseFloat(slider.value);
            number.value = formatNumber(v, isIntType, uiStep);
            checkRange(v);
            markDirty(row);
        });

        number.addEventListener("input", function () {
            var v = parseFloat(number.value);
            if (isFinite(v)) slider.value = String(clamp(v, uiMin, uiMax));
            checkRange(v);
            markDirty(row);
        });

        return {
            element: container,
            extra:   hints,

            getValue: function () { return num(number.value, initialValue); },

            setValue: function (v) {
                var n = num(v, 0);
                number.value = formatNumber(n, isIntType, uiStep);
                slider.value = String(clamp(n, uiMin, uiMax));
                checkRange(n);
            },

            validate: function () {
                var v = parseFloat(number.value);
                if (!isFinite(v)) {
                    return { ok: false, message: "Please enter a valid number." };
                }
                if (v < min || v > max) {
                    return {
                        ok: false,
                        message: "Value must be between " +
                            formatNumber(min, isIntType, uiStep) + " and " +
                            formatNumber(max, isIntType, uiStep) + "."
                    };
                }
                return { ok: true };
            },

            hasWarning: function () { return warningActive; },
            warningMessage: function () { return warningMessage; }
        };
    }

    /* ----- String -------------------------------------------------------- */

    function buildStringControl(item, row) {
        var minLen = num(field(item, "min", 0), 0);
        var maxLen = num(field(item, "max", 0), 0);
        var raw    = field(item, "Value", "");
        var initialValue = (raw === null || raw === undefined) ? "" : String(raw);

        var input = document.createElement("input");
        input.type  = "text";
        input.value = initialValue;

        var hints = document.createElement("div");
        hints.className = "length-hints";
        var hintMin = document.createElement("span");
        hintMin.textContent = "Min length: " + minLen;
        var hintMax = document.createElement("span");
        hintMax.textContent = "Max length: " + (maxLen > 0 ? maxLen : "\u2014");
        hints.appendChild(hintMin);
        hints.appendChild(hintMax);

        var warningActive  = false;
        var warningMessage = "";

        function checkLength(v) {
            var len = v.length;
            warningActive = (len < minLen) || (maxLen > 0 && len > maxLen);
            warningMessage = "Length must be between " + minLen + " and " +
                (maxLen > 0 ? maxLen : "unlimited") + " characters.";
            return warningActive;
        }

        checkLength(initialValue);

        input.addEventListener("input", function () {
            checkLength(input.value);
            markDirty(row);
        });

        return {
            element: input,
            extra:   hints,

            getValue: function () { return input.value; },

            setValue: function (v) {
                input.value = (v === null || v === undefined) ? "" : String(v);
                checkLength(input.value);
            },

            validate: function () {
                var len = input.value.length;
                if (len < minLen) {
                    return { ok: false, message: "Minimum length is " + minLen + " characters." };
                }
                if (maxLen > 0 && len > maxLen) {
                    return { ok: false, message: "Maximum length is " + maxLen + " characters." };
                }
                return { ok: true };
            },

            hasWarning: function () { return warningActive; },
            warningMessage: function () { return warningMessage; }
        };
    }

    /* ---------------------------------------------------------------------
       Row construction
       --------------------------------------------------------------------- */

    function createRow(item) {
        var id = field(item, "Id", null);

        var row = {
            id:             id,
            item:           item,
            dirty:          false,
            statusState:    "idle",
            statusMessage:  "",
            saveToken:      0,
            showSaveButton: true,
            element:        null,
            statusEl:       null,
            control:        null
        };

        row.markDirty = function () { markDirty(row); };
        row.getValue  = function () { return row.control ? row.control.getValue() : null; };
        row.validate  = function () {
            return (row.control && row.control.validate) ? row.control.validate() : { ok: true };
        };

        var tr = document.createElement("tr");
        tr.className = "config-row";
        tr.setAttribute("data-id", String(id));

        /* ---- Name ---- */
        var tdName = document.createElement("td");
        tdName.textContent = String(field(item, "Label", "Unnamed"));
        tr.appendChild(tdName);

        /* ---- Description ---- */
        var tdDesc = document.createElement("td");
        tdDesc.className = "config-description";
        var desc = field(item, "Description", "");
        if (desc) tdDesc.appendChild(document.createTextNode(String(desc)));

        var info = field(item, "Info", "");
        if (info) {
            var infoEl = document.createElement("span");
            infoEl.className = "info";
            infoEl.textContent = String(info);
            tdDesc.appendChild(infoEl);
        }
        tr.appendChild(tdDesc);

        /* ---- Value cell ---- */
        var tdValue = document.createElement("td");

        var wrap = document.createElement("div");
        wrap.className = "config-input";

        var control = buildControl(item, row);
        row.control = control;

        // Hints above the control row (only numerics/strings have these)
        if (control.extra) wrap.appendChild(control.extra);

        var defaultBtn = document.createElement("button");
        defaultBtn.type = "button";
        defaultBtn.className = "btn btn--autowidth btn--right";
        defaultBtn.textContent = ICON.DEFAULT;
        defaultBtn.title = "Apply default value";

        var statusEl = document.createElement("span");
        statusEl.className = "config-status";
        row.statusEl = statusEl;

        if (isBool(item)) {
            /* ---- Bool: everything on one row ----
               [☑][💾]                          [Default]        */
            var row1 = document.createElement("div");
            row1.className = "input-row";
            row1.appendChild(control.element);
            row1.appendChild(statusEl);
            row1.appendChild(defaultBtn);
            wrap.appendChild(row1);
        } else {
            /* ---- Numeric / string: control row, then status + default row ----
               Min: 0                            Max: 86400
               [========slider========] [260]
               [💾]                              [Default]        */
            var inputRow = document.createElement("div");
            inputRow.className = "input-row";
            inputRow.appendChild(control.element);
            wrap.appendChild(inputRow);

            var statusRow = document.createElement("div");
            statusRow.className = "status-row";
            statusRow.appendChild(statusEl);
            statusRow.appendChild(defaultBtn);
            wrap.appendChild(statusRow);
        }

        tdValue.appendChild(wrap);
        tr.appendChild(tdValue);

        row.element = tr;

        defaultBtn.addEventListener("click", function () {
            var def = field(item, "DefaultValue", undefined);
            if (def === undefined) return;
            control.setValue(def);
            markDirty(row);
            saveRow(row);
        });

        renderStatus(row);
        return row;
    }

    function createGroupHeader(name) {
        var tr = document.createElement("tr");
        tr.className = "config-group-header";
        var td = document.createElement("td");
        td.colSpan = 3;
        td.textContent = String(name);
        tr.appendChild(td);
        return tr;
    }

    /* ---------------------------------------------------------------------
       Rendering
       --------------------------------------------------------------------- */

    function renderConfig(list) {
        rows = Object.create(null);

        if (!list.length) {
            var tr = document.createElement("tr");
            var td = document.createElement("td");
            td.colSpan = 3;
            td.className = "config-placeholder";
            td.textContent = "No configuration items were returned by the device.";
            tr.appendChild(td);
            ui.tableBody.appendChild(tr);
            return;
        }

        // De-duplicate by Id
        var seen = Object.create(null);
        var unique = [];
        list.forEach(function (item, index) {
            var id = field(item, "Id", "idx-" + index);
            var key = String(id);
            if (seen[key]) return;
            seen[key] = true;
            unique.push(item);
        });

        // Group by Metadata.Group, preserving first-seen order.
        var order  = [];
        var groups = Object.create(null);
        unique.forEach(function (item) {
            var g = String(field(item, "Group", "General"));
            if (!groups[g]) { groups[g] = []; order.push(g); }
            groups[g].push(item);
        });

        order.forEach(function (groupName) {
            ui.tableBody.appendChild(createGroupHeader(groupName));
            groups[groupName].forEach(function (item) {
                var row = createRow(item);
                if (row.id !== null && row.id !== undefined) {
                    rows[String(row.id)] = row;
                }
                ui.tableBody.appendChild(row.element);
            });
        });
    }

    /* ---------------------------------------------------------------------
       Global status / message helpers
       --------------------------------------------------------------------- */

    function setGlobalStatus(text) {
        if (ui.globalStatus) ui.globalStatus.textContent = text || "";
    }

    function showMessage(message, kind) {
        if (!ui.globalStatus) return;
        ui.globalStatus.textContent = message;
        ui.globalStatus.style.color = (kind === "error") ? "#f44336" : "#ccc";
    }

    /* ---------------------------------------------------------------------
       Bootstrap
       --------------------------------------------------------------------- */

    function init() {
        ui.tableBody    = document.getElementById("configTableBody");
        ui.refreshBtn   = document.getElementById("configRefreshButton");
        ui.globalStatus = document.getElementById("configGlobalStatus");

        if (!ui.tableBody) {
            console.error("Configuration: #configTableBody not found \u2013 cannot render.");
            return;
        }

        if (ui.refreshBtn) {
            ui.refreshBtn.addEventListener("click", function () {
                loadConfig();
            });
        }

        loadConfig();
    }

    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", init);
    } else {
        init();
    }
})();