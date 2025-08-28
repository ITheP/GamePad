var countdown = 5;
var refreshInterval;

function updateTime() {
    let now = new Date();
    let timeString = now.getHours().toString().padStart(2, '0') + ':'
        + now.getMinutes().toString().padStart(2, '0') + ':'
        + now.getSeconds().toString().padStart(2, '0');
    document.getElementById('currentTime').innerText = timeString;
}

function startAutoRefresh() {
    IntervalManager.add(() => {
        updateTime();
    }, 1000);
}

startAutoRefresh();