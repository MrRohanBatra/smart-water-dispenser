async function updateStatus() {
    let response = await fetch("/state");
    let data = await response.json();
    document.getElementById("status").textContent = data.state ? "ON" : "OFF";
}

async function dispense() {
    let volume = document.getElementById("volume").value;
    if (volume > 0 && volume <= 1000) {
        await fetch(`/dispense?ml=${volume}`);
        updateStatus();
    } else {
        alert("Enter a valid amount (1-1000mL)");
    }
}

setInterval(updateStatus, 2000);
updateStatus();