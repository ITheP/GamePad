var countdown = 5;
var refreshInterval;
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

var statsStatus = document.getElementById('countdownDisplay');

function startAutoRefresh() {
    countdown = 5;
    
    refreshInterval = setInterval(() => {
        try {
            if (countdown > 1) {
                countdown--;
                if (countdown < 6)
                    statsStatus.innerHTML = countdown + 's until refresh';
            }
            else if (countdown == 1) {
                countdown--;    // Make sure this doesn't trigger while fetchStats is running
                fetchStats();
            }
        }
        catch (error) {
            clearInterval(refreshInterval);
        }
    }, 1000);
}

function fetchStats() {
    statsStatus.innerText = 'Fetching latest stats...';

    fetch('/component/stats')
        .then(response => {
            if (!response.ok) { throw new Error('Network response was not ok'); }
            return response.text();
        })
        .then(html => {
            statsTable.innerHTML = html;
            countdown = 6;
            statsStatus.innerText = 'Stats updated';
            updateTime();
        })
        .catch(() => {
            statsStatus.innerText = 'Fetching latest stats failed, will try again in a few seconds.';
            countdown = 8;
        });
}

var savedState = localStorage.getItem('autoRefresh') !== 'false';
fetchStats();
document.getElementById('refreshBox').checked = savedState;
if (savedState) {
    startAutoRefresh();
}