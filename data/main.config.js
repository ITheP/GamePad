function updateAccessPoints() {
    fetch('/json/access_points')
        .then(response => response.json())
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
        .catch(error => console.error('Error fetching access points:', error));
}

function updateWiFiStatus() {
    fetch('/json/wifi_status')
        .then(response => response.json())
        .then(data => {
            const statusSpan = document.getElementById('wifiStatus');
            if (data.Status) {
                statusSpan.textContent = data.Status;
            }
        })
        .catch(error => console.error('Error fetching wifi status:', error));
}

function handleWiFiFormSubmit(event) {
    event.preventDefault();
    const form = document.getElementById('wifiForm');
    const responseDiv = document.getElementById('formResponse');
    
    const formData = new FormData(form);
    
    fetch('/api/UpdateWifiDetails', {
        method: 'POST',
        body: formData
    })
    .then(response => response.json())
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

startConfigAutoRefresh();
