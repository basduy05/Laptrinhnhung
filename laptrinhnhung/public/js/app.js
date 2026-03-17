document.addEventListener("DOMContentLoaded", () => {
    const btnSpeak = document.getElementById("btn-speak");
    const statusDiv = document.getElementById("status");
    const resultDiv = document.getElementById("result");

    let englishVoice = null;

    function initVoices() {
        const voices = window.speechSynthesis.getVoices();
        if (!voices || voices.length === 0) return;

        const normalized = (s) => (s || '').toLowerCase();
        const scored = voices.map(v => ({
            v,
            lang: normalized(v.lang),
            name: normalized(v.name),
            local: !!v.localService
        }));

        const scoreVoice = (x, preferredLang) => {
            let s = 0;
            const pref = preferredLang.toLowerCase();
            if (x.lang === pref) s += 50;
            if (x.lang.startsWith(pref.split('-')[0])) s += 15;
            if (x.name.includes('microsoft')) s += 10;
            if (x.local) s += 5;

            // Prefer female/younger-sounding voices when available (best-effort)
            const femaleHints = ['female', 'jenny', 'aria', 'sara', 'zira', 'natasha', 'emma', 'olivia', 'amy', 'thuy', 'linh'];
            if (femaleHints.some(h => x.name.includes(h))) s += 12;
            const maleHints = ['male', 'david', 'mark', 'guy', 'george'];
            if (maleHints.some(h => x.name.includes(h))) s -= 6;

            return s;
        };

        const enCandidates = scored
            .filter(x => x.lang.startsWith('en'))
            .sort((a, b) => scoreVoice(b, 'en-US') - scoreVoice(a, 'en-US'));
        englishVoice = enCandidates.length ? enCandidates[0].v : null;

        // English-only UX: do not pick Vietnamese voices.
    }

    window.speechSynthesis.onvoiceschanged = initVoices;
    initVoices();

    function speakFriendly(text) {
        if (!('speechSynthesis' in window)) return;
        window.speechSynthesis.cancel();

        const u = new SpeechSynthesisUtterance(text);
        u.lang = 'en-US';
        // Friendlier, more energetic tone (subtle)
        u.rate = 1.10;
        u.pitch = 1.18;
        u.volume = 1.0;

        if (englishVoice) u.voice = englishVoice;

        window.speechSynthesis.speak(u);
    }

    // Kiểm tra trình duyệt hỗ trợ
    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
    if (!SpeechRecognition) {
        alert("Trình duyệt không hỗ trợ nhận diện giọng nói!");
        return;
    }

    const recognition = new SpeechRecognition();
    // English-only
    recognition.lang = 'en-US';
    recognition.continuous = false;

    btnSpeak.addEventListener("click", () => {
        recognition.start();
        statusDiv.innerText = "Listening...";
        btnSpeak.disabled = true;
    });

    recognition.onresult = (event) => {
        const command = event.results[0][0].transcript;
        statusDiv.innerText = "Heard: " + command;
        processCommand(command);
    };

    recognition.onerror = (event) => {
        statusDiv.innerText = "Error: " + event.error;
        btnSpeak.disabled = false;
    };

    recognition.onend = () => {
        btnSpeak.disabled = false;
    };

    function processCommand(text) {
        resultDiv.innerText = "Sending to AI...";
        
        const formData = new FormData();
        formData.append('command', text);

        // Gọi về Controller
        fetch(`${BASE_URL}/AiController/process`, {
            method: 'POST',
            body: formData
        })
        .then(response => response.json())
        .then(data => {
            console.log(data);
            if(data.status === 'success') {
                const reply = (data.ai_action && data.ai_action.speak) ? data.ai_action.speak : '';
                resultDiv.innerText = "AI: " + reply;
                speakFriendly(reply);
            } else {
                resultDiv.innerText = "Error: " + data.message;
            }
        })
        .catch(error => {
            console.error(error);
            resultDiv.innerText = "Server connection error!";
        });
    }
});