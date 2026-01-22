var ui;

function updateTime() {
    const el = document.getElementById('currentTime');
    if (el) {
        let now = new Date();
        let timeString =
            now.getHours().toString().padStart(2, '0') + ':' +
            now.getMinutes().toString().padStart(2, '0') + ':' +
            now.getSeconds().toString().padStart(2, '0');
        el.innerText = timeString;
    }
}

// rangeExpansion = 0->1 = 0->100% +/- extra % outside of max range
function randomisedValue(min, max, rangeExpansion) {
    const range = max - min;
    const extra = range * rangeExpansion;   // allow going outside of range
    const low = min - extra;
    const high = max + extra;
    return (Math.random() * (high - low)) + low;
}

function startAutoRefresh() {
    ui = {
        content: document.getElementById('content'),
        response: document.getElementById('uiResponse'),
        batteryLevel: document.getElementById('uiBatteryLevel'),
        batteryVoltage: document.getElementById('uiBatteryVoltage'),
        wifiSignal: document.getElementById('uiWiFiSignal'),
        wifiSignalLabel: document.getElementById('uiWiFiSignalLabel')
    };

    // Load initial device info and battery info
    updateBatteryInformation();
    updateWiFiInformation();
    updateTime();

    if (typeof IntervalManager !== "undefined") {
        IntervalManager.add(() => {
            updateBatteryInformation();
            updateWiFiInformation();
        }, 10000);

        IntervalManager.add(() => {
            updateTime();
        }, 1000);
    }

    // Test's (remember to disable updates above which will probably trigger after the tests run and overwrite them)
    // let batteryVoltage = randomisedValue(3.3, 4.2, 0.15);
    // let batteryLevel = randomisedValue(0, 100, 0.15);
    // let wifiSignal = randomisedValue(-90, -30, 0.15);
    // setGauge(ui.batteryVoltage, 3.3, 4.2, batteryVoltage, "v", "⚡", "Test");
    // setGauge(ui.batteryLevel, 0, 100, batteryLevel, "%", "🔋", "Test");
    // setGauge(ui.wifiSignal, -30, -90, wifiSignal, "dBm", "🗼", "☠️");
}

function showResponseError(message = 'Please check controller is powered on and successfully connected to your WiFi network') {
    ui.response.innerHTML = '<span style="color: red;"><strong>Error</strong> - ' + message + '</span>';
}

function showResponse(message) {
    ui.response.innerHTML = '<span style="color: green;">' + message + '</span>';
}

function clearResponse() {
    ui.response.innerHTML = '';
}

// Accounts for
// - input ascending or descending
// - output ascending or descending
// - positive or negative values
function mapValue(x, inMin, inMax, outMin, outMax) {
    const t = (x - inMin) / (inMax - inMin);      // normalized 0–1 (or 1–0 if reversed)
    const y = outMin + t * (outMax - outMin);     // scale to output range

    // clamp to whichever bound is lower/higher
    return Math.max(Math.min(outMin, outMax),
        Math.min(Math.max(outMin, outMax), y));
}

// Clamp that accounts for positive and negative values
function clamp(value, a, b) {
    return Math.max(Math.min(a, b), Math.min(Math.max(a, b), value));
}

function setGauge(gauge, min, max, value, unit, icon, empty) {
    const fillElem = gauge.querySelector('.gauge-fill');
    const valueElem = gauge.querySelector('.gauge-text .value');
    const unitElem = gauge.querySelector('.gauge-text .unit');
    const warningElem = gauge.querySelector('.gauge-warning');

    // const range = max - min;
    // let pct = Math.max(0, Math.min (1, (value - min) / range)) * 100;

    // Accounts for positive ranges and negative ranges
    const clampedValue = clamp(value, min, max);
    let pct = mapValue(clampedValue, min, max, 0, 100);

    // Out of range handling
    // Always show a minimum amount
    if (pct < 1)
        pct = 1;
    else if (pct > 100)
        pct = 100;

    if (pct < 5)
        warningElem.textContent = empty;
    else
        warningElem.textContent = icon;

    // Earlier version had dynamic gradients. Here for reference.
    // fillElem.style.background = 'conic-gradient(from -120deg, #ff0000 0deg, #ff0000 10deg, #ff9900 30deg, #ffff00 60deg, #00ff00 90deg, #00ff00 240deg)';

    // Update CSS variable for fill percentage
    fillElem.style.setProperty('--fill-pct', pct);

    valueElem.textContent = value.toFixed(1);
    unitElem.textContent = unit;
}

function updateBatteryInformation() {
    fetch('/json/battery_info')
        .then(response => {
            if (!response.ok) throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            return response.json();
        })
        .then(data => {
            let batteryVoltage = data.BatteryVoltage;
            let batteryLevel = data.BatteryLevel;

            setGauge(ui.batteryVoltage, 3.3, 4.2, batteryVoltage, "v", "⚡", "Empty");
            setGauge(ui.batteryLevel, 0, 100, batteryLevel, "%", "🔋", "Empty");
        })
        .catch(error => {
            console.error('Error fetching battery information: ', error);
            showResponseError("Unable to get battery information.");

            setGauge(ui.batteryVoltage, 3.3, 4.2, 0, "v", "⚡", "❓");
            setGauge(ui.batteryLevel, 0, 100, 0, "%", "🔋", "❓");
        });
}

function updateWiFiInformation() {
    fetch('/json/wifi_status')
        .then(response => {
            if (!response.ok) throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            return response.json();
        })
        .then(data => {
            // Signal Level...
            // -30 is amazing
            // -90 is crap
            // That's our range, anything else can be clamped
            // let percent = mapValue(data.RSSI, -30, -90, 100, 0);

            setGauge(ui.wifiSignal, -30, -90, data.RSSI, "dBm", "🗼", "☠️");
            ui.wifiSignalLabel.textContent = data.Status;
        })
        .catch(error => {
            console.error('Error fetching WiFi Signal Level: ', error);
            showResponseError("Unable to get WiFi Signal Level.");
            ui.wifiSignalLabel.textContent = "WiFi Signal";
            setGauge(ui.wifiSignal, -30, -90, -30, "dBm", "🗼", "☠️");
        });
}

startAutoRefresh();