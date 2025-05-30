let countdown = 5;
let refreshInterval;
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
        clearInterval(refreshInterval);
        document.getElementById('countdownDisplay').innerHTML = '';
    }
}

function startAutoRefresh() {
    countdown = 5;
    document.getElementById('countdownDisplay').innerHTML = countdown + 's until refresh';
    refreshInterval = setInterval(() => {
        countdown--;
        document.getElementById('countdownDisplay').innerHTML = countdown + 's until refresh';
        if (countdown <= 0) {
            fetchStats();
            countdown = 5;
        }
    }, 1000);
}

function fetchStats() {
    fetch('/component/stats')
        .then(response => {
            if (!response.ok) { throw new Error('Network response was not ok'); }
            return response.text();
        })
        .then(html => { document.getElementById('statsTable').innerHTML = html; })
        .catch(() => {
            document.getElementById('statsTable').innerHTML = '<p class=\warning\>Sorry, unable to communicate with GamePad to retrieve information. It may be turned off, not connected, lost connection or not have its Wifi and Web Service turned on.</p>';
        });
}

// window.onload = function () {
    let savedState = localStorage.getItem('autoRefresh') !== 'false';
    document.getElementById('refreshBox').checked = savedState;
    updateTime();
    setInterval(updateTime, 1000);
    if (savedState) {
        startAutoRefresh();
    }
// }