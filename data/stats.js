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
        IntervalManager.clearAll();
        document.getElementById('countdownDisplay').innerHTML = '';
    }
}

var statsStatus = document.getElementById('countdownDisplay');
var fetchingStats = false;

function startAutoRefresh() {
    countdown = 5;

    IntervalManager.add(() => {
        if (fetchingStats == false) {
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
                IntervalManager.clearAll();
                statsStatus.innerText = 'Error fetching stats: ' + (error?.message || error);
            }
        }
    }, 1000);
}

function fetchStats() {
    fetchingStats = true;

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

            fetchingStats = false;
        })
        .catch(() => {
            statsStatus.innerText = 'Fetching latest stats failed, will try again in a few seconds.';
            countdown = 8;

            fetchingStats = false;
        });
}

var savedState = localStorage.getItem('autoRefresh') !== 'false';
fetchStats();
document.getElementById('refreshBox').checked = savedState;
if (savedState) {
    startAutoRefresh();
}