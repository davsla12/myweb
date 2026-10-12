// 1. Po načtení stránky se načtou programy (složky)
document.addEventListener('DOMContentLoaded', () => {
    fetch('/api/programs')
        .then(response => {
            if (!response.ok) throw new Error('API s programy nedostupné');
            return response.json();
        })
        .then(programy => {
            const select = document.getElementById('programSelect');
            programy.forEach(program => {
                const opt = document.createElement('option');
                opt.value = program.soubor;
                opt.textContent = program.nazev;
                select.appendChild(opt);
            });
        })
        .catch(err => console.error('Chyba při načítání programů:', err));
});

// Pomocná funkce pro kompletní zastavení a reset přehrávače
function resetPlayer() {
    const playerContainer = document.getElementById('playerContainer');
    const videoPlayer = document.getElementById('videoPlayer');
    const videoSource = document.getElementById('videoSource');
    const subTrack = document.getElementById('subTrack');

    if (videoPlayer) {
        videoPlayer.pause();
        if (videoSource) videoSource.removeAttribute('src');
        if (subTrack) subTrack.removeAttribute('src');
        videoPlayer.load(); // Uvolní streamovaný soubor z paměti
    }

    if (playerContainer) {
        playerContainer.style.display = 'none';
    }
}

// 2. Po změně programu se resetuje přehrávač a načtou se nová videa
function LoadvideoSelect() {
    const folder = document.getElementById('programSelect').value;
    const videoSelect = document.getElementById('videoSelect');

    // Vždy nejprve zastavíme spuštěné video a vyčistíme nabídku
    resetPlayer();
    videoSelect.innerHTML = '<option value="">-- Zvolte epizodu / film --</option>';

    if (!folder) return;

    // encodeURIComponent zajistí správné předání mezer a závorek v URL
    fetch(`/api/videa/${encodeURIComponent(folder)}`)
        .then(response => {
            if (!response.ok) throw new Error('API s videi nedostupné');
            return response.json();
        })
        .then(videa => {
            videa.forEach(video => {
                const opt = document.createElement('option');
                opt.value = `${folder}/${video.soubor}`;
                opt.setAttribute('data-sub', video.titulky ? `${folder}/${video.titulky}` : '');
                opt.textContent = video.nazev;
                videoSelect.appendChild(opt);
            });
        })
        .catch(err => console.error('Chyba při načítání videí:', err));
}

// 3. Spuštění vybraného videa
function loadVideo() {
    const select = document.getElementById('videoSelect');
    if (!select || select.selectedIndex < 0) return;

    const selectedOption = select.options[select.selectedIndex];
    const videoPath = selectedOption.value;
    const subPath = selectedOption.getAttribute('data-sub');

    const playerContainer = document.getElementById('playerContainer');
    const videoPlayer = document.getElementById('videoPlayer');
    const videoSource = document.getElementById('videoSource');
    const subTrack = document.getElementById('subTrack');

    if (!videoPath) {
        resetPlayer();
        return;
    }

    const cleanVideoPath = videoPath.replace(/^\//, '');
    const cleanSubPath = subPath ? subPath.replace(/^\//, '') : '';

    videoSource.src = `/files/${cleanVideoPath}`;
    subTrack.src = cleanSubPath ? `/files/${cleanSubPath}` : '';

    playerContainer.style.display = 'block';
    videoPlayer.disablePictureInPicture = true;
    videoPlayer.autoplay = false;
    videoPlayer.load();
}
