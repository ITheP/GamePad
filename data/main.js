var countdown = 5;
var refreshInterval;

const ui = {
    content: document.getElementById('content'),
    response: document.getElementById('uiResponse'),
    batteryLevel: document.getElementById('uiBatteryLevel'),
    batteryVoltage: document.getElementById('uiBatteryVoltage')
};

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

function startAutoRefresh() {
    // Load initial device info and battery info
    updateBatteryInformation();
    updateTime();

    IntervalManager.add(() => {
        updateBatteryInformation();
    }, 10000);

    IntervalManager.add(() => {
        updateTime();
    }, 1000);
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

function setGauge(id, min, max, value, unit, icon, empty) {
    const gauge = document.getElementById(id);
    if (!gauge) return;

    const fillElem = gauge.querySelector('.gauge-fill');
    const valueElem = gauge.querySelector('.gauge-text .value');
    const unitElem = gauge.querySelector('.gauge-text .unit');
    const warningElem = gauge.querySelector('.gauge-warning');

    const range = max - min;
    let pct = Math.max(0, Math.min(1, (value - min) / range)) * 100;

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

// rangeExpansion = 0->1 = 0->100% +/- extra % outside of max range
// function randomisedValue(min, max, rangeExpansion) {
//     const range = max - min;
//     const extra = range * rangeExpansion;   // allow going outside of range
//     const low = min - extra;
//     const high = max + extra;
//     return (Math.random() * (high - low)) + low;
// }

function updateBatteryInformation() {
    fetch('/json/battery_info')
        .then(response => {
            if (!response.ok) throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            return response.json();
        })
        .then(data => {
            // Handle input fields separately - they need special treatment
            if (ui.batteryLevel && data.BatteryLevel !== undefined) {
                ui.batteryLevel.innerHTML = data.BatteryLevel + "%";
            }
            if (ui.batteryVoltage && data.BatteryVoltage !== undefined) {
                ui.batteryVoltage.value = data.BatteryVoltage + "v";
            }

            let batteryVoltage = randomisedValue(3.7, 4.7, 0.15);
            let batteryLevel = randomisedValue(0, 100, 0.15);


            setGauge("gaugeVoltage", 3.3, 4.2, batteryVoltage, "v", "⚡", "Empty");
            setGauge("gaugePercent", 0, 100, batteryLevel, "%", "🔋", "Empty");


        })
        .catch(error => {
            console.error('Error fetching battery information:', error);
            showResponseError("Unable to get battery information.");

            setGauge("gaugeVoltage", 3.3, 4.2, 0, "v", "⚡", "❓");
            setGauge("gaugePercent", 0, 100, 0, "%", "🔋", "❓");
        });

    // Test's
    // let batteryVoltage = randomisedValue(3.3, 4.2, 0.15);
    // let batteryLevel = randomisedValue(0, 100, 0.15);
    // setGauge("gaugeVoltage", 3.3, 4.2, batteryVoltage, "v", "⚡", "Empty");
    // setGauge("gaugePercent", 0, 100, batteryLevel, "%", "🔋", "Empty");
}

startAutoRefresh();