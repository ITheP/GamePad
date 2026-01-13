
const responseDiv = document.getElementById('formResponse');

// Efficient placeholder replacement with single-pass algorithm
function replacePlaceholders(html, dataMap) {
    const result = [];
    let i = 0;

    while (i < html.length) {
        if (html[i] === '{') {
            // Find the closing brace
            const endIdx = html.indexOf('}', i);
            if (endIdx !== -1) {
                // Extract the placeholder name
                const placeholder = html.substring(i + 1, endIdx);
                // Add the value from dataMap, or keep the placeholder if not found
                result.push(dataMap.hasOwnProperty(placeholder) ? dataMap[placeholder] : html.substring(i, endIdx + 1));
                i = endIdx + 1;
            } else {
                // No closing brace, just add the character
                result.push(html[i]);
                i++;
            }
        } else {
            result.push(html[i]);
            i++;
        }
    }

    return result.join('');
}

function loadHotspotContent() {
    fetch('/json/hotspot_info')
        .then(response => {
            if (!response.ok) throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            return response.json();
        })
        .then(data => {
            // Get the content div
            const contentDiv = document.getElementById('content');

            // Replace placeholders with actual data
            const processedHtml = replacePlaceholders(contentDiv.innerHTML, data);
            contentDiv.innerHTML = processedHtml;

            // Handle input fields separately - they need special treatment
            const ssidInput = document.getElementById('ssid');
            const passwordInput = document.getElementById('password');

            if (ssidInput && data.WiFiSSID !== undefined) {
                ssidInput.value = data.WiFiSSID;
            }
            if (passwordInput && data.WiFiPassword !== undefined) {
                passwordInput.value = data.WiFiPassword;
            }
        })
        .catch(error => {
            responseDiv.innerHTML = '<span style="color: red;"><strong>Error</strong> - Please check controller is powered on and is in Hotspot mode.</span>';
            console.error('Error fetching hotspot info:', error);
        });
}

function updateAccessPoints() {
    fetch('/json/access_points')
        .then(response => {
            if (!response.ok) throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            return response.json();
        })
        .then(data => {
            const datalist = document.getElementById('ssidList');
            datalist.innerHTML = '';
            if (data.accessPoints && Array.isArray(data.accessPoints)) {
                data.accessPoints.forEach(ap => {
                    const option = document.createElement('option');
                    option.value = ap.SSID;
                    datalist.appendChild(option);
                });
            }
        })
        .catch(error => {
            responseDiv.innerHTML = '<span style="color: red;"><strong>Error</strong> - Please check controller is powered on and is in Hotspot mode.</span>';
            console.error('Error fetching access points:', error);
        });
}

function updateWiFiStatus() {
    fetch('/json/wifi_status')
        .then(response => {
            if (!response.ok) throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            return response.json();
        })
        .then(data => {
            const statusSpan = document.getElementById('wifiStatus');
            if (data.Status) {
                statusSpan.textContent = data.Status;
            }
        })
        .catch(error => {
            const statusSpan = document.getElementById('wifiStatus');
            statusSpan.innerHTML = '<span style="color: red;"><strong>Unable to fetch status.</strong></span>';

            responseDiv.innerHTML = '<span style="color: red;"><strong>Error</strong> - Please check controller is powered on and is in Hotspot mode.</span>';
            console.error('Error fetching wifi status:', error);
        });
}

function handleWiFiFormSubmit(event) {
    event.preventDefault();
    const form = document.getElementById('wifiForm');

    const formData = new FormData(form);

    fetch('/api/UpdateWifiDetails', {
        method: 'POST',
        body: formData
    })
        .then(response => {
            if (!response.ok) throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            return response.json();
        })
        .then(data => {
            let message = '';
            if (data.error) {
                message = '<p style="color: red;"><strong>Error:</strong> ' + escapeHtml(data.error) + '</p>';
            } else if (data.status === 'ok') {
                message = '<p style="color: green;"><strong>Success:</strong> WiFi details saved</p>';
            } else {
                message = '<p style="color: orange;"><strong>Response:</strong> ' + escapeHtml(JSON.stringify(data)) + '</p>';
            }
            responseDiv.innerHTML = message;
        })
        .catch(error => {
            responseDiv.innerHTML = '<p style="color: red;"><strong>Error:</strong> ' + escapeHtml(error.message) + '</p>';
        });
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

function startConfigAutoRefresh() {
    // Load initial hotspot info (placeholders + WiFi form values)
    loadHotspotContent();

    updateAccessPoints();
    updateWiFiStatus();
    IntervalManager.add(() => {
        updateAccessPoints();
        updateWiFiStatus();
    }, 10000);

    const form = document.getElementById('wifiForm');
    if (form) {
        form.addEventListener('submit', handleWiFiFormSubmit);
    }
}

// Start configuration when page is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', startConfigAutoRefresh);
} else {
    startConfigAutoRefresh();
}
