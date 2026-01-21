// TO DO
// Make the loading work so the ui references are correct
// Make the screen default to "Loading..." until data is fetched
// On error, show error and insert into all values <unable to get data>
// Stick a retry button in the error message that refreshes the whole page

function refreshUIReferences() {
    ui = {
        content: document.getElementById('content'),
        form: document.getElementById('uiForm'),

        ssid: document.getElementById('uiSSID'),
        password: document.getElementById('uiPassword'),
        response: document.getElementById('uiResponse'),
        wifiStatus: document.getElementById('uiWiFiStatus'),

        ssidList: document.getElementById('ssidList')
    }
}

const IntervalManager = (() => {
    const ids = [];

    return {
        add(fn, delay) {
            const id = setInterval(fn, delay);
            ids.push(id);
            return id;
        },
        clearAll() {
            ids.forEach(clearInterval);
            ids.length = 0;
        }
    };
})();

function togglePasswordVisibility() {
    const p = ui.password;
    p.type = (p.type === 'password') ? 'text' : 'password';
}

function showResponseError(message = 'Unable to communicate with controller. Please check it is currently in Hotspot mode and you are connected to its WiFi access point.') {
    ui.response.innerHTML = '<span style="color: red;"><strong>Error</strong> - ' +
    message +
    '<br/><button onclick="location.reload()">Refresh</button>' +
    '</span>';
}

// // Efficient placeholder replacement with single-pass algorithm
// function replacePlaceholders(html, dataMap) {
//     const result = [];
//     let i = 0;

//     while (i < html.length) {
//         if (html[i] === '{') {
//             // Find the closing brace
//             const endIdx = html.indexOf('}', i);
//             if (endIdx !== -1) {
//                 // Extract the placeholder name
//                 const placeholder = html.substring(i + 1, endIdx);
//                 // Add the value from dataMap, or keep the placeholder if not found

//                 result.push(dataMap.hasOwnProperty(placeholder) ? dataMap[placeholder] : html.substring(i, endIdx + 1));
//                 i = endIdx + 1;
//             } else {
//                 // No closing brace, just add the character
//                 result.push(html[i]);
//                 i++;
//             }
//         } else {
//             result.push(html[i]);
//             i++;
//         }
//     }

//     return result.join('');
// }

function loadHotspotContent() {
    fetch('/json/hotspot_info')
        .then(response => {
            if (!response.ok) throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            return response.json();
        })
        .then(data => {
            if (ui.ssid && data.WiFiSSID !== undefined) {
                ui.ssid.value = data.WiFiSSID;
            }
            if (ui.password && data.WiFiPassword !== undefined) {
                ui.password.value = data.WiFiPassword;
            }
        })
        .catch(error => {
            console.error('Error fetching hotspot info:', error);
            showResponseError();
        });
}

function updateAccessPoints() {
    fetch('/json/access_points')
        .then(response => {
            if (!response.ok) throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            return response.json();
        })
        .then(data => {
            const datalist = ui.ssidList;
            ui.ssidList.innerHTML = '';
            if (data.accessPoints && Array.isArray(data.accessPoints)) {
                data.accessPoints.forEach(ap => {
                    const option = document.createElement('option');
                    option.value = ap.SSID;
                    datalist.appendChild(option);
                });
            }
        })
        .catch(error => {
            console.error('Error fetching access points:', error);
            showResponseError();
        });
}

function updateWiFiStatus() {
    fetch('/json/wifi_status')
        .then(response => {
            if (!response.ok) throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            return response.json();
        })
        .then(data => {
            if (data.Status) {
                ui.wifiStatus.textContent = data.Status;
            }
        })
        .catch(error => {
            console.error('Error fetching wifi status:', error);
            ui.wifiStatus.innerHTML = '<span style="color: red;"><strong>Unable to fetch status.</strong></span>';
            showResponseError();
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
            console.error('Error updating WiFi details: ', error);
            showResponseError(escapeHtml(error.message));
        });
}

const escapeMap = {
    '&': '&amp;',
    '<': '&lt;',
    '>': '&gt;',
    '"': '&quot;',
    "'": '&#039;'
};

function escapeHtml(text) {
    // const div = document.createElement('div');
    // div.textContent = text;
    // return div.innerHTML;

    return text.replace(/[&<>"']/g, m => escapeMap[m]);
}

function startConfigAutoRefresh() {
    refreshUIReferences();
    // Load initial hotspot info (placeholders + WiFi form values)
    loadHotspotContent();
    updateAccessPoints();
    updateWiFiStatus();

    IntervalManager.add(() => {
        updateAccessPoints();
        updateWiFiStatus();
    }, 10000);

    if (ui.form) {
        ui.form.addEventListener('submit', handleWiFiFormSubmit);
    }
}

// Start configuration when page is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', startConfigAutoRefresh);
} else {
    startConfigAutoRefresh();
}
