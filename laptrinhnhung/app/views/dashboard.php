<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Miki - Light Hub</title>
    
    <script src="https://cdn.tailwindcss.com"></script>
    <script src="https://unpkg.com/@phosphor-icons/web"></script>
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;500;600;700;800&display=swap" rel="stylesheet">

    <script>
        tailwind.config = {
            theme: {
                extend: {
                    fontFamily: { sans: ['Outfit', 'sans-serif'] },
                    colors: { 
                        glass: 'rgba(255, 255, 255, 0.7)',
                        primary: '#6366f1', 
                    },
                    animation: {
                        'blob': 'blob 10s infinite',
                        'float': 'float 6s ease-in-out infinite',
                        'wave': 'wave 1.2s linear infinite'
                    },
                    keyframes: {
                        blob: {
                            '0%': { transform: 'translate(0px, 0px) scale(1)' },
                            '33%': { transform: 'translate(30px, -50px) scale(1.1)' },
                            '66%': { transform: 'translate(-20px, 20px) scale(0.9)' },
                            '100%': { transform: 'translate(0px, 0px) scale(1)' }
                        },
                        float: {
                            '0%, 100%': { transform: 'translateY(0)' },
                            '50%': { transform: 'translateY(-10px)' }
                        },
                        wave: {
                            '0%, 100%': { height: '10%' },
                            '50%': { height: '100%' }
                        }
                    }
                }
            }
        }
    </script>

    <style>
        body {
            background-color: #f8fafc; /* Slate 50 */
            color: #334155; /* Slate 700 */
            overflow-x: hidden;
        }

        /* Avoid double scrollbars on desktop: keep scrolling inside <main> */
        @media (min-width: 768px) {
            body { overflow-y: hidden; }
        }

        :root {
            /* Theme variables (daylight defaults) */
            --mesh-a: hsla(253,16%,96%,1);
            --mesh-b: hsla(225,39%,96%,1);
            --mesh-c: hsla(339,49%,97%,1);
            --blob-a: rgba(167, 139, 250, 0.30);
            --blob-b: rgba(196, 181, 253, 0.20);
            --blob-c: rgba(56, 189, 248, 0.20);
            --night-overlay: 0;
        }

        body.theme-morning {
            /* Cooler, brighter morning */
            --mesh-a: hsla(228, 55%, 97%, 1);
            --mesh-b: hsla(253, 45%, 97%, 1);
            --mesh-c: hsla(205, 70%, 97%, 1);
            --blob-a: rgba(99, 102, 241, 0.22);
            --blob-b: rgba(167, 139, 250, 0.18);
            --blob-c: rgba(56, 189, 248, 0.18);
            --night-overlay: 0;
        }

        body.theme-afternoon {
            --mesh-a: hsla(42, 92%, 96%, 1);
            --mesh-b: hsla(225, 39%, 96%, 1);
            --mesh-c: hsla(340, 70%, 97%, 1);
            /* Warmer, more noticeable afternoon */
            --blob-a: rgba(251, 191, 36, 0.24);
            --blob-b: rgba(244, 114, 182, 0.18);
            --blob-c: rgba(99, 102, 241, 0.14);
            --night-overlay: 0;
        }

        body.theme-evening {
            --mesh-a: hsla(35, 90%, 95%, 1);
            --mesh-b: hsla(20, 90%, 96%, 1);
            --mesh-c: hsla(225, 40%, 96%, 1);
            /* Slightly dimmed evening + warm glow */
            --blob-a: rgba(251, 146, 60, 0.26);
            --blob-b: rgba(244, 63, 94, 0.14);
            --blob-c: rgba(99, 102, 241, 0.12);
            --night-overlay: 0.12;
        }

        body.theme-night {
            --mesh-a: hsla(222, 47%, 11%, 1);
            --mesh-b: hsla(223, 47%, 14%, 1);
            --mesh-c: hsla(228, 45%, 18%, 1);
            --blob-a: rgba(99, 102, 241, 0.22);
            --blob-b: rgba(56, 189, 248, 0.10);
            --blob-c: rgba(236, 72, 153, 0.12);
            --night-overlay: 0.55;
        }
        
        /* Light Gradient Background */
        .bg-gradient-mesh {
            background: radial-gradient(at 0% 0%, var(--mesh-a) 0, transparent 55%), 
                        radial-gradient(at 50% 0%, var(--mesh-b) 0, transparent 55%), 
                        radial-gradient(at 100% 0%, var(--mesh-c) 0, transparent 55%);
            position: fixed; top: 0; left: 0; right: 0; bottom: 0; z-index: -2;
        }

        .bg-night-overlay {
            position: fixed;
            inset: 0;
            z-index: -2;
            background: rgba(15, 23, 42, var(--night-overlay));
            pointer-events: none;
        }
        
        .blob-bg {
            position: absolute; width: 600px; height: 600px; 
            background: linear-gradient(180deg, var(--blob-a) 0%, var(--blob-b) 100%);
            filter: blur(80px); border-radius: 50%; z-index: -1; animation: blob 15s infinite alternate;
        }

        .blob-bg.blob-accent {
            background: var(--blob-c);
        }

        .clock-glass {
            background: rgba(255, 255, 255, 0.55);
            backdrop-filter: blur(28px);
            -webkit-backdrop-filter: blur(28px);
        }

        body.theme-night .clock-glass {
            background: rgba(15, 23, 42, 0.30);
            border-color: rgba(255,255,255,0.12) !important;
        }

        body.theme-night #clock-time {
            color: #f8fafc;
        }

        body.theme-night #clock-date {
            color: rgba(199, 210, 254, 0.95);
        }

        body.theme-night .bento-card {
            border-color: rgba(255, 255, 255, 0.14);
        }

        body.theme-night {
            color: #e2e8f0;
            color-scheme: dark;
        }

        body.theme-night .text-slate-800 { color: #f8fafc !important; }
        body.theme-night .text-slate-700 { color: #e2e8f0 !important; }
        body.theme-night .text-slate-600 { color: #cbd5e1 !important; }
        body.theme-night .text-slate-500 { color: rgba(226, 232, 240, 0.72) !important; }
        body.theme-night .text-slate-400 { color: rgba(226, 232, 240, 0.62) !important; }

        body.theme-night .bento-card {
            background: rgba(15, 23, 42, 0.42);
            box-shadow: 0 10px 40px -10px rgba(0, 0, 0, 0.35),
                        0 0 20px rgba(255, 255, 255, 0.06) inset;
        }

        body.theme-night .bento-card:hover {
            background: rgba(15, 23, 42, 0.55);
            box-shadow: 0 20px 50px -16px rgba(0, 0, 0, 0.45);
        }

        body.theme-night input,
        body.theme-night select,
        body.theme-night textarea {
            background: rgba(15, 23, 42, 0.28) !important;
            border-color: rgba(255, 255, 255, 0.16) !important;
            color: #e2e8f0 !important;
        }

        body.theme-night input::placeholder,
        body.theme-night textarea::placeholder {
            color: rgba(226, 232, 240, 0.55) !important;
        }

        body.theme-night .bg-white\/60,
        body.theme-night .bg-white\/40,
        body.theme-night .bg-slate-50 {
            background: rgba(15, 23, 42, 0.22) !important;
        }

        body.theme-night #greeting-title {
            color: #f8fafc;
        }

        body.theme-night #greeting-accent {
            color: #c7d2fe;
        }

        /* Light Glass Bento Card */
        .bento-card {
            background: rgba(255, 255, 255, 0.65);
            backdrop-filter: blur(24px);
            -webkit-backdrop-filter: blur(24px);
            border: 1px solid rgba(255, 255, 255, 0.6);
            box-shadow: 0 10px 40px -10px rgba(0, 0, 0, 0.05), 
                        0 0 20px rgba(255, 255, 255, 0.5) inset;
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
        }
        .bento-card:hover {
            background: rgba(255, 255, 255, 0.85);
            transform: translateY(-3px);
            box-shadow: 0 20px 40px -10px rgba(99, 102, 241, 0.15);
            border-color: #fff;
        }

        /* Mic-like Button Effect (reusable) */
        .mic-like {
            will-change: transform;
            transition: transform 0.18s cubic-bezier(0.4, 0, 0.2, 1),
                        box-shadow 0.18s cubic-bezier(0.4, 0, 0.2, 1),
                        filter 0.18s cubic-bezier(0.4, 0, 0.2, 1);
        }
        .mic-like:hover { transform: scale(1.05); }
        .mic-like:active { transform: scale(0.95); }
        .mic-like:focus-visible {
            outline: none;
            box-shadow: 0 0 0 4px rgba(199, 210, 254, 0.75);
        }
        /* Ensure bento-card buttons use mic-like transform instead of translateY */
        .bento-card.mic-like:hover { transform: scale(1.05); }
        .bento-card.mic-like:active { transform: scale(0.95); }

        .sensor-alert-transition {
            transition: background-color 0.7s ease,
                        border-color 0.7s ease,
                        color 0.7s ease,
                        box-shadow 0.7s ease;
        }

        /* 7-color rainbow gradient helpers (Tailwind palette hexes) */
        .rainbow-7 {
            background-image: linear-gradient(90deg,
                #f43f5e 0%,
                #fb923c 16%,
                #fbbf24 32%,
                #22c55e 48%,
                #06b6d4 64%,
                #3b82f6 80%,
                #8b5cf6 100%
            );
        }
        .rainbow-7-text {
            background-image: inherit;
            -webkit-background-clip: text;
            background-clip: text;
            color: transparent;
        }
        .swatch-rainbow {
            background-image: conic-gradient(
                #f43f5e,
                #fb923c,
                #fbbf24,
                #22c55e,
                #06b6d4,
                #3b82f6,
                #8b5cf6,
                #f43f5e
            );
        }

        /* AI Glow Effect */
        .miki-glow-active {
            border-color: #6366f1 !important;
            box-shadow: 0 0 30px rgba(99, 102, 241, 0.4) !important;
            background: rgba(255, 255, 255, 0.95) !important;
        }

        /* Scrollbar */
        ::-webkit-scrollbar { width: 6px; }
        ::-webkit-scrollbar-track { background: transparent; }
        ::-webkit-scrollbar-thumb { background: #cbd5e1; border-radius: 10px; }
        ::-webkit-scrollbar-thumb:hover { background: #94a3b8; }

        /* Visualizer */
        .wave-bar { width: 4px; background: #8b5cf6; border-radius: 2px; animation: wave 1s ease-in-out infinite; }
        .wave-bar:nth-child(2) { animation-delay: 0.1s; }
        .wave-bar:nth-child(3) { animation-delay: 0.2s; }
        .wave-bar:nth-child(4) { animation-delay: 0.3s; }
        .paused { animation-play-state: paused; height: 20%; background: #cbd5e1; }
    </style>
</head>
<body class="theme-afternoon min-h-screen md:h-screen flex flex-col md:flex-row antialiased selection:bg-indigo-100 selection:text-indigo-700">

    <div class="bg-gradient-mesh"></div>
    <div class="bg-night-overlay"></div>
    <div class="blob-bg top-[-10%] left-[-10%] opacity-70"></div>
    <div class="blob-bg blob-accent bottom-[-10%] right-[-10%]" style="animation-delay: 5s;"></div>

    <div class="fixed top-6 right-6 z-[210]">
        <button id="voice-fab" onclick="toggleVoicePanel()" class="bento-card px-5 py-3 rounded-full bg-white/60 border border-white text-slate-800 font-bold hover:bg-white transition-all flex items-center gap-3 shadow-lg shadow-indigo-500/10">
            <i class="ph-fill ph-microphone text-indigo-600 text-xl"></i>
            <span class="hidden sm:inline">Giao tiếp giọng nói</span>
        </button>

        <div id="voice-panel" class="hidden mt-3 bento-card rounded-[28px] p-5 bg-white/65 border border-white w-[min(420px,calc(100vw-3rem))] shadow-2xl">
            <div class="flex items-center justify-between gap-3">
                <div class="font-extrabold text-slate-800">Miki</div>
                <button onclick="toggleVoicePanel(false)" class="text-slate-400 hover:text-slate-600"><i class="ph-bold ph-x"></i></button>
            </div>

            <div class="mt-3 flex gap-2">
                <button id="voice-tab" onclick="setVoicePanelMode('voice')" class="flex-1 px-4 py-2.5 rounded-2xl bg-indigo-100 text-indigo-700 font-bold">Giọng nói</button>
                <button id="chat-tab" onclick="setVoicePanelMode('chat')" class="flex-1 px-4 py-2.5 rounded-2xl bg-white/60 border border-white text-slate-700 font-bold hover:bg-white transition-all">Chat</button>
            </div>

            <div class="mt-4">
                <div id="voice-mode" class="">
                    <div class="flex items-center justify-between gap-3">
                        <div class="min-w-0">
                            <div class="font-bold text-slate-800">Điều khiển giọng nói</div>
                            <div class="text-xs text-slate-500">Bấm mic và nói tiếng Việt</div>
                        </div>
                        <button onclick="toggleVoice()" id="mic-btn" class="bg-white/80 backdrop-blur-xl border border-white px-4 py-2.5 rounded-full flex items-center gap-3 shadow-lg shadow-indigo-500/10 hover:scale-105 transition-all active:scale-95 group">
                            <div class="relative">
                                <div class="absolute inset-0 bg-indigo-500 blur-md opacity-20 rounded-full"></div>
                                <i class="ph-fill ph-microphone text-xl text-indigo-600 relative z-10"></i>
                            </div>
                            <span id="mic-label" class="font-bold text-xs tracking-wide text-slate-700 group-hover:text-indigo-600 transition-colors">MIC</span>
                        </button>
                    </div>
                </div>

                <div id="chat-mode" class="hidden">
                    <div class="flex flex-col sm:flex-row gap-3">
                        <input id="chat-input" type="text" class="flex-1 bg-white/70 border border-white rounded-2xl py-2.5 px-4 text-slate-700 focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100 focus:outline-none transition-all" placeholder="Nhập tin nhắn cho Miki...">
                        <button onclick="submitChat()" class="sm:w-auto w-full px-5 py-2.5 rounded-2xl bg-emerald-100 text-emerald-700 font-bold hover:bg-emerald-600 hover:text-white transition-all">Gửi</button>
                    </div>
                </div>

                <div id="ai-response-box" class="mt-4 hidden">
                    <div id="ai-card" class="bento-card p-5 rounded-[22px] border-l-4 border-l-indigo-500 bg-gradient-to-r from-white to-indigo-50/50 shadow-xl shadow-indigo-100 transition-all duration-300">
                        <div class="flex items-start gap-4">
                            <div class="w-12 h-12 rounded-2xl bg-white text-indigo-600 flex items-center justify-center flex-shrink-0 shadow-lg shadow-indigo-200 border border-indigo-50">
                                <i class="ph-fill ph-sparkle text-xl"></i>
                            </div>
                            <div class="flex-1 min-w-0">
                                <div class="flex items-center justify-between gap-3 mb-2">
                                    <div class="flex items-center gap-2">
                                        <span class="text-xs font-bold text-indigo-500 uppercase tracking-wider">Miki</span>
                                        <div class="flex gap-1 h-3 items-end">
                                            <div class="wave-bar h-full paused"></div>
                                            <div class="wave-bar h-2/3 paused"></div>
                                            <div class="wave-bar h-full paused"></div>
                                        </div>
                                    </div>
                                    <button onclick="toggleAiResponse(false)" class="text-slate-400 hover:text-slate-600"><i class="ph-bold ph-x"></i></button>
                                </div>
                                <div id="ai-text" class="text-sm md:text-base leading-relaxed text-slate-700 font-medium break-words max-h-40 overflow-auto pr-1">Thinking...</div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <!-- Toasts: desktop -> below Emergency Stop (sidebar); mobile -> fixed overlay -->
    <div id="toast-container-mobile" class="md:hidden fixed top-24 right-4 z-[200] flex flex-col gap-3 pointer-events-none"></div>

    <div id="settings-modal" class="fixed inset-0 z-[100] bg-slate-900/40 hidden flex items-center justify-center backdrop-blur-md transition-opacity duration-300">
        <div class="bg-white/90 backdrop-blur-xl p-8 rounded-[32px] w-full max-w-lg mx-4 shadow-2xl relative border border-white">
            <div class="flex items-center justify-between mb-6">
                <h2 class="text-2xl font-bold text-slate-800 flex items-center gap-3">
                    <i class="ph-duotone ph-gear-six text-indigo-500"></i> Settings
                </h2>
                <button onclick="toggleSettings()" class="text-slate-400 hover:text-slate-600"><i class="ph-bold ph-x text-xl"></i></button>
            </div>
            
            <div class="space-y-6">
                <div class="group">
                    <label class="text-xs font-bold text-slate-500 uppercase tracking-wider mb-2 block">IP ESP32 Gateway</label>
                    <input type="text" id="esp-ip-input" class="w-full bg-slate-50 border border-slate-200 rounded-xl py-3 px-5 text-slate-700 focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100 focus:outline-none transition-all font-medium" placeholder="192.168.1.100  (or 127.0.0.1:8081 USB)">
                </div>

                <div class="group">
                    <label class="text-xs font-bold text-slate-500 uppercase tracking-wider mb-2 block">Google Apps Script WebApp URL (RFID)</label>
                    <input type="text" id="gscript-url-input" class="w-full bg-slate-50 border border-slate-200 rounded-xl py-3 px-5 text-slate-700 focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100 focus:outline-none transition-all font-medium" placeholder="https://script.google.com/macros/s/&lt;GSCRIPT_ID&gt;/exec">
                </div>
                
                <div class="group">
                    <label class="text-xs font-bold text-slate-500 uppercase tracking-wider mb-2 block">Gemini API Key</label>
                    <input type="password" id="gemini-key-input" class="w-full bg-slate-50 border border-slate-200 rounded-xl py-3 px-5 text-slate-700 focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100 focus:outline-none transition-all" placeholder="AIzaSy...">
                </div>

                <div class="group">
                    <label class="text-xs font-bold text-slate-500 uppercase tracking-wider mb-2 block">GCP TTS API Key (Optional)</label>
                    <input type="password" id="gcp-tts-key-input" class="w-full bg-slate-50 border border-slate-200 rounded-xl py-3 px-5 text-slate-700 focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100 focus:outline-none transition-all" placeholder="AIzaSy...">
                </div>

                
                
                <div class="pt-2">
                    <button onclick="saveSettings()" class="w-full bg-indigo-600 hover:bg-indigo-700 text-white font-bold py-3.5 rounded-2xl shadow-lg shadow-indigo-500/30 transition-all active:scale-95">Save Configuration</button>
                </div>
            </div>
        </div>
    </div>

    <aside class="hidden md:flex flex-col w-24 lg:w-72 m-4 bento-card rounded-[32px] justify-between py-8 px-4 relative z-10">
        <div class="space-y-3">
            <div class="flex items-center gap-4 px-3 mb-2">
                <div class="w-12 h-12 rounded-2xl bg-indigo-600 text-white flex items-center justify-center shadow-lg shadow-indigo-500/20">
                    <i class="ph-fill ph-house-line text-2xl"></i>
                </div>
                <div class="hidden lg:block">
                    <h1 class="font-bold text-2xl tracking-tight text-slate-800">Miki</h1>
                    <p class="text-xs text-slate-500 font-medium tracking-wide">SMART HOME</p>
                </div>
            </div>

            <button onclick="sendCommand('OFF')" class="w-full bento-card p-4 rounded-2xl flex items-center gap-3 border-rose-100 bg-rose-50/50 hover:bg-rose-500 hover:border-rose-500 group transition-all duration-300">
                <div class="w-10 h-10 rounded-2xl bg-rose-100 flex items-center justify-center group-hover:bg-white/20 transition-colors">
                    <i class="ph-fill ph-warning-circle text-2xl text-rose-500 group-hover:text-white"></i>
                </div>
                <div class="min-w-0 text-left">
                    <div class="text-xs font-bold text-rose-500 group-hover:text-white tracking-widest uppercase">Emergency Stop</div>
                    <div class="text-xs text-slate-500 group-hover:text-white/80 truncate">Tắt nhanh thiết bị</div>
                </div>
            </button>

            <div id="toast-container-desktop" class="hidden md:flex flex-col gap-3 pointer-events-none"></div>
        </div>

        <div class="space-y-3">
            <nav class="space-y-2">
                <a href="#" class="flex items-center gap-4 px-4 py-4 bg-indigo-50 text-indigo-600 rounded-2xl border border-indigo-100/50 shadow-sm">
                    <i class="ph-duotone ph-squares-four text-2xl"></i>
                    <span class="font-bold hidden lg:block">Dashboard</span>
                </a>
                <button onclick="toggleSettings()" class="w-full flex items-center gap-4 px-4 py-4 text-slate-500 hover:text-indigo-600 hover:bg-white rounded-2xl transition-all group">
                    <i class="ph-duotone ph-sliders text-2xl group-hover:scale-110 transition-transform"></i>
                    <span class="font-medium hidden lg:block">Settings</span>
                </button>
            </nav>

            <div class="p-5 bento-card rounded-2xl bg-white/40 border-0">
                <div class="flex items-center justify-between mb-2">
                    <span class="text-xs font-bold text-slate-400 uppercase">System</span>
                    <span id="status-dot" class="w-2.5 h-2.5 rounded-full bg-slate-300 ring-4 ring-slate-100"></span>
                </div>
                <p id="connection-msg" class="text-sm font-bold text-slate-600 truncate">Connecting...</p>
            </div>
        </div>
    </aside>

    <main class="flex-1 min-w-0 p-4 pb-10 md:p-8 md:pb-12 md:overflow-y-auto relative z-0">
        
        <header class="mb-10">
            <div class="md:hidden flex items-center gap-3 mb-6">
                <div class="w-10 h-10 rounded-xl bg-indigo-600 flex items-center justify-center text-white shadow-md"><i class="ph-bold ph-house"></i></div>
                <span class="font-bold text-xl text-slate-800">Miki Hub</span>
            </div>

            <div class="flex items-start justify-between gap-4">
                <div class="min-w-0">
                    <div class="flex items-center gap-3">
                        <div id="greeting-icon-wrap" class="w-12 h-12 rounded-2xl bg-white/70 border border-white flex items-center justify-center shadow-sm">
                            <i id="greeting-icon" class="ph-duotone ph-sun text-2xl text-amber-500"></i>
                        </div>
                        <h2 id="greeting-title" class="text-2xl md:text-4xl font-extrabold text-slate-800 tracking-tight leading-tight">
                            <span id="greeting-time">Good Morning</span>, <span id="greeting-accent" class="text-indigo-600">Nhóm 2 - 74DCTT23</span>
                        </h2>
                    </div>

                    <div class="mt-4 flex flex-wrap items-stretch gap-3">
                        <div class="inline-flex flex-col bento-card clock-glass px-5 py-3 rounded-2xl border border-white">
                            <div class="text-2xl font-bold tracking-tight text-slate-800" id="clock-time">00:00</div>
                            <div class="text-xs font-bold text-indigo-400 uppercase tracking-widest" id="clock-date">--/--/--</div>
                        </div>

                        <div id="top-sensors" class="flex flex-wrap items-stretch gap-3">
                            <div class="bento-card px-4 py-3 rounded-2xl flex items-center gap-3">
                                <div class="w-10 h-10 rounded-2xl bg-orange-50 text-orange-500 flex items-center justify-center border border-orange-100">
                                    <i class="ph-duotone ph-thermometer text-xl"></i>
                                </div>
                                <div>
                                    <div class="text-[11px] font-bold text-slate-400 uppercase tracking-wider">Temp</div>
                                    <div class="text-lg font-extrabold text-slate-800" id="val-temp">--°C</div>
                                </div>
                            </div>
                            <div class="bento-card px-4 py-3 rounded-2xl flex items-center gap-3">
                                <div class="w-10 h-10 rounded-2xl bg-blue-50 text-blue-500 flex items-center justify-center border border-blue-100">
                                    <i class="ph-duotone ph-drop text-xl"></i>
                                </div>
                                <div>
                                    <div class="text-[11px] font-bold text-slate-400 uppercase tracking-wider">Humid</div>
                                    <div class="text-lg font-extrabold text-slate-800" id="val-hum">--%</div>
                                </div>
                            </div>
                            <div id="card-gas" class="bento-card px-4 py-3 rounded-2xl flex items-center gap-3 sensor-alert-transition">
                                <div id="icon-gas" class="w-10 h-10 rounded-2xl bg-emerald-50 text-emerald-500 flex items-center justify-center border border-emerald-100 sensor-alert-transition">
                                    <i class="ph-duotone ph-wind text-xl"></i>
                                </div>
                                <div>
                                    <div class="text-[11px] font-bold text-slate-400 uppercase tracking-wider">Air</div>
                                    <div class="text-lg font-extrabold text-slate-800" id="val-gas">--</div>
                                </div>
                            </div>
                            <div id="card-fire" class="bento-card px-4 py-3 rounded-2xl flex items-center gap-3 sensor-alert-transition">
                                <div id="icon-fire" class="w-10 h-10 rounded-2xl bg-rose-50 text-rose-500 flex items-center justify-center border border-rose-100 sensor-alert-transition">
                                    <i class="ph-duotone ph-fire text-xl"></i>
                                </div>
                                <div>
                                    <div class="text-[11px] font-bold text-slate-400 uppercase tracking-wider">Status</div>
                                    <div class="text-lg font-extrabold text-slate-800" id="val-fire">Safe</div>
                                </div>
                            </div>

                            <button type="button" class="bento-card px-4 py-3 rounded-2xl flex items-center gap-3 text-left">
                                <div class="w-10 h-10 rounded-2xl bg-indigo-50 text-indigo-500 flex items-center justify-center border border-indigo-100">
                                    <i class="ph-duotone ph-waveform text-xl"></i>
                                </div>
                                <div>
                                    <div class="text-[11px] font-bold text-slate-400 uppercase tracking-wider">Ultrasonic</div>
                                    <div class="text-lg font-extrabold text-slate-800" id="val-ultrasonic">-- cm</div>
                                </div>
                            </button>

                            <button type="button" class="bento-card px-4 py-3 rounded-2xl flex items-center gap-3 text-left">
                                <div class="w-10 h-10 rounded-2xl bg-amber-50 text-amber-500 flex items-center justify-center border border-amber-100">
                                    <i class="ph-duotone ph-sun-dim text-xl"></i>
                                </div>
                                <div>
                                    <div class="text-[11px] font-bold text-slate-400 uppercase tracking-wider">LDR</div>
                                    <div class="text-lg font-extrabold text-slate-800" id="val-ldr">--</div>
                                </div>
                            </button>
                        </div>
                    </div>
                </div>

                <div class="flex items-start justify-end">
                    <!-- Voice panel is fixed in the top-right corner -->
                </div>
            </div>
        </header>

        <h3 class="text-lg font-bold text-slate-700 mb-5 flex items-center gap-2">
            <i class="ph-duotone ph-faders text-indigo-500"></i> Manual Controls
        </h3>

        <div class="space-y-6">
            <div class="grid grid-cols-1 lg:grid-cols-2 xl:grid-cols-3 gap-6">

            <div id="card-light1" class="bento-card p-6 rounded-[28px] flex flex-col justify-between">
                <div class="flex justify-between items-start">
                    <div class="w-14 h-14 rounded-2xl bg-amber-50 text-amber-500 border border-amber-100 flex items-center justify-center">
                        <i class="ph-duotone ph-lightbulb text-3xl"></i>
                    </div>
                    <div class="text-right">
                        <div class="flex items-center justify-end gap-2">
                            <h4 class="font-bold text-lg text-slate-800">Light 1</h4>
                            <span id="st-light1" class="w-2.5 h-2.5 rounded-full bg-slate-300 ring-4 ring-slate-100"></span>
                        </div>
                        <p class="text-xs text-slate-500">Floor 1</p>
                    </div>
                </div>
                <div class="mt-5 bg-white/60 border border-white rounded-2xl p-4 min-h-[168px]">
                    <div class="flex flex-wrap sm:flex-nowrap items-end justify-between gap-4">
                        <div class="w-40 flex-shrink-0 flex flex-col gap-2">
                            <button id="btn-light1-on" onclick="sendCommand('LED1_ON')" class="w-full px-5 py-2.5 rounded-xl bg-amber-100 text-amber-600 font-bold hover:bg-amber-500 hover:text-white transition-all shadow-sm">ON</button>
                            <button id="btn-light1-off" onclick="sendCommand('LED1_OFF')" class="w-full px-5 py-2.5 rounded-xl bg-slate-100 text-slate-500 font-bold hover:bg-slate-200 transition-all shadow-sm">OFF</button>
                        </div>
                        <div class="flex flex-col items-end gap-2">
                            <div class="flex items-center gap-2 text-sm text-slate-500">
                                <span id="sw-light1" class="w-3 h-3 rounded-full bg-amber-400 ring-4 ring-amber-100"></span>
                                <span class="font-bold">Màu</span>
                            </div>
                            <div class="grid grid-cols-3 gap-3">
                                <button onclick="setLightColor('LIGHT1','YELLOW')" class="w-9 h-9 rounded-full bg-amber-400 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT1','RED')" class="w-9 h-9 rounded-full bg-rose-500 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT1','GREEN')" class="w-9 h-9 rounded-full bg-emerald-500 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT1','BLUE')" class="w-9 h-9 rounded-full bg-indigo-500 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT1','PURPLE')" class="w-9 h-9 rounded-full bg-violet-500 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT1','WHITE')" class="w-9 h-9 rounded-full bg-white border border-slate-200 shadow-sm"></button>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <div id="card-light2" class="bento-card p-6 rounded-[28px] flex flex-col justify-between">
                <div class="flex justify-between items-start">
                    <div class="w-14 h-14 rounded-2xl bg-indigo-50 text-indigo-500 border border-indigo-100 flex items-center justify-center">
                        <i class="ph-duotone ph-lightbulb text-3xl"></i>
                    </div>
                    <div class="text-right">
                        <div class="flex items-center justify-end gap-2">
                            <h4 class="font-bold text-lg text-slate-800">Light 2</h4>
                            <span id="st-light2" class="w-2.5 h-2.5 rounded-full bg-slate-300 ring-4 ring-slate-100"></span>
                        </div>
                        <p class="text-xs text-slate-500">Floor 2</p>
                    </div>
                </div>
                <div class="mt-5 bg-white/60 border border-white rounded-2xl p-4 min-h-[168px]">
                    <div class="flex flex-wrap sm:flex-nowrap items-end justify-between gap-4">
                        <div class="w-40 flex-shrink-0 flex flex-col gap-2">
                            <button id="btn-light2-on" onclick="sendCommand('LIGHT2_ON')" class="w-full px-5 py-2.5 rounded-xl bg-indigo-100 text-indigo-600 font-bold hover:bg-indigo-600 hover:text-white transition-all shadow-sm">ON</button>
                            <button id="btn-light2-off" onclick="sendCommand('LIGHT2_OFF')" class="w-full px-5 py-2.5 rounded-xl bg-slate-100 text-slate-500 font-bold hover:bg-slate-200 transition-all shadow-sm">OFF</button>
                        </div>
                        <div class="flex flex-col items-end gap-2">
                            <div class="flex items-center gap-2 text-sm text-slate-500">
                                <span id="sw-light2" class="w-3 h-3 rounded-full bg-amber-400 ring-4 ring-amber-100"></span>
                                <span class="font-bold">Màu</span>
                            </div>
                            <div class="grid grid-cols-3 gap-3">
                                <button onclick="setLightColor('LIGHT2','YELLOW')" class="w-9 h-9 rounded-full bg-amber-400 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT2','RED')" class="w-9 h-9 rounded-full bg-rose-500 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT2','GREEN')" class="w-9 h-9 rounded-full bg-emerald-500 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT2','BLUE')" class="w-9 h-9 rounded-full bg-indigo-500 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT2','PURPLE')" class="w-9 h-9 rounded-full bg-violet-500 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT2','WHITE')" class="w-9 h-9 rounded-full bg-white border border-slate-200 shadow-sm"></button>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <div id="card-light3" class="bento-card p-6 rounded-[28px] flex flex-col justify-between">
                <div class="flex justify-between items-start">
                    <div class="w-14 h-14 rounded-2xl bg-cyan-50 text-cyan-500 border border-cyan-100 flex items-center justify-center">
                        <i class="ph-duotone ph-lightbulb text-3xl"></i>
                    </div>
                    <div class="text-right">
                        <div class="flex items-center justify-end gap-2">
                            <h4 class="font-bold text-lg text-slate-800">Light 3</h4>
                            <span id="st-light3" class="w-2.5 h-2.5 rounded-full bg-slate-300 ring-4 ring-slate-100"></span>
                        </div>
                        <p class="text-xs text-slate-500">Floor 3</p>
                    </div>
                </div>
                <div class="mt-5 bg-white/60 border border-white rounded-2xl p-4 min-h-[168px]">
                    <div class="flex flex-wrap sm:flex-nowrap items-end justify-between gap-4">
                        <div class="w-40 flex-shrink-0 flex flex-col gap-2">
                            <button id="btn-light3-on" onclick="sendCommand('LIGHT3_ON')" class="w-full px-5 py-2.5 rounded-xl bg-cyan-100 text-cyan-700 font-bold hover:bg-cyan-600 hover:text-white transition-all shadow-sm">ON</button>
                            <button id="btn-light3-off" onclick="sendCommand('LIGHT3_OFF')" class="w-full px-5 py-2.5 rounded-xl bg-slate-100 text-slate-500 font-bold hover:bg-slate-200 transition-all shadow-sm">OFF</button>
                        </div>
                        <div class="flex flex-col items-end gap-2">
                            <div class="flex items-center gap-2 text-sm text-slate-500">
                                <span id="sw-light3" class="w-3 h-3 rounded-full bg-amber-400 ring-4 ring-amber-100"></span>
                                <span class="font-bold">Màu</span>
                            </div>
                            <div class="grid grid-cols-3 gap-3">
                                <button onclick="setLightColor('LIGHT3','YELLOW')" class="w-9 h-9 rounded-full bg-amber-400 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT3','RED')" class="w-9 h-9 rounded-full bg-rose-500 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT3','GREEN')" class="w-9 h-9 rounded-full bg-emerald-500 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT3','BLUE')" class="w-9 h-9 rounded-full bg-indigo-500 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT3','PURPLE')" class="w-9 h-9 rounded-full bg-violet-500 border border-white shadow-sm"></button>
                                <button onclick="setLightColor('LIGHT3','WHITE')" class="w-9 h-9 rounded-full bg-white border border-slate-200 shadow-sm"></button>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <div id="card-lights-all" class="bento-card p-6 rounded-[28px] flex flex-col justify-between">
                <div class="flex justify-between items-start">
                    <div class="w-14 h-14 rounded-2xl bg-amber-50 text-amber-500 border border-amber-100 flex items-center justify-center">
                        <i class="ph-duotone ph-lamp text-3xl"></i>
                    </div>
                    <div class="text-right">
                        <div class="flex items-center justify-end gap-2">
                            <h4 class="font-bold text-lg text-slate-800">Đèn 3 tầng</h4>
                            <div class="flex items-center gap-1">
                                <span id="st-light1-mini" class="w-2.5 h-2.5 rounded-full bg-slate-300 ring-4 ring-slate-100"></span>
                                <span id="st-light2-mini" class="w-2.5 h-2.5 rounded-full bg-slate-300 ring-4 ring-slate-100"></span>
                                <span id="st-light3-mini" class="w-2.5 h-2.5 rounded-full bg-slate-300 ring-4 ring-slate-100"></span>
                            </div>
                        </div>
                        <p class="text-xs text-slate-500">Bật/tắt & đổi màu đồng bộ</p>
                    </div>
                </div>
                <div class="mt-5 bg-white/60 border border-white rounded-2xl p-4 min-h-[168px]">
                    <div class="flex flex-wrap sm:flex-nowrap items-end justify-between gap-4">
                        <div class="w-40 flex-shrink-0 flex flex-col gap-2">
                            <button id="btn-lightsall-on" onclick="sendCommand('LIGHTS_ALL_ON')" class="w-full h-11 px-5 py-2.5 rounded-xl bg-amber-100 text-amber-700 font-bold hover:bg-amber-500 hover:text-white transition-all shadow-sm">ON</button>
                            <button id="btn-lightsall-off" onclick="sendCommand('LIGHTS_ALL_OFF')" class="w-full h-11 px-5 py-2.5 rounded-xl bg-slate-100 text-slate-600 font-bold hover:bg-slate-200 transition-all shadow-sm">OFF</button>
                        </div>
                        <div class="flex flex-col items-end gap-2">
                            <div class="flex items-center gap-2 text-sm text-slate-500">
                                <span id="sw-lights-all" class="w-3 h-3 rounded-full bg-amber-400 ring-4 ring-amber-100"></span>
                                <span class="font-bold">Màu</span>
                            </div>
                            <div class="flex flex-col items-end gap-3">
                                <div class="grid grid-cols-3 gap-3">
                                    <button onclick="setAllLightsColor('YELLOW')" class="w-9 h-9 rounded-full bg-amber-400 border border-white shadow-sm"></button>
                                    <button onclick="setAllLightsColor('RED')" class="w-9 h-9 rounded-full bg-rose-500 border border-white shadow-sm"></button>
                                    <button onclick="setAllLightsColor('GREEN')" class="w-9 h-9 rounded-full bg-emerald-500 border border-white shadow-sm"></button>
                                </div>
                                <div class="grid grid-cols-4 gap-3">
                                    <button onclick="setAllLightsColor('BLUE')" class="w-9 h-9 rounded-full bg-indigo-500 border border-white shadow-sm"></button>
                                    <button onclick="setAllLightsColor('PURPLE')" class="w-9 h-9 rounded-full bg-violet-500 border border-white shadow-sm"></button>
                                    <button onclick="setAllLightsColor('WHITE')" class="w-9 h-9 rounded-full bg-white border border-slate-200 shadow-sm"></button>
                                    <button onclick="setAllLightsColor('RAINBOW')" class="w-9 h-9 rounded-full bg-white border border-slate-200 shadow-sm flex items-center justify-center" aria-label="Nhiều màu">
                                        <span class="w-5 h-5 rounded-full swatch-rainbow ring-2 ring-white/70"></span>
                                    </button>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <div id="card-curtain" class="bento-card p-6 rounded-[28px] flex flex-col gap-5">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-4">
                        <div class="w-14 h-14 rounded-2xl bg-indigo-50 text-indigo-600 border border-indigo-100 flex items-center justify-center">
                            <i id="icon-curtain" class="ph-duotone ph-columns text-3xl"></i>
                        </div>
                        <div>
                            <h4 class="font-bold text-lg text-slate-800">Rèm cửa</h4>
                            <p class="text-xs text-slate-500">Mở / Dừng / Đóng</p>
                        </div>
                    </div>
                    <span id="st-curtain" class="w-2.5 h-2.5 rounded-full bg-slate-300 ring-4 ring-slate-100"></span>
                </div>

                <div class="grid grid-cols-3 gap-2">
                    <button id="btn-curtain-open" onclick="sendCommand('CURTAIN_OPEN')" class="py-2.5 rounded-xl bg-indigo-100 text-indigo-700 font-bold hover:bg-indigo-600 hover:text-white transition-all">MỞ</button>
                    <button id="btn-curtain-stop" onclick="sendCommand('CURTAIN_STOP')" class="py-2.5 rounded-xl bg-slate-100 text-slate-700 font-bold hover:bg-slate-200 transition-all">DỪNG</button>
                    <button id="btn-curtain-close" onclick="sendCommand('CURTAIN_CLOSE')" class="py-2.5 rounded-xl bg-slate-100 text-slate-700 font-bold hover:bg-slate-200 transition-all">ĐÓNG</button>
                </div>

                <div class="bg-white/60 border border-white rounded-2xl p-4">
                    <div class="text-xs font-bold text-slate-400 uppercase mb-1.5">Trạng thái</div>
                    <div id="curtain-status-text" class="text-sm font-bold text-slate-700 truncate">Chưa có dữ liệu</div>
                </div>
            </div>

            <div id="card-fans" class="bento-card p-6 rounded-[28px] flex flex-col gap-5">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-4">
                        <div class="w-14 h-14 rounded-2xl bg-cyan-50 text-cyan-500 border border-cyan-100 flex items-center justify-center">
                            <i class="ph-duotone ph-fan text-3xl"></i>
                        </div>
                        <div>
                            <h4 class="font-bold text-lg text-slate-800">Quạt</h4>
                            <p class="text-xs text-slate-500">Quạt 1 / Quạt 2</p>
                        </div>
                    </div>
                </div>

                <div class="grid grid-cols-1 sm:grid-cols-2 gap-4">
                    <div class="bg-white/60 border border-white rounded-2xl p-4 min-h-[168px]">
                        <div class="flex items-center justify-between mb-3">
                            <div class="flex items-center gap-2">
                                <i id="icon-fan2" class="ph-duotone ph-fan text-2xl text-sky-500"></i>
                                <span class="font-bold text-slate-700">Quạt 1</span>
                            </div>
                            <span id="st-fan2" class="w-2.5 h-2.5 rounded-full bg-slate-300 ring-4 ring-slate-100"></span>
                        </div>
                        <div class="flex flex-col gap-2">
                            <button id="btn-fan2-on" onclick="sendCommand('FAN2_ON')" class="w-full px-5 py-2.5 rounded-xl bg-emerald-100 text-emerald-700 font-bold hover:bg-emerald-600 hover:text-white transition-all">ON</button>
                            <button id="btn-fan2-off" onclick="sendCommand('FAN2_OFF')" class="w-full px-5 py-2.5 rounded-xl bg-slate-100 text-slate-600 font-bold hover:bg-slate-200 transition-all">OFF</button>
                        </div>
                    </div>

                    <div class="bg-white/60 border border-white rounded-2xl p-4 min-h-[168px]">
                        <div class="flex items-center justify-between mb-3">
                            <div class="flex items-center gap-2">
                                <i id="icon-fan" class="ph-duotone ph-fan text-2xl text-cyan-500"></i>
                                <span class="font-bold text-slate-700">Quạt 2</span>
                            </div>
                            <span id="st-fan1" class="w-2.5 h-2.5 rounded-full bg-slate-300 ring-4 ring-slate-100"></span>
                        </div>
                        <div class="flex flex-col gap-2">
                            <button id="btn-fan1-on" onclick="sendCommand('FAN1_ON')" class="w-full px-5 py-2.5 rounded-xl bg-emerald-100 text-emerald-700 font-bold hover:bg-emerald-600 hover:text-white transition-all">ON</button>
                            <button id="btn-fan1-off" onclick="sendCommand('FAN1_OFF')" class="w-full px-5 py-2.5 rounded-xl bg-slate-100 text-slate-600 font-bold hover:bg-slate-200 transition-all">OFF</button>
                        </div>
                    </div>
                </div>
            </div>

            </div>

            <div id="door-rfid-card" class="bento-card p-6 rounded-[28px]">
                <div class="grid grid-cols-1 lg:grid-cols-12 gap-4">
                    <div class="bg-white/60 border border-white rounded-2xl p-5 lg:col-span-4 flex flex-col">
                        <div class="flex items-center gap-4 mb-4">
                            <div class="w-14 h-14 rounded-2xl bg-amber-50 text-amber-500 border border-amber-100 flex items-center justify-center">
                                <i class="ph-duotone ph-door text-3xl"></i>
                            </div>
                            <div class="flex-1 min-w-0">
                                <div class="flex items-center gap-2">
                                    <h4 class="font-bold text-lg text-slate-800">Cửa</h4>
                                    <span id="st-door" class="w-2.5 h-2.5 rounded-full bg-slate-300 ring-4 ring-slate-100"></span>
                                </div>
                                <p id="door-status-text" class="text-xs text-slate-500 truncate">Chưa có dữ liệu</p>
                            </div>
                        </div>

                        <div class="grid grid-cols-2 gap-2">
                            <button id="btn-door-open" onclick="sendCommand('DOOR_OPEN')" class="py-2.5 rounded-xl bg-emerald-100 text-emerald-700 font-bold hover:bg-emerald-600 hover:text-white transition-all">MỞ</button>
                            <button id="btn-door-close" onclick="sendCommand('DOOR_CLOSE')" class="py-2.5 rounded-xl bg-slate-100 text-slate-700 font-bold hover:bg-slate-200 transition-all">ĐÓNG</button>
                        </div>

                        <div class="mt-4 border-t border-slate-100 pt-4 space-y-4">
                            <div>
                                <div class="text-xs font-bold text-slate-400 uppercase mb-2">Mở bằng mật khẩu</div>
                                <div class="flex gap-2">
                                    <input id="door-pin-input" type="password" inputmode="numeric" autocomplete="one-time-code" class="flex-1 bg-slate-50 border border-slate-200 rounded-xl py-2.5 px-4 text-slate-700 focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100 focus:outline-none transition-all" placeholder="Nhập mật khẩu">
                                    <button onclick="openDoorWithPin()" class="px-4 py-2.5 rounded-xl bg-indigo-100 text-indigo-700 font-bold hover:bg-indigo-600 hover:text-white transition-all">MỞ</button>
                                </div>
                            </div>

                            <div>
                                <div class="text-xs font-bold text-slate-400 uppercase mb-2">Thêm mật khẩu</div>
                                <div class="space-y-2">
                                    <div class="flex gap-2">
                                        <input id="door-pin-set-input" type="password" inputmode="numeric" autocomplete="new-password" class="flex-1 bg-slate-50 border border-slate-200 rounded-xl py-2.5 px-4 text-slate-700 focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100 focus:outline-none transition-all" placeholder="Mật khẩu mới">
                                        <button onclick="setDoorPin()" class="px-4 py-2.5 rounded-xl bg-emerald-100 text-emerald-700 font-bold hover:bg-emerald-600 hover:text-white transition-all">Lưu</button>
                                    </div>

                                    <div class="flex gap-2">
                                        <input id="door-pin-emergency-input" type="password" inputmode="numeric" autocomplete="new-password" class="flex-1 bg-slate-50 border border-slate-200 rounded-xl py-2.5 px-4 text-slate-700 focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100 focus:outline-none transition-all" placeholder="Mật khẩu khẩn cấp">
                                        <button onclick="setDoorEmergencyPin()" class="px-4 py-2.5 rounded-xl bg-rose-100 text-rose-700 font-bold hover:bg-rose-600 hover:text-white transition-all">Lưu</button>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>

                    <div class="bg-white/60 border border-white rounded-2xl p-5 lg:col-span-8">
                        <div class="flex items-center justify-between mb-1.5">
                            <div class="text-base font-extrabold text-slate-800">RFID</div>
                            <button id="btn-rfid-refresh" type="button" onclick="try{refreshRfid('manual')}catch(e){}" class="text-sm font-bold text-indigo-600 hover:text-indigo-700">Refresh</button>
                        </div>
                        <div class="text-sm text-slate-500 mb-3">
                            Vừa quét: <span id="rfid-last-uid" class="font-bold text-slate-700">--</span>
                            · <span id="rfid-last-time">--</span>
                            · <span id="rfid-last-auth" class="font-bold text-slate-700">--</span>
                        </div>

                        <div class="space-y-2 mb-3">
                            <input id="rfid-uid-input" type="text" class="w-full bg-slate-50 border border-slate-200 rounded-xl py-2.5 px-4 text-slate-700 focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100 focus:outline-none transition-all" placeholder="UID thẻ (ví dụ: 04A1B2C3)">
                            <input id="rfid-name-input" type="text" class="w-full bg-slate-50 border border-slate-200 rounded-xl py-2.5 px-4 text-slate-700 focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100 focus:outline-none transition-all" placeholder="Tên (bắt buộc)">
                            <div class="grid grid-cols-2 gap-2">
                                <button type="button" onclick="addRfidCard()" class="py-2.5 rounded-xl bg-indigo-100 text-indigo-700 font-bold hover:bg-indigo-600 hover:text-white transition-all">Thêm</button>
                                <button type="button" onclick="deleteRfidCard()" class="py-2.5 rounded-xl bg-rose-100 text-rose-700 font-bold hover:bg-rose-600 hover:text-white transition-all">Xoá</button>
                            </div>
                        </div>

                        <div class="grid grid-cols-1 sm:grid-cols-12 gap-3">
                            <div class="bg-white/60 border border-white rounded-2xl p-3 sm:col-span-4">
                                <div class="text-sm font-bold text-slate-400 uppercase mb-2">Thẻ đã lưu</div>
                                <div id="rfid-cards" class="text-xs text-slate-700 space-y-1 max-h-40 overflow-auto"></div>
                            </div>
                            <div class="bg-white/60 border border-white rounded-2xl p-4 sm:col-span-8">
                                <div class="text-sm font-bold text-slate-400 uppercase mb-2">Lịch sử quét</div>
                                <div id="rfid-history" class="text-sm text-slate-700 space-y-1 max-h-40 overflow-auto"></div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>

    </main>

    <script>
        // Cấu hình từ PHP
        const BASE_URL = "<?php echo BASE_URL; ?>";
        // ESP32 Gateway base URL (ưu tiên localStorage, fallback từ config)
        const ESP_DEFAULT_BASE_URL = "<?php echo ESP_BASE_URL; ?>";

        function normalizeBaseUrl(url) {
            let u = String(url || '').trim();
            if (!u) return '';
            if (!/^https?:\/\//i.test(u)) u = 'http://' + u;
            u = u.replace(/\/+$/, '');
            return u;
        }

        let ESP_IP = normalizeBaseUrl(localStorage.getItem('esp_ip') || ESP_DEFAULT_BASE_URL);

        // Đồng bộ Gemini API key
        async function syncGeminiApiKey(apiKey) {
            if (!apiKey) return;
            try {
                await fetch(`${BASE_URL}/AiController/saveApiKey`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ apiKey })
                });
            } catch (e) { console.warn('Sync Gemini Key error', e); }
        }

        // Đồng bộ TTS API key
        async function syncGcpTtsApiKey(apiKey) {
            if (!apiKey) return;
            try {
                await fetch(`${BASE_URL}/AiController/saveTtsKey`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ apiKey })
                });
            } catch (e) { console.warn('Sync TTS Key error', e); }
        }
        
        let vietnameseVoice = null;
        let recognition;
        let isListening = false;
        let recognitionTarget = 'voice';

        const VN_TZ = 'Asia/Ho_Chi_Minh';

        function showToast(message, type = 'success') {
            const isDesktop = (window.matchMedia && window.matchMedia('(min-width: 768px)').matches);
            const container = document.getElementById(isDesktop ? 'toast-container-desktop' : 'toast-container-mobile')
                || document.getElementById('toast-container-mobile')
                || document.getElementById('toast-container-desktop');
            if (!container) return;

            // Limit visible toasts
            const MAX_TOASTS = 2;
            while (container.childElementCount >= MAX_TOASTS) {
                const first = container.firstElementChild;
                if (!first) break;
                try { first.remove(); } catch (e) { break; }
            }

            const el = document.createElement('div');
            const isOk = String(type || 'success') === 'success';
            const bg = isOk ? 'bg-emerald-600' : 'bg-rose-600';
            el.className = `pointer-events-none ${bg} text-white px-5 py-3 rounded-2xl shadow-xl w-full ring-1 ring-white/15 backdrop-blur-xl transition-all duration-300 ease-out`;
            // Enter state
            el.classList.add('opacity-0', 'translate-y-2', 'scale-[0.98]');

            const row = document.createElement('div');
            row.className = 'flex items-start gap-3';

            const iconWrap = document.createElement('div');
            iconWrap.className = 'mt-0.5 w-8 h-8 rounded-xl bg-white/15 flex items-center justify-center flex-none';
            const icon = document.createElement('i');
            icon.className = 'ph-fill ph-bell text-xl text-white';
            iconWrap.appendChild(icon);

            const text = document.createElement('div');
            text.className = 'text-sm font-semibold leading-snug';
            text.innerText = String(message || '');

            row.appendChild(iconWrap);
            row.appendChild(text);
            el.appendChild(row);

            container.appendChild(el);

            // Animate in
            requestAnimationFrame(() => {
                el.classList.remove('opacity-0', 'translate-y-2', 'scale-[0.98]');
                el.classList.add('opacity-100', 'translate-y-0', 'scale-100');
            });

            // Animate out
            setTimeout(() => {
                el.classList.add('opacity-0', 'translate-y-2', 'scale-[0.98]');
                setTimeout(() => {
                    try { el.remove(); } catch (e) {}
                }, 260);
            }, 2900);
        }

        // --- ESP32 API client (fetch + timeout + retry) ---
        const ESP_API_TIMEOUT_MS = 2500;
        const ESP_API_RETRY = 1; // 0-1 lần

        function _joinUrl(base, path) {
            const b = String(base || '').replace(/\/+$/, '');
            const p = String(path || '');
            if (!b) return p;
            if (!p) return b;
            if (/^https?:\/\//i.test(p)) return p;
            return b + (p.startsWith('/') ? p : '/' + p);
        }

        async function apiFetch(path, options = {}) {
            const url = _joinUrl(ESP_IP, path);

            const method = (options.method || 'GET').toUpperCase();
            const attemptMax = 1 + (Number.isFinite(ESP_API_RETRY) ? ESP_API_RETRY : 0);

            for (let attempt = 1; attempt <= attemptMax; attempt++) {
                const controller = new AbortController();
                const timer = setTimeout(() => controller.abort(), ESP_API_TIMEOUT_MS);
                try {
                    const res = await fetch(url, {
                        mode: 'cors',
                        cache: 'no-store',
                        ...options,
                        method,
                        signal: controller.signal
                    });
                    return res;
                } catch (err) {
                    const isAbort = err && (err.name === 'AbortError');
                    const isNetwork = err instanceof TypeError || isAbort;

                    console.error('[ESP API] request failed', {
                        url,
                        method,
                        attempt,
                        attemptMax,
                        error: String(err && err.message ? err.message : err)
                    });

                    if (attempt >= attemptMax || !isNetwork) throw err;
                } finally {
                    clearTimeout(timer);
                }
            }
            throw new Error('apiFetch: unreachable');
        }

        async function apiReadJson(res) {
            const text = await res.text();
            if (!text) return null;
            try {
                return JSON.parse(text);
            } catch (e) {
                console.error('[ESP API] invalid JSON', { status: res.status, text: text.slice(0, 400) });
                throw new Error('Invalid JSON from ESP32');
            }
        }

        async function apiGetJson(path) {
            const res = await apiFetch(path, { method: 'GET' });
            const data = await apiReadJson(res);
            if (!res.ok) {
                const msg = (data && (data.error || data.message)) ? (data.error || data.message) : ('HTTP ' + res.status);
                throw new Error(msg);
            }
            return data;
        }

        async function apiPostJson(path, bodyObj) {
            const res = await apiFetch(path, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(bodyObj || {})
            });
            const data = await apiReadJson(res);
            if (!res.ok) {
                const msg = (data && (data.error || data.message)) ? (data.error || data.message) : ('HTTP ' + res.status);
                throw new Error(msg);
            }
            return data;
        }

        // --- Google Apps Script RFID CRUD client ---
        function normalizeUid(raw) {
            return String(raw || '').toUpperCase().replace(/\s+/g, '').trim();
        }

        function pickObjField(obj, keys) {
            if (!obj || typeof obj !== 'object') return '';
            for (const k of (keys || [])) {
                if (Object.prototype.hasOwnProperty.call(obj, k) && obj[k] != null) {
                    const v = String(obj[k]).trim();
                    if (v) return v;
                }
            }
            return '';
        }

        function pickRfidUid(card) {
            if (!card) return '';
            if (Array.isArray(card)) return (card.length ? String(card[0] ?? '') : '');
            if (typeof card === 'string' || typeof card === 'number') return String(card);
            if (typeof card !== 'object') return '';
            return pickObjField(card, [
                'uid', 'UID', 'Uid',
                'id', 'ID',
                'rfid', 'RFID',
                'card_uid', 'cardUid', 'cardID', 'cardId',
                'card', 'Card',
            ]);
        }

        function pickRfidName(card) {
            if (!card || typeof card !== 'object') return '';
            if (Array.isArray(card)) return (card.length >= 2 ? String(card[1] ?? '').trim() : '');
            return pickObjField(card, [
                'name', 'Name',
                'ten', 'Ten',
                'card_name', 'cardName',
                'owner', 'Owner',
                'holder', 'Holder',
            ]);
        }

        function isValidUid(uid) {
            return /^[0-9A-F]{4,20}$/.test(String(uid || ''));
        }

        function getGscriptUrl() {
            const fromStorage = String(localStorage.getItem('gscript_url') || '').trim();
            let u = fromStorage;

            // Fallback: if user typed in Settings but didn't press Save yet.
            if (!u) {
                const el = document.getElementById('gscript-url-input');
                const typed = el ? String(el.value || '').trim() : '';
                if (typed) {
                    u = typed;
                    try { localStorage.setItem('gscript_url', u); } catch (e) {}
                }
            }

            return u ? u.replace(/\/+$/, '') : '';
        }

        async function gscriptCall(payload) {
            const url = getGscriptUrl();
            if (!url) throw new Error('Chưa cấu hình Google Apps Script URL');

            // Use same-origin proxy to avoid browser CORS restrictions with Apps Script.
            const proxyUrl = `${BASE_URL}/RfidController/proxy`;
            const res = await fetch(proxyUrl, {
                method: 'POST',
                cache: 'no-store',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ gscript_url: url, payload: (payload || {}) })
            });

            const text = await res.text();
            let data;
            try {
                data = text ? JSON.parse(text) : null;
            } catch (e) {
                data = null;
            }

            if (!res.ok) {
                const msg = (data && data.message) ? String(data.message) : ('HTTP ' + res.status);
                throw new Error(msg);
            }
            if (!data || typeof data !== 'object') {
                throw new Error('Apps Script không trả JSON hợp lệ');
            }
            return data;
        }

        async function gscriptCallWithActionFallback(actionCandidates, basePayload) {
            const tries = Array.isArray(actionCandidates) ? actionCandidates.filter(Boolean) : [];
            const lastErr = { data: null };
            for (const action of tries.length ? tries : [String((basePayload || {}).action || '')]) {
                const payload = { ...(basePayload || {}), action };
                const data = await gscriptCall(payload);
                lastErr.data = data;
                if (data && data.ok === true) return data;

                // Only retry on the common deployment mismatch.
                const msg = (data && data.message) ? String(data.message) : '';
                if (/unknown\s+action/i.test(msg)) continue;
                return data;
            }
            return lastErr.data;
        }

        function getCurtainStatusTextFromSensors(sensorsJson) {
            const root = sensorsJson && typeof sensorsJson === 'object' ? sensorsJson : {};
            const devices = (root.devices && typeof root.devices === 'object') ? root.devices : {};

            const stRaw = (root.curtain_state != null) ? root.curtain_state : devices.curtain_state;
            const motionRaw = (root.curtain_motion != null) ? root.curtain_motion : devices.curtain_motion;

            const st = String(stRaw ?? '').trim().toUpperCase();
            const motion = (motionRaw === null || motionRaw === undefined || motionRaw === '') ? NaN : Number(motionRaw);

            if (st === 'OPENING') return 'Rèm: ĐANG MỞ';
            if (st === 'CLOSING') return 'Rèm: ĐANG ĐÓNG';
            if (st === 'STOP') return 'Rèm: DỪNG';

            if (Number.isFinite(motion)) {
                if (motion > 0) return 'Rèm: ĐANG MỞ';
                if (motion < 0) return 'Rèm: ĐANG ĐÓNG';
                return 'Rèm: DỪNG';
            }

            return 'Rèm: chưa có dữ liệu';
        }

        async function sendCurtainAction(action) {
            if (!ESP_IP) throw new Error('Chưa cấu hình IP/URL ESP32');
            const a = String(action || '').trim().toLowerCase();
            const path = (a === 'open') ? '/api/curtain/open'
                : (a === 'close') ? '/api/curtain/close'
                : (a === 'stop') ? '/api/curtain/stop'
                : '';
            if (!path) throw new Error('Curtain action không hợp lệ');

            const res = await apiFetch(path, { method: 'GET' });
            if (!res.ok) {
                const txt = await res.text().catch(() => '');
                const suffix = txt ? (': ' + txt.slice(0, 200)) : '';
                throw new Error('ESP32 trả lỗi HTTP ' + res.status + suffix);
            }
            return true;
        }

        function toggleVoicePanel(force) {
            const panel = document.getElementById('voice-panel');
            const fab = document.getElementById('voice-fab');
            if (!panel) return;
            const wantOpen = (typeof force === 'boolean') ? force : panel.classList.contains('hidden');
            if (wantOpen) panel.classList.remove('hidden');
            else panel.classList.add('hidden');
            if (fab) fab.classList.toggle('hidden', wantOpen);
            localStorage.setItem('voice_panel_open', wantOpen ? '1' : '0');

            // New conversation greeting behavior
            if (wantOpen) {
                greetMikiIfNeeded();
            } else {
                aiConversationStarted = false;
            }
        }

        function setVoicePanelMode(mode) {
            const m = String(mode || '').toLowerCase() === 'chat' ? 'chat' : 'voice';
            const voiceMode = document.getElementById('voice-mode');
            const chatMode = document.getElementById('chat-mode');
            const voiceTab = document.getElementById('voice-tab');
            const chatTab = document.getElementById('chat-tab');

            if (voiceMode) voiceMode.classList.toggle('hidden', m !== 'voice');
            if (chatMode) chatMode.classList.toggle('hidden', m !== 'chat');

            if (voiceTab) {
                voiceTab.className = (m === 'voice')
                    ? 'flex-1 px-4 py-2.5 rounded-2xl bg-indigo-100 text-indigo-700 font-bold'
                    : 'flex-1 px-4 py-2.5 rounded-2xl bg-white/60 border border-white text-slate-700 font-bold hover:bg-white transition-all';
            }
            if (chatTab) {
                chatTab.className = (m === 'chat')
                    ? 'flex-1 px-4 py-2.5 rounded-2xl bg-indigo-100 text-indigo-700 font-bold'
                    : 'flex-1 px-4 py-2.5 rounded-2xl bg-white/60 border border-white text-slate-700 font-bold hover:bg-white transition-all';
            }

            localStorage.setItem('voice_panel_mode', m);
            if (m === 'chat') {
                const inp = document.getElementById('chat-input');
                if (inp) setTimeout(() => inp.focus(), 0);
            }
        }

        function toggleAiResponse(force) {
            const aiBox = document.getElementById('ai-response-box');
            if (!aiBox) return;
            const wantOpen = (typeof force === 'boolean') ? force : aiBox.classList.contains('hidden');
            if (wantOpen) aiBox.classList.remove('hidden');
            else aiBox.classList.add('hidden');
        }

        let aiConversationStarted = false;

        async function greetMikiIfNeeded() {
            if (aiConversationStarted) return;
            if (typeof isProcessingAi !== 'undefined' && isProcessingAi) return;

            const aiText = document.getElementById('ai-text');
            if (!aiText) return;
            const greeting = 'Chào bạn, tôi là Miki trợ lí ảo nhà thông minh tôi có thể giúp gì cho bạn!';

            toggleAiResponse(true);
            aiText.innerText = greeting;
            try { await speak(greeting); } catch (e) {}

            aiConversationStarted = true;
        }

        function submitChat() {
            const inp = document.getElementById('chat-input');
            const text = inp ? String(inp.value || '').trim() : '';
            if (!text) {
                showToast('Vui lòng nhập nội dung chat', 'error');
                return;
            }
            if (inp) inp.value = '';
            processWithOllama(text);
        }

        function dictateToChat() {
            if (!recognition) return;
            toggleVoicePanel(true);
            setVoicePanelMode('chat');
            recognitionTarget = 'chat';
            try { recognition.stop(); } catch (e) {}
            try { recognition.start(); } catch (e) {}
        }

        function setMicButtonState(listening) {
            const btn = document.getElementById('mic-btn');
            const label = document.getElementById('mic-label');
            if (!btn) return;

            const icon = btn.querySelector('i.ph-microphone');

            if (listening) {
                btn.classList.add('bg-indigo-500', 'border-indigo-500');
                btn.classList.remove('bg-white/80');
                if (icon) {
                    icon.classList.add('text-white');
                    icon.classList.remove('text-indigo-600');
                }
                if (label) {
                    label.innerText = 'Đang nghe...';
                    label.classList.add('text-white');
                    label.classList.remove('text-slate-700');
                }
            } else {
                btn.classList.remove('bg-indigo-500', 'border-indigo-500');
                btn.classList.add('bg-white/80');
                if (icon) {
                    icon.classList.remove('text-white');
                    icon.classList.add('text-indigo-600');
                }
                if (label) {
                    label.innerText = 'MIC';
                    label.classList.remove('text-white');
                    label.classList.add('text-slate-700');
                }
            }
        }

        function getVietnamHour(now = new Date()) {
            try {
                const parts = new Intl.DateTimeFormat('en-US', { timeZone: VN_TZ, hour: '2-digit', hour12: false }).formatToParts(now);
                const h = parts.find(p => p.type === 'hour')?.value;
                const hour = Number(h);
                return Number.isFinite(hour) ? hour : now.getHours();
            } catch (e) {
                return now.getHours();
            }
        }

        function updateGreetingByTime() {
            const now = new Date();
            const hour = getVietnamHour(now);

            let label = 'Good Morning';
            let icon = { cls: 'ph-duotone ph-sun', color: 'text-amber-500' };
            if (hour >= 12 && hour <= 17) {
                label = 'Good Afternoon';
                icon = { cls: 'ph-duotone ph-sun-horizon', color: 'text-orange-500' };
            } else if (hour >= 18 && hour <= 21) {
                label = 'Good Evening';
                icon = { cls: 'ph-duotone ph-cloud-sun', color: 'text-amber-500' };
            } else if (hour >= 22 || hour <= 4) {
                label = 'Good Night';
                icon = { cls: 'ph-duotone ph-moon-stars', color: 'text-indigo-200' };
            }

            const gt = document.getElementById('greeting-time');
            if (gt) gt.innerText = label;
            const gi = document.getElementById('greeting-icon');
            if (gi) gi.className = icon.cls + ' text-2xl ' + icon.color;
        }

        let clockTimeFmt;
        let clockDateFmt;
        try {
            clockTimeFmt = new Intl.DateTimeFormat('vi-VN', { timeZone: VN_TZ, hour: '2-digit', minute: '2-digit' });
            clockDateFmt = new Intl.DateTimeFormat('vi-VN', { timeZone: VN_TZ, weekday: 'short', day: '2-digit', month: '2-digit', year: 'numeric' });
        } catch (e) {
            clockTimeFmt = new Intl.DateTimeFormat('vi-VN', { hour: '2-digit', minute: '2-digit' });
            clockDateFmt = new Intl.DateTimeFormat('vi-VN', { weekday: 'short', day: '2-digit', month: '2-digit', year: 'numeric' });
        }

        function formatLikeTopClock(dateObj) {
            if (!dateObj || isNaN(dateObj.getTime())) return '--';
            try {
                return `${clockDateFmt.format(dateObj)} · ${clockTimeFmt.format(dateObj)}`;
            } catch (e) {
                return dateObj.toLocaleString('vi-VN');
            }
        }

        setInterval(() => {
            const now = new Date();
            const t = document.getElementById('clock-time');
            const d = document.getElementById('clock-date');
            if (t) t.innerText = clockTimeFmt.format(now);
            if (d) d.innerText = clockDateFmt.format(now);
        }, 1000);

        // Greeting follows real Vietnam time.
        updateGreetingByTime();
        setInterval(updateGreetingByTime, 60 * 1000);

        // 2. Voice (TTS) & AI Glow Logic
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
                if (x.lang === preferredLang.toLowerCase()) s += 50;
                if (x.lang.startsWith(preferredLang.split('-')[0].toLowerCase())) s += 15;
                // Prefer Google/Chrome voices when available (closer to gTTS/Google voice)
                if (x.name.includes('google')) s += 35;
                if (x.name.includes('microsoft')) s += 10;
                if (x.local) s += 5;
                const femaleHints = ['female', 'linh', 'thuy', 'hoai', 'my', 'hoaimy', 'jenny', 'aria', 'sara', 'zira', 'natasha', 'emma', 'olivia', 'amy'];
                if (femaleHints.some(h => x.name.includes(h))) s += 18;
                return s;
            };

            const viCandidates = scored
                .filter(x => x.lang.startsWith('vi'))
                .sort((a, b) => scoreVoice(b, 'vi-VN') - scoreVoice(a, 'vi-VN'));
            vietnameseVoice = viCandidates.length ? viCandidates[0].v : null;
        }

        window.speechSynthesis.onvoiceschanged = initVoices;
        initVoices();
        setTimeout(initVoices, 250);

        function setAiSpeaking(active) {
            const bars = document.querySelectorAll('.wave-bar');
            const card = document.getElementById('ai-card');
            
            if(active) {
                bars.forEach(b => b.classList.remove('paused'));
                if(card) card.classList.add('miki-glow-active');
            } else {
                bars.forEach(b => b.classList.add('paused'));
                if(card) card.classList.remove('miki-glow-active');
            }
        }

        function speakWithBrowser(text) {
            window.speechSynthesis.cancel();
            const u = new SpeechSynthesisUtterance(text);
            u.lang = 'vi-VN';
            // Closer to gTTS defaults
            u.rate = 1.0;
            u.pitch = 1.0;
            if (vietnameseVoice) u.voice = vietnameseVoice;
            
            u.onstart = () => setAiSpeaking(true);
            u.onend = () => setAiSpeaking(false);
            
            window.speechSynthesis.speak(u);
        }

        async function speak(text) {
            const gcpKey = localStorage.getItem('gcp_tts_api_key') || '';
            setAiSpeaking(true);

            if (gcpKey) {
                try {
                    const res = await fetch(`${BASE_URL}/AiController/tts`, {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ text })
                    });
                    const data = await res.json();
                    if (data.status === 'success' && data.audioContent) {
                        const mime = (data.audioEncoding || '').toUpperCase() === 'MP3' ? 'audio/mpeg' : 'audio/mpeg';
                        const player = new Audio(`data:${mime};base64,${data.audioContent}`);
                        player.onended = () => setAiSpeaking(false);
                        await player.play();
                        return;
                    }
                } catch (e) { }
            }
            speakWithBrowser(text);
        }

        // Typing Effect Logic
        function typeTextToElement(el, text) {
            if (!el) return;
            const target = String(text || '');

            const oldTimer = el.dataset.typeTimerId ? parseInt(el.dataset.typeTimerId, 10) : null;
            if (oldTimer) {
                clearInterval(oldTimer);
                delete el.dataset.typeTimerId;
            }

            el.innerText = '';
            if (target.length === 0) return;

            const intervalMs = 25; 
            let i = 0;
            const timer = setInterval(() => {
                i++;
                el.innerText = target.slice(0, i);
                if (i >= target.length) {
                    clearInterval(timer);
                    delete el.dataset.typeTimerId;
                }
            }, intervalMs);

            el.dataset.typeTimerId = String(timer);
        }

        // 3. Send Command
        let pendingCommand = null;

        function _expectedStateForCommand(cmd) {
            const c = String(cmd || '');
            const exp = {};
            if (c === 'FAN1_ON') exp.fan1 = true;
            if (c === 'FAN1_OFF') exp.fan1 = false;
            if (c === 'FAN2_ON') exp.fan2 = true;
            if (c === 'FAN2_OFF') exp.fan2 = false;
            if (c === 'LED1_ON' || c === 'LIGHT1_ON') exp.light1 = true;
            if (c === 'LED1_OFF' || c === 'LIGHT1_OFF') exp.light1 = false;
            if (c === 'LIGHT2_ON') exp.light2 = true;
            if (c === 'LIGHT2_OFF') exp.light2 = false;
            if (c === 'LIGHT3_ON') exp.light3 = true;
            if (c === 'LIGHT3_OFF') exp.light3 = false;
            if (c === 'LIGHTS_ALL_ON') { exp.light1 = true; exp.light2 = true; exp.light3 = true; }
            if (c === 'LIGHTS_ALL_OFF') { exp.light1 = false; exp.light2 = false; exp.light3 = false; }

            // When selecting a global color (including RAINBOW), the UX expects lights to be ON.
            if (c.startsWith('LIGHTS_ALL_COLOR_')) {
                const color = c.substring(String('LIGHTS_ALL_COLOR_').length).trim().toUpperCase();
                if (color) exp.lights_all_color = color;
                exp.light1 = true;
                exp.light2 = true;
                exp.light3 = true;
            }
            if (c === 'DOOR_OPEN') exp.door_open = true;
            if (c === 'DOOR_CLOSE') exp.door_open = false;
            if (c.startsWith('DOOR_PIN_EMERGENCY_SET:')) exp.door_emergency_pin_set = true;
            if (c === 'CURTAIN_OPEN') exp.curtain_open = true;
            if (c === 'CURTAIN_CLOSE') exp.curtain_open = false;
            return Object.keys(exp).length ? exp : null;
        }

        function _matchesExpected(devices, expected) {
            if (!devices || !expected) return false;
            return Object.keys(expected).every(k => {
                if (!Object.prototype.hasOwnProperty.call(devices, k)) return false;
                const expVal = expected[k];
                const devVal = devices[k];
                if (typeof expVal === 'boolean') return !!devVal === expVal;
                // Compare strings (e.g., lights_all_color)
                return String(devVal ?? '').trim().toUpperCase() === String(expVal ?? '').trim().toUpperCase();
            });
        }

        async function sendCommand(cmd) {
            console.log("Send:", cmd);

            // Curtain must use dedicated endpoints (no generic /control?cmd=...)
            if (cmd === 'CURTAIN_OPEN' || cmd === 'CURTAIN_CLOSE' || cmd === 'CURTAIN_STOP') {
                try {
                    const curtainIcon = document.getElementById('icon-curtain');
                    if (curtainIcon) {
                        curtainIcon.classList.add('animate-pulse');
                        setTimeout(() => curtainIcon.classList.remove('animate-pulse'), 900);
                    }

                    if (cmd === 'CURTAIN_OPEN') await sendCurtainAction('open');
                    if (cmd === 'CURTAIN_CLOSE') await sendCurtainAction('close');
                    if (cmd === 'CURTAIN_STOP') await sendCurtainAction('stop');

                    if (cmd === 'CURTAIN_OPEN') showToast('Đã gửi lệnh MỞ', 'success');
                    if (cmd === 'CURTAIN_CLOSE') showToast('Đã gửi lệnh ĐÓNG', 'success');
                    if (cmd === 'CURTAIN_STOP') showToast('Đã gửi lệnh DỪNG', 'success');
                } catch (e) {
                    console.error("Connection Error:", e);
                    document.getElementById('connection-msg').innerText = "Disconnected";
                    document.getElementById('status-dot').className = "w-2.5 h-2.5 rounded-full bg-rose-500 ring-4 ring-rose-100";
                    showToast('Gửi lệnh rèm thất bại: ' + (e && e.message ? e.message : 'mất kết nối'), 'error');
                } finally {
                    setTimeout(pollSensorsOnce, 300);
                }
                return;
            }

            const safeCmd = encodeURIComponent(String(cmd || '').trim());
            const path = `/control?cmd=${safeCmd}`;
            
            try {
                const data = await apiGetJson(path);

                if (!data || data.ok !== true) {
                    const errMsg = (data && data.error) ? String(data.error) : 'ESP32 từ chối lệnh';
                    showToast(errMsg, 'error');
                    return;
                }

                if (cmd === 'OFF') {
                    showToast('Đã gửi lệnh DỪNG KHẨN CẤP', 'success');
                } else {
                    showToast('Đã gửi lệnh: ' + cmd, 'success');
                }

                const expected = _expectedStateForCommand(cmd);
                if (expected) {
                    pendingCommand = { cmd, expected, at: Date.now() };
                }

                if (cmd === 'FAN1_ON' || cmd === 'FAN1_OFF') {
                    const fanIcon = document.getElementById('icon-fan');
                    if (fanIcon) {
                        fanIcon.classList.add('animate-spin');
                        setTimeout(() => fanIcon.classList.remove('animate-spin'), 1200);
                    }
                }
                if (cmd === 'FAN2_ON' || cmd === 'FAN2_OFF') {
                    const fanIcon2 = document.getElementById('icon-fan2');
                    if (fanIcon2) {
                        fanIcon2.classList.add('animate-spin');
                        setTimeout(() => fanIcon2.classList.remove('animate-spin'), 1200);
                    }
                }
            } catch(e) {
                console.error("Connection Error:", e);
                document.getElementById('connection-msg').innerText = "Disconnected";
                document.getElementById('status-dot').className = "w-2.5 h-2.5 rounded-full bg-rose-500 ring-4 ring-rose-100";
                showToast('Gửi lệnh thất bại: ' + (e && e.message ? e.message : 'mất kết nối'), 'error');
            } finally {
                // Refresh device states shortly after sending a command
                setTimeout(pollSensorsOnce, 300);
                setTimeout(refreshRfid, 600);
            }
        }

        const colorClassMap = {
            YELLOW: 'bg-amber-400 ring-amber-100',
            RED: 'bg-rose-500 ring-rose-100',
            GREEN: 'bg-emerald-500 ring-emerald-100',
            BLUE: 'bg-indigo-500 ring-indigo-100',
            PURPLE: 'bg-violet-500 ring-violet-100',
            WHITE: 'bg-white ring-slate-100',
            RAINBOW: 'swatch-rainbow ring-indigo-100'
        };

        function setSwatch(id, color) {
            const el = document.getElementById(id);
            if (!el) return;
            const c = String(color || '').toUpperCase();
            const cls = colorClassMap[c] || colorClassMap.YELLOW;

            // reset minimal set we use
            el.className = 'w-3 h-3 rounded-full ring-4 ' + cls;
        }

        function setLightColor(lightKey, color) {
            const lk = String(lightKey || '').toUpperCase();
            const c = String(color || '').toUpperCase();
            if (!lk || !c) return;
            sendCommand(`${lk}_COLOR_${c}`);
            if (lk === 'LIGHT1') setSwatch('sw-light1', c);
            if (lk === 'LIGHT2') setSwatch('sw-light2', c);
            if (lk === 'LIGHT3') setSwatch('sw-light3', c);
        }

        async function setAllLightsColor(color) {
            const c = String(color || '').toUpperCase();
            if (!c) return;
            // In UI, users expect "nhiều màu" to actually turn the lights on.
            if (c === 'RAINBOW') {
                await sendCommand('LIGHTS_ALL_ON');
                await new Promise(r => setTimeout(r, 220));
                await sendCommand(`LIGHTS_ALL_COLOR_${c}`);
            } else {
                await sendCommand(`LIGHTS_ALL_COLOR_${c}`);
            }
            setSwatch('sw-lights-all', c);
            // Optimistic UI update
            const rainbowPrimary = 'PURPLE';
            setSwatch('sw-light3', c === 'RAINBOW' ? rainbowPrimary : c);
            // Requirement: Light 1 should match Light 3 when controlling all floors
            setSwatch('sw-light1', c === 'RAINBOW' ? rainbowPrimary : c);
            setSwatch('sw-light2', c === 'RAINBOW' ? 'BLUE' : c);
        }

        function openDoorWithPin() {
            const el = document.getElementById('door-pin-input');
            const pin = (el ? String(el.value || '').trim() : '');
            if (!pin) {
                showToast('Vui lòng nhập mật khẩu', 'error');
                return;
            }
            if (el) el.value = '';
            sendCommand(`DOOR_PIN:${pin}`);
        }

        function setDoorPin() {
            const el = document.getElementById('door-pin-set-input');
            const pin = (el ? String(el.value || '').trim() : '');
            if (!pin) {
                showToast('Vui lòng nhập mật khẩu mới', 'error');
                return;
            }
            if (el) el.value = '';
            sendCommand(`DOOR_PIN_SET:${pin}`);
            showToast('Đã gửi lệnh lưu mật khẩu', 'success');
        }

        function setDoorEmergencyPin() {
            const el = document.getElementById('door-pin-emergency-input');
            const pin = (el ? String(el.value || '').trim() : '');
            if (!pin) {
                showToast('Vui lòng nhập mật khẩu khẩn cấp', 'error');
                return;
            }
            if (el) el.value = '';
            sendCommand(`DOOR_PIN_EMERGENCY_SET:${pin}`);
            showToast('Đã gửi lệnh lưu mật khẩu khẩn cấp', 'success');
        }

        async function refreshRfid(source = 'auto') {
            // Prevent overlapping refreshes (interval + manual clicks).
            if (window.__rfidRefreshInFlight) {
                // If user clicked while auto refresh is running, queue exactly one extra run.
                if (source === 'manual') window.__rfidRefreshPending = true;
                return;
            }
            window.__rfidRefreshInFlight = true;

            const btnRfidRefresh = document.getElementById('btn-rfid-refresh');
            const prevBtnText = btnRfidRefresh ? String(btnRfidRefresh.innerText || '') : '';
            if (btnRfidRefresh) {
                btnRfidRefresh.disabled = true;
                btnRfidRefresh.innerText = 'Đang tải...';
            }

            try {
            const gscriptUrl = getGscriptUrl();
            const canUseGscript = !!gscriptUrl;

            // If neither ESP nor Apps Script is configured, show a clear hint instead of staying blank.
            if (!canUseGscript && !ESP_IP) {
                const now = Date.now();
                window.__rfidConfigToastAt = window.__rfidConfigToastAt || 0;
                if (now - window.__rfidConfigToastAt > 15000) {
                    showToast('RFID: Chưa cấu hình Apps Script URL hoặc ESP32 URL', 'error');
                    window.__rfidConfigToastAt = now;
                }

                const setText = (id, text) => {
                    const el = document.getElementById(id);
                    if (el) el.innerText = String(text || '--');
                };
                setText('rfid-last-uid', '--');
                setText('rfid-last-time', '--');
                setText('rfid-last-auth', '--');

                const cardsEl = document.getElementById('rfid-cards');
                if (cardsEl) {
                    cardsEl.innerHTML = '';
                    const row = document.createElement('div');
                    row.className = 'text-slate-400 text-sm';
                    row.innerText = '(chưa cấu hình nguồn dữ liệu)';
                    cardsEl.appendChild(row);
                }

                const histEl = document.getElementById('rfid-history');
                if (histEl) {
                    histEl.innerHTML = '';
                    const row = document.createElement('div');
                    row.className = 'text-slate-400 text-sm';
                    row.innerText = '(chưa cấu hình nguồn dữ liệu)';
                    histEl.appendChild(row);
                }
                return;
            }

            const toEpochMs = (v) => {
                if (v === null || v === undefined || v === '') return null;
                if (typeof v === 'number' && Number.isFinite(v)) {
                    // Heuristic: seconds vs milliseconds
                    // seconds usually ~1e9-1e10; ms ~1e12-1e13
                    return v < 1e12 ? Math.round(v * 1000) : Math.round(v);
                }
                const s = String(v).trim();
                if (!s) return null;
                const n = Number(s);
                if (Number.isFinite(n)) return n < 1e12 ? Math.round(n * 1000) : Math.round(n);

                // IMPORTANT: Never Date-parse slash dates (DD/MM/YYYY ...)
                // because browsers may treat them as MM/DD and flip day/month.
                if (/^\d{1,2}\/\d{1,2}\/\d{4}(?:\b|\s)/.test(s)) return null;

                const d = new Date(s);
                const t = d.getTime();
                return Number.isFinite(t) ? t : null;
            };

            const isReasonableEpochMs = (ms) => {
                // guard against "1970" / invalid timestamps
                return Number.isFinite(ms) && ms >= 946684800000 && ms <= 4102444800000;
            };

            const pickEpochMs = (...vals) => {
                for (const v of vals) {
                    const ms = toEpochMs(v);
                    if (isReasonableEpochMs(ms)) return ms;
                }
                return null;
            };

            const formatTsFromAny = (...vals) => {
                const ms = pickEpochMs(...vals);
                if (!ms) return '--';
                return formatLikeTopClock(new Date(ms));
            };

            const normalizeSheetTimeText = (s) => {
                let t = String(s || '').trim();
                if (!t) return '';

                // Remove JS Date suffix like: "GMT+0700 (Giờ Đông Dương)".
                t = t.replace(/\s*GMT[+-]\d{4}.*$/i, '').trim();
                // GAS/JS sometimes includes a comma between date and time.
                t = t.replace(/,\s*/g, ' ').trim();

                const pad2 = (n) => {
                    const x = String(n);
                    return x.length === 1 ? ('0' + x) : x;
                };

                // ISO (preferred unambiguous): 2026-01-06 or 2026-01-06 13:05:02
                // Convert to DD/MM/YYYY and keep time if present.
                {
                    const m = t.match(/^(\d{4})-(\d{1,2})-(\d{1,2})(?:[ T](\d{1,2}):(\d{1,2})(?::(\d{1,2}))?)?/);
                    if (m) {
                        const yyyy = m[1];
                        const mm = pad2(m[2]);
                        const dd = pad2(m[3]);
                        const hh = m[4] ? pad2(m[4]) : null;
                        const mi = m[5] ? pad2(m[5]) : null;
                        const ss = m[6] ? pad2(m[6]) : null;
                        const datePart = `${dd}/${mm}/${yyyy}`;
                        if (hh && mi) {
                            const timePart = ss ? `${hh}:${mi}:${ss}` : `${hh}:${mi}`;
                            return `${datePart} ${timePart}`;
                        }
                        return datePart;
                    }
                }

                // Slash format from Sheet/UI: DD/MM/YYYY (optionally with time)
                // IMPORTANT: per project requirement, treat as DD/MM/YYYY (do NOT swap).
                {
                    const m = t.match(/^(\d{1,2})\/(\d{1,2})\/(\d{4})(.*)$/);
                    if (m) {
                        const dd = pad2(m[1]);
                        const mm = pad2(m[2]);
                        const yyyy = m[3];
                        const rest = String(m[4] || '').trim();
                        return rest ? `${dd}/${mm}/${yyyy} ${rest}` : `${dd}/${mm}/${yyyy}`;
                    }
                }

                return t;
            };

            const setText = (id, text) => {
                const el = document.getElementById(id);
                if (el) el.innerText = String(text || '--');
            };

            const renderSavedCards = (list) => {
                const el = document.getElementById('rfid-cards');
                if (!el) return;
                el.innerHTML = '';
                if (!list || !list.length) {
                    const row = document.createElement('div');
                    row.className = 'text-slate-400 text-sm';
                    row.innerText = '(trống)';
                    el.appendChild(row);
                    return;
                }

                list.slice(0, 30).forEach(card => {
                    const uidRaw = pickRfidUid(card);
                    const key = normalizeUid(uidRaw);
                    const cached = key ? (window.__rfidCardCache && window.__rfidCardCache[key] ? window.__rfidCardCache[key] : null) : null;
                    const uid = key || String(uidRaw || '').trim();
                    const name = cached && typeof cached === 'object'
                        ? String(cached.name || '').trim()
                        : String(pickRfidName(card) || '').trim();

                    const btn = document.createElement('button');
                    btn.type = 'button';
                    // Match the visual style of scan history rows, but keep click-to-fill.
                    btn.className = 'w-full text-left flex items-center justify-between gap-2 px-2 py-1.5 rounded-lg hover:bg-white/70 transition-all';

                    const left = document.createElement('div');
                    left.className = 'truncate';
                    left.innerText = name ? `${name} · ${uid}` : uid;

                    const right = document.createElement('div');
                    right.className = 'text-xs text-slate-400';
                    right.innerText = '';

                    btn.appendChild(left);
                    btn.appendChild(right);

                    btn.addEventListener('click', () => {
                        const uidEl = document.getElementById('rfid-uid-input');
                        const nameEl = document.getElementById('rfid-name-input');
                        if (uidEl) uidEl.value = uid;
                        if (nameEl) nameEl.value = name;
                    });
                    el.appendChild(btn);
                });
            };
            const setList = (id, items) => {
                const el = document.getElementById(id);
                if (!el) return;
                el.innerHTML = '';
                (items || []).slice(0, 20).forEach(x => {
                    const row = document.createElement('div');
                    row.className = 'flex items-center justify-between gap-2';
                    const left = document.createElement('div');
                    left.className = 'truncate';
                    const uid = normalizeUid(pickRfidUid(x) || String((x && typeof x === 'object') ? (x.uid || x.UID || x.rfid || x.card_uid || '') : (x || '')));
                    const name = String(pickRfidName(x) || ((x && typeof x === 'object') ? (x.name || x.ten || x.Name || x.Ten || '') : '')).trim();
                    const ok = (x && typeof x === 'object') ? (x.authorized ?? x.ok ?? null) : null;
                    const status = (x && typeof x === 'object') ? (x.status ?? x.finalStatus ?? x.state ?? '') : '';
                    const tag = ok === true ? 'OK' : (ok === false ? 'FAIL' : (status ? String(status).toUpperCase() : ''));
                    const label = name ? `${name} · ${uid}` : uid;
                    left.innerText = tag ? `${label} (${tag})` : `${label}`;
                    const right = document.createElement('div');
                    right.className = 'text-xs text-slate-400';
                    const sheetTs = (x && typeof x === 'object')
                        ? String(x.timestamp ?? x.ts ?? x.datetime ?? x.time ?? '').trim()
                        : '';
                    const sheetMs = pickEpochMs((x && typeof x === 'object') ? (x.sheet_at ?? null) : null);
                    const ms = pickEpochMs(
                        sheetMs,
                        (x && typeof x === 'object') ? (x.time_ms ?? null) : null,
                        (x && typeof x === 'object') ? (x.updated_at ?? null) : null,
                        (x && typeof x === 'object') ? (x.at ?? null) : null,
                        (x && typeof x === 'object') ? (x.ts_ms ?? null) : null,
                        (x && typeof x === 'object') ? (x.timestamp_ms ?? null) : null
                    );
                    // Prefer timestamp string from Google Sheet (exact), otherwise show full date+time.
                    right.innerText = sheetTs ? normalizeSheetTimeText(sheetTs) : (ms ? formatLikeTopClock(new Date(ms)) : '');
                    row.appendChild(left);
                    row.appendChild(right);
                    el.appendChild(row);
                });
            };

            try {
                // Optional: last scan (from Apps Script if configured, else from ESP32/bridge).
                let last = null;
                if (canUseGscript) {
                    const data = await gscriptCallWithActionFallback(
                        ['rfid_last', 'last', 'rfid_recent', 'recent'],
                        {}
                    );
                    if (data && typeof data === 'object' && data.ok === false) {
                        throw new Error(String(data.message || 'Apps Script báo lỗi'));
                    }
                    last = (data && data.last) ? data.last : (data || null);
                } else {
                    if (!ESP_IP) throw new Error('no-esp');
                    last = await apiGetJson('/rfid/last');
                }

                if (last) {
                    const uid = pickRfidUid(last) || '--';
                    const name = pickRfidName(last);
                    const label = name ? `${uid} · ${name}` : uid;
                    setText('rfid-last-uid', label);

                    const ms = pickEpochMs(last.time_ms, last.updated_at, last.at, last.ts_ms, last.timestamp_ms);
                    const tsText = (last.timestamp ? normalizeSheetTimeText(last.timestamp) : formatTsFromAny(ms));
                    setText('rfid-last-time', tsText);

                    const authTag = (last.authorized === true) ? 'OK'
                        : ((last.authorized === false) ? 'FAIL'
                            : (last.status ? String(last.status).toUpperCase() : '--'));
                    setText('rfid-last-auth', authTag);

                    // Notify only when a NEW scan appears (avoid spamming every refresh)
                    const eventKey = `${String(uid)}|${String(authTag)}|${String(ms ?? tsText ?? '')}`;
                    window.__lastRfidEventKey = window.__lastRfidEventKey || '';

                    const looksRealUid = uid && uid !== '--' && String(uid).trim().length >= 4;
                    if (looksRealUid && eventKey !== window.__lastRfidEventKey && window.__lastRfidEventKey) {
                        const msg = `RFID: ${label} (${authTag})`;
                        const ok = (authTag === 'OK' || authTag === 'IN' || authTag === 'OUT');
                        showToast(msg, ok ? 'success' : 'error');
                    }
                    if (looksRealUid) window.__lastRfidEventKey = eventKey;
                }
            } catch (e) {
                if (String(e && e.message || '') !== 'no-esp') {
                    console.warn('[RFID] last failed', e);

                    const now = Date.now();
                    window.__rfidLastErrToastAt = window.__rfidLastErrToastAt || 0;
                    if (now - window.__rfidLastErrToastAt > 15000) {
                        showToast('RFID: Không đọc được lần quét gần nhất', 'error');
                        window.__rfidLastErrToastAt = now;
                    }
                }
            }

            try {
                let list = [];
                window.__rfidCardListSupported = window.__rfidCardListSupported ?? null;
                if (canUseGscript) {
                    try {
                        const cards = await gscriptCallWithActionFallback(
                            ['rfid_list', 'rfid_cards', 'cards', 'card_list', 'list'],
                            {}
                        );
                        if (cards && typeof cards === 'object' && cards.ok === false) {
                            const msg = String(cards.message || 'Apps Script báo lỗi');
                            // The provided Apps Script URL may not implement a card list endpoint.
                            if (/unknown\s+action/i.test(msg)) {
                                window.__rfidCardListSupported = false;
                                list = [];
                            } else {
                                throw new Error(msg);
                            }
                        } else {
                            window.__rfidCardListSupported = true;
                            list = Array.isArray(cards.cards) ? cards.cards : (Array.isArray(cards.list) ? cards.list : []);
                        }
                    } catch (e) {
                        // If Apps Script deployment doesn't support rfid_list (e.g. "Unknown action"),
                        // fall back to ESP bridge endpoint so the UI can still show saved cards.
                        if (/unknown\s+action/i.test(String((e && e.message) ? e.message : ''))) {
                            window.__rfidCardListSupported = false;
                        }
                        if (ESP_IP) {
                            const cards = await apiGetJson('/rfid/cards');
                            list = Array.isArray(cards.cards) ? cards.cards : [];
                        } else {
                            // No ESP fallback: keep list empty.
                            list = [];
                        }
                    }
                } else if (ESP_IP) {
                    const cards = await apiGetJson('/rfid/cards');
                    list = Array.isArray(cards.cards) ? cards.cards : [];
                }

                const prevCache = window.__rfidCardCache || {};
                window.__rfidCardCache = {};

                list.forEach((card) => {
                    const uid = pickRfidUid(card);
                    const key = normalizeUid(uid);
                    if (!key) return;
                    const name = pickRfidName(card);
                    const prev = prevCache[key];
                    const prevName = (prev && typeof prev === 'object') ? String(prev.name || prev.ten || prev.card_name || prev.cardName || '') : '';
                    window.__rfidCardCache[key] = {
                        ...(typeof card === 'object' ? card : { uid: key }),
                        uid: key,
                        name: (name && name.trim()) ? name.trim() : ((prevName && prevName.trim()) ? prevName.trim() : ''),
                    };
                });

                // Keep for later enrichment from history (Apps Script only) and re-render if names appear.
                window.__rfidSavedCardsLastList = Array.isArray(list) ? list : [];

                renderSavedCards(list);
            } catch (e) {
                console.warn('[RFID] cards failed', e);

                // Also reflect the error inside the "Thẻ đã lưu" box (users often miss toasts).
                const cardsEl = document.getElementById('rfid-cards');
                if (cardsEl) {
                    cardsEl.innerHTML = '';
                    const row = document.createElement('div');
                    row.className = 'text-slate-400 text-sm';
                    const msg = (e && e.message) ? String(e.message) : '';
                    row.innerText = msg ? `(lỗi: ${msg})` : '(lỗi tải danh sách thẻ)';
                    cardsEl.appendChild(row);
                }

                const now = Date.now();
                window.__rfidCardsErrToastAt = window.__rfidCardsErrToastAt || 0;
                if (now - window.__rfidCardsErrToastAt > 15000) {
                    const msg = (e && e.message) ? String(e.message) : '';
                    const suffix = msg ? (': ' + msg) : '';
                    showToast('RFID: Không tải được danh sách thẻ (kiểm tra Apps Script URL/Deploy)' + suffix, 'error');
                    window.__rfidCardsErrToastAt = now;
                }
            }

            try {
                if (canUseGscript) {
                    const hist = await gscriptCallWithActionFallback(
                        ['rfid_history', 'history', 'rfid_logs', 'logs'],
                        { limit: 20 }
                    );
                    if (hist && typeof hist === 'object' && hist.ok === false) {
                        throw new Error(String(hist.message || 'Apps Script báo lỗi'));
                    }
                    const list = Array.isArray(hist.history) ? hist.history : (Array.isArray(hist.logs) ? hist.logs : []);
                    setList('rfid-history', list);

                    // Enrich saved-card names from history (Apps Script may only return UID for rfid_list).
                    // If we learn any missing names, re-render "Thẻ đã lưu" to match history style.
                    {
                        const cache = window.__rfidCardCache || {};
                        let changed = false;
                        (list || []).forEach((row) => {
                            if (!row || typeof row !== 'object') return;
                            const uid = normalizeUid(pickRfidUid(row) || String(row.uid || row.rfid || row.card_uid || row.UID || ''));
                            if (!uid) return;
                            const name = String(pickRfidName(row) || row.name || row.ten || row.Name || row.Ten || '').trim();
                            if (!name) return;
                            const existing = cache[uid];
                            if (existing && typeof existing === 'object') {
                                const curName = String(existing.name || '').trim();
                                if (!curName) {
                                    existing.name = name;
                                    cache[uid] = existing;
                                    changed = true;
                                }
                            }
                        });

                        if (changed) {
                            window.__rfidCardCache = cache;
                            const savedList = window.__rfidSavedCardsLastList;
                            if (Array.isArray(savedList) && savedList.length) {
                                renderSavedCards(savedList);
                            }
                        }
                    }

                    // If history exists and last-scan UI is empty, fill it from the newest log.
                    const first = list && list.length ? list[0] : null;
                    const curLastUid = String((document.getElementById('rfid-last-uid') || {}).innerText || '').trim();
                    if (first && (!curLastUid || curLastUid === '--')) {
                        const uid = pickRfidUid(first) || first.uid || first.UID || '--';
                        const name = pickRfidName(first) || (first.name ? String(first.name) : '') || (first.Name ? String(first.Name) : '');
                        const label = name ? `${uid} · ${name}` : uid;
                        setText('rfid-last-uid', label);
                        setText('rfid-last-time', first.timestamp ? normalizeSheetTimeText(first.timestamp) : '--');
                        setText('rfid-last-auth', first.status ? String(first.status).toUpperCase() : '--');
                    }
                } else {
                    if (!ESP_IP) throw new Error('no-esp');
                    const hist = await apiGetJson('/rfid/history');
                    if (hist) {
                        const list = Array.isArray(hist.history) ? hist.history : [];
                        setList('rfid-history', list);
                    }
                }
            } catch (e) {
                if (String(e && e.message || '') !== 'no-esp') {
                    console.warn('[RFID] history failed', e);

                    const now = Date.now();
                    window.__rfidHistErrToastAt = window.__rfidHistErrToastAt || 0;
                    if (now - window.__rfidHistErrToastAt > 15000) {
                        showToast('RFID: Không tải được lịch sử quét', 'error');
                        window.__rfidHistErrToastAt = now;
                    }
                }
            }
        } finally {
            window.__rfidRefreshInFlight = false;
            if (btnRfidRefresh) {
                btnRfidRefresh.disabled = false;
                btnRfidRefresh.innerText = prevBtnText || 'Refresh';
            }
            if (window.__rfidRefreshPending) {
                window.__rfidRefreshPending = false;
                // Run once more shortly after finishing.
                setTimeout(() => refreshRfid('auto'), 50);
            }
        }
        }

        async function addRfidCard() {
            const uidEl = document.getElementById('rfid-uid-input');
            const nameEl = document.getElementById('rfid-name-input');

            const uid = normalizeUid(uidEl ? uidEl.value : '');
            const name = (nameEl ? String(nameEl.value || '').trim() : '');

            if (!uid || !isValidUid(uid)) {
                showToast('UID không hợp lệ (HEX 4..20 ký tự, không khoảng trắng)', 'error');
                return;
            }
            if (!name) {
                showToast('Vui lòng nhập Tên', 'error');
                return;
            }

            try {
                const cache = window.__rfidCardCache || {};
                const exists = !!cache[uid];
                const actionCandidates = exists
                    ? ['rfid_update', 'rfid_edit', 'update', 'edit']
                    : ['rfid_add', 'rfid_create', 'add', 'create'];
                const data = await gscriptCallWithActionFallback(actionCandidates, {
                    uid, UID: uid, card_uid: uid, rfid: uid, id: uid,
                    name, Name: name, ten: name, Ten: name,
                    owner: name, holder: name, user: name,
                });

                const ok = (data && data.ok === true);
                const msg = (data && data.message) ? String(data.message) : (ok ? 'OK' : 'FAIL');
                if (!ok && /unknown\s+action/i.test(msg)) {
                    showToast('Apps Script URL này không hỗ trợ THÊM/CẬP NHẬT thẻ (thiếu action)', 'error');
                    return;
                }
                showToast(msg, ok ? 'success' : 'error');
                if (!ok) return;

                // Verify persistence when the Apps Script supports listing.
                try {
                    const cards = await gscriptCallWithActionFallback(['rfid_list', 'cards', 'card_list', 'list'], {});
                    const list = Array.isArray(cards && cards.cards) ? cards.cards : (Array.isArray(cards && cards.list) ? cards.list : []);
                    const found = (list || []).some((c) => normalizeUid(pickRfidUid(c)) === uid);
                    if (!found) {
                        showToast('Apps Script báo OK nhưng danh sách thẻ chưa thay đổi (kiểm tra Apps Script ghi Sheet)', 'error');
                    }
                } catch (_) {}

                if (uidEl) uidEl.value = '';
                if (nameEl) nameEl.value = '';
                refreshRfid();
            } catch (e) {
                showToast('Thêm thẻ thất bại: ' + (e && e.message ? e.message : ''), 'error');
            }
        }

        async function deleteRfidCard() {
            const uidEl = document.getElementById('rfid-uid-input');
            const nameEl = document.getElementById('rfid-name-input');

            const uid = normalizeUid(uidEl ? uidEl.value : '');
            if (!uid || !isValidUid(uid)) {
                showToast('UID không hợp lệ (HEX 4..20 ký tự, không khoảng trắng)', 'error');
                return;
            }
            try {
                const data = await gscriptCallWithActionFallback(['rfid_del', 'rfid_delete', 'rfid_remove', 'del', 'delete', 'remove'], {
                    uid, UID: uid, card_uid: uid, rfid: uid, id: uid,
                });
                const ok = (data && data.ok === true);
                const msg = (data && data.message) ? String(data.message) : (ok ? 'OK' : 'FAIL');
                if (!ok && /unknown\s+action/i.test(msg)) {
                    showToast('Apps Script URL này không hỗ trợ XOÁ thẻ (thiếu action)', 'error');
                    return;
                }
                showToast(msg, ok ? 'success' : 'error');
                if (!ok) return;

                // Verify persistence when the Apps Script supports listing.
                try {
                    const cards = await gscriptCallWithActionFallback(['rfid_list', 'cards', 'card_list', 'list'], {});
                    const list = Array.isArray(cards && cards.cards) ? cards.cards : (Array.isArray(cards && cards.list) ? cards.list : []);
                    const found = (list || []).some((c) => normalizeUid(pickRfidUid(c)) === uid);
                    if (found) {
                        showToast('Apps Script báo OK nhưng thẻ vẫn còn (kiểm tra Apps Script xoá trong Sheet)', 'error');
                    }
                } catch (_) {}
                if (uidEl) uidEl.value = '';
                if (nameEl) nameEl.value = '';
                refreshRfid();
            } catch (e) {
                showToast('Xoá thẻ thất bại: ' + (e && e.message ? e.message : ''), 'error');
            }
        }

        function applyDeviceStates(devices) {
            if (!devices) return;

            const toBool = (v) => {
                if (v === true || v === false) return v;
                if (v === 1 || v === 0) return v === 1;
                const s = String(v ?? '').trim().toLowerCase();
                if (s === 'true' || s === '1' || s === 'on' || s === 'yes') return true;
                if (s === 'false' || s === '0' || s === 'off' || s === 'no' || s === '') return false;
                return !!v;
            };

            const allColor = String(devices.lights_all_color ?? '').trim().toUpperCase();
            // Important: "Nhiều màu" is a *mode* users expect to mean the 3-floor lights are ON.
            // If the backend is late/partial, we still present ON while in RAINBOW mode.
            const rainbowMode = allColor === 'RAINBOW';
            const light1On = toBool(devices.light1) || rainbowMode;
            const light2On = toBool(devices.light2) || rainbowMode;
            const light3On = toBool(devices.light3) || rainbowMode;

            const setTogglePair = (cfg, isOn) => {
                if (!cfg) return;
                const onBtn = document.getElementById(cfg.onId);
                const offBtn = document.getElementById(cfg.offId);

                const applyBtnClass = (btn, cls) => {
                    if (!btn) return;
                    btn.className = cls;
                    if (btn.dataset && btn.dataset.noMicLike === '1') return;
                    btn.classList.add('mic-like');
                };

                if (onBtn) applyBtnClass(onBtn, isOn ? cfg.onActive : cfg.onInactive);
                if (offBtn) applyBtnClass(offBtn, isOn ? cfg.offInactive : cfg.offActive);
            };

            const setDot = (id, on) => {
                const el = document.getElementById(id);
                if (!el) return;
                el.classList.remove('bg-slate-300', 'ring-slate-100', 'bg-emerald-500', 'ring-emerald-100');
                if (on) {
                    el.classList.add('bg-emerald-500', 'ring-emerald-100');
                } else {
                    el.classList.add('bg-slate-300', 'ring-slate-100');
                }
            };

            setDot('st-light1', light1On);
            setDot('st-light2', light2On);
            setDot('st-light3', light3On);
            setDot('st-light1-mini', light1On);
            setDot('st-light2-mini', light2On);
            setDot('st-light3-mini', light3On);
            setDot('st-fan1', toBool(devices.fan1));
            setDot('st-fan2', toBool(devices.fan2));

            // Curtain status: prefer motion/state from /sensors (curtain_motion, curtain_state)
            const curtainState = String(devices.curtain_state ?? '').trim().toUpperCase();
            const cmRaw = (devices.curtain_motion === null || devices.curtain_motion === undefined || devices.curtain_motion === '') ? NaN : Number(devices.curtain_motion);
            const curtainMotion = Number.isFinite(cmRaw) ? cmRaw : null;
            const hasCurtainState = !!curtainState || curtainMotion !== null;

            if (hasCurtainState) {
                const txt = document.getElementById('curtain-status-text');
                const isOpening = curtainState === 'OPENING' || (curtainMotion !== null && curtainMotion > 0);
                const isClosing = curtainState === 'CLOSING' || (curtainMotion !== null && curtainMotion < 0);
                const isStop = curtainState === 'STOP' || (curtainMotion !== null && curtainMotion === 0);

                setDot('st-curtain', isOpening || isClosing);
                if (txt) txt.innerText = isOpening ? 'Đang mở' : (isClosing ? 'Đang đóng' : (isStop ? 'Dừng' : 'Chưa có dữ liệu'));

                // Highlight open/close based on motion direction (stop keeps both neutral)
                const openBtn = document.getElementById('btn-curtain-open');
                const closeBtn = document.getElementById('btn-curtain-close');
                const openInactive = 'py-2.5 rounded-xl bg-indigo-100 text-indigo-700 font-bold hover:bg-indigo-600 hover:text-white transition-all';
                const openActive = 'py-2.5 rounded-xl bg-indigo-600 text-white font-bold transition-all';
                const closeInactive = 'py-2.5 rounded-xl bg-slate-100 text-slate-700 font-bold hover:bg-slate-200 transition-all';
                const closeActive = 'py-2.5 rounded-xl bg-slate-600 text-white font-bold transition-all';

                if (openBtn) {
                    openBtn.className = isOpening ? openActive : openInactive;
                    openBtn.classList.add('mic-like');
                }
                if (closeBtn) {
                    closeBtn.className = isClosing ? closeActive : closeInactive;
                    closeBtn.classList.add('mic-like');
                }
            } else if (Object.prototype.hasOwnProperty.call(devices, 'curtain_open')) {
                setDot('st-curtain', toBool(devices.curtain_open));
                const txt = document.getElementById('curtain-status-text');
                if (txt) txt.innerText = devices.curtain_open ? 'Đang mở' : 'Đang đóng';

                setTogglePair({
                    onId: 'btn-curtain-open',
                    offId: 'btn-curtain-close',
                    onInactive: 'py-2.5 rounded-xl bg-indigo-100 text-indigo-700 font-bold hover:bg-indigo-600 hover:text-white transition-all',
                    onActive: 'py-2.5 rounded-xl bg-indigo-600 text-white font-bold transition-all',
                    offInactive: 'py-2.5 rounded-xl bg-slate-100 text-slate-700 font-bold hover:bg-slate-200 transition-all',
                    offActive: 'py-2.5 rounded-xl bg-slate-600 text-white font-bold transition-all'
                }, toBool(devices.curtain_open));
            }

            // Highlight ON/OFF buttons by current state
            setTogglePair({
                onId: 'btn-fan1-on',
                offId: 'btn-fan1-off',
                onInactive: 'w-full px-5 py-2.5 rounded-xl bg-emerald-100 text-emerald-700 font-bold hover:bg-emerald-600 hover:text-white transition-all',
                onActive: 'w-full px-5 py-2.5 rounded-xl bg-emerald-600 text-white font-bold transition-all',
                offInactive: 'w-full px-5 py-2.5 rounded-xl bg-slate-100 text-slate-600 font-bold hover:bg-slate-200 transition-all',
                offActive: 'w-full px-5 py-2.5 rounded-xl bg-slate-600 text-white font-bold transition-all'
            }, toBool(devices.fan1));

            setTogglePair({
                onId: 'btn-fan2-on',
                offId: 'btn-fan2-off',
                onInactive: 'w-full px-5 py-2.5 rounded-xl bg-emerald-100 text-emerald-700 font-bold hover:bg-emerald-600 hover:text-white transition-all',
                onActive: 'w-full px-5 py-2.5 rounded-xl bg-emerald-600 text-white font-bold transition-all',
                offInactive: 'w-full px-5 py-2.5 rounded-xl bg-slate-100 text-slate-600 font-bold hover:bg-slate-200 transition-all',
                offActive: 'w-full px-5 py-2.5 rounded-xl bg-slate-600 text-white font-bold transition-all'
            }, toBool(devices.fan2));

            setTogglePair({
                onId: 'btn-light1-on',
                offId: 'btn-light1-off',
                onInactive: 'w-full px-5 py-2.5 rounded-xl bg-amber-100 text-amber-600 font-bold hover:bg-amber-500 hover:text-white transition-all shadow-sm',
                onActive: 'w-full px-5 py-2.5 rounded-xl bg-amber-500 text-white font-bold transition-all shadow-sm',
                offInactive: 'w-full px-5 py-2.5 rounded-xl bg-slate-100 text-slate-500 font-bold hover:bg-slate-200 transition-all shadow-sm',
                offActive: 'w-full px-5 py-2.5 rounded-xl bg-slate-600 text-white font-bold transition-all shadow-sm'
            }, light1On);

            setTogglePair({
                onId: 'btn-light2-on',
                offId: 'btn-light2-off',
                onInactive: 'w-full px-5 py-2.5 rounded-xl bg-indigo-100 text-indigo-600 font-bold hover:bg-indigo-600 hover:text-white transition-all shadow-sm',
                onActive: 'w-full px-5 py-2.5 rounded-xl bg-indigo-600 text-white font-bold transition-all shadow-sm',
                offInactive: 'w-full px-5 py-2.5 rounded-xl bg-slate-100 text-slate-500 font-bold hover:bg-slate-200 transition-all shadow-sm',
                offActive: 'w-full px-5 py-2.5 rounded-xl bg-slate-600 text-white font-bold transition-all shadow-sm'
            }, light2On);

            setTogglePair({
                onId: 'btn-light3-on',
                offId: 'btn-light3-off',
                onInactive: 'w-full px-5 py-2.5 rounded-xl bg-cyan-100 text-cyan-700 font-bold hover:bg-cyan-600 hover:text-white transition-all shadow-sm',
                onActive: 'w-full px-5 py-2.5 rounded-xl bg-cyan-600 text-white font-bold transition-all shadow-sm',
                offInactive: 'w-full px-5 py-2.5 rounded-xl bg-slate-100 text-slate-500 font-bold hover:bg-slate-200 transition-all shadow-sm',
                offActive: 'w-full px-5 py-2.5 rounded-xl bg-slate-600 text-white font-bold transition-all shadow-sm'
            }, light3On);

            // lights_all: reflect combined state (on if any floor is on)
            const anyOn = (light1On || light2On || light3On);
            setTogglePair({
                onId: 'btn-lightsall-on',
                offId: 'btn-lightsall-off',
                onInactive: 'w-full h-11 px-5 py-2.5 rounded-xl bg-amber-100 text-amber-700 font-bold hover:bg-amber-500 hover:text-white transition-all shadow-sm',
                onActive: 'w-full h-11 px-5 py-2.5 rounded-xl bg-amber-500 text-white font-bold transition-all shadow-sm',
                offInactive: 'w-full h-11 px-5 py-2.5 rounded-xl bg-slate-100 text-slate-600 font-bold hover:bg-slate-200 transition-all shadow-sm',
                offActive: 'w-full h-11 px-5 py-2.5 rounded-xl bg-slate-600 text-white font-bold transition-all shadow-sm'
            }, anyOn);

            if (Object.prototype.hasOwnProperty.call(devices, 'door_open')) {
                setDot('st-door', toBool(devices.door_open));
                const txt = document.getElementById('door-status-text');
                if (txt) txt.innerText = devices.door_open ? 'Đang mở' : 'Đang đóng';

                setTogglePair({
                    onId: 'btn-door-open',
                    offId: 'btn-door-close',
                    onInactive: 'py-2.5 rounded-xl bg-indigo-100 text-indigo-700 font-bold hover:bg-indigo-600 hover:text-white transition-all',
                    onActive: 'py-2.5 rounded-xl bg-indigo-600 text-white font-bold transition-all',
                    offInactive: 'py-2.5 rounded-xl bg-slate-100 text-slate-600 font-bold hover:bg-slate-200 transition-all',
                    offActive: 'py-2.5 rounded-xl bg-slate-600 text-white font-bold transition-all'
                }, toBool(devices.door_open));
            }

            if (devices.light1_color) setSwatch('sw-light1', devices.light1_color);
            if (devices.light2_color) setSwatch('sw-light2', devices.light2_color);
            if (devices.light3_color) setSwatch('sw-light3', devices.light3_color);
            if (devices.lights_all_color) setSwatch('sw-lights-all', devices.lights_all_color);

            if (allColor === 'RAINBOW') {
                const primary = (devices.light3_color ? String(devices.light3_color).toUpperCase() : 'PURPLE');
                setSwatch('sw-light3', primary);
                setSwatch('sw-light1', primary);
            }
        }

        async function pollSensorsOnce() {
            if (!ESP_IP) return;
            try {
                const data = await apiGetJson('/sensors');

                // --- Sensor alert styling (Gas/Fire) ---
                const clamp01 = (x) => Math.max(0, Math.min(1, Number(x)));
                const _hsla = (h, s, l, a) => `hsla(${Math.round(h)}, ${Math.round(s)}%, ${Math.round(l)}%, ${a})`;

                // Toast cooldown helpers (avoid emergency spam)
                window.__toastCooldown = window.__toastCooldown || {};
                const nowMs = Date.now();
                const canToast = (key, intervalMs) => {
                    const last = Number(window.__toastCooldown[key] || 0);
                    if (!last || (nowMs - last) >= intervalMs) {
                        window.__toastCooldown[key] = nowMs;
                        return true;
                    }
                    return false;
                };

                function applyAlertStyleToTile(tileId, iconId, hue, strength01) {
                    const tile = document.getElementById(tileId);
                    const icon = document.getElementById(iconId);
                    const t = clamp01(strength01);
                    if (!tile || !icon) return;

                    // Subtle background on tile, stronger on icon
                    tile.style.backgroundColor = _hsla(hue, 95, 96 - t * 14, 0.75);
                    tile.style.borderColor = _hsla(hue, 85, 86 - t * 10, 0.55);
                    tile.style.boxShadow = t > 0.01 ? `0 12px 40px -20px ${_hsla(hue, 95, 55, 0.55)}` : '';

                    icon.style.backgroundColor = _hsla(hue, 95, 92 - t * 18, 0.85);
                    icon.style.borderColor = _hsla(hue, 85, 80 - t * 12, 0.7);
                    icon.style.color = _hsla(hue, 85, 38, 1);
                }

                // Gas: >250 starts yellow, >=400 red
                const gasVal = (data && data.gas_value !== null && data.gas_value !== undefined) ? Number(data.gas_value) : NaN;
                if (Number.isFinite(gasVal)) {
                    if (gasVal <= 250) {
                        applyAlertStyleToTile('card-gas', 'icon-gas', 140, 0);
                    } else if (gasVal >= 400) {
                        applyAlertStyleToTile('card-gas', 'icon-gas', 0, 1);
                    } else {
                        const t = (gasVal - 250) / (400 - 250);
                        const hue = 55 * (1 - clamp01(t)); // 55 (yellow) -> 0 (red)
                        applyAlertStyleToTile('card-gas', 'icon-gas', hue, clamp01(t));
                    }
                }

                // Fire: when FIRE, gradually go yellow -> red
                window.__fireAlertStartAt = window.__fireAlertStartAt || null;
                const fireStatus = (data && data.fire_status) ? String(data.fire_status).trim().toUpperCase() : '';
                const isFire = fireStatus === 'FIRE';
                if (isFire) {
                    if (!window.__fireAlertStartAt) window.__fireAlertStartAt = Date.now();
                    const t = clamp01((Date.now() - window.__fireAlertStartAt) / 2000);
                    const hue = 55 * (1 - t);
                    applyAlertStyleToTile('card-fire', 'icon-fire', hue, t);
                } else {
                    window.__fireAlertStartAt = null;
                    applyAlertStyleToTile('card-fire', 'icon-fire', 140, 0);
                }

                // --- Targeted notifications (avoid sensor spam) ---
                // 1) Door open/close
                window.__lastDoorOpen = (typeof window.__lastDoorOpen === 'boolean') ? window.__lastDoorOpen : null;
                const doorOpen = (data && data.devices && Object.prototype.hasOwnProperty.call(data.devices, 'door_open')) ? !!data.devices.door_open : null;
                const doorLastMethodNow = (data && data.devices && data.devices.door_last_method != null) ? String(data.devices.door_last_method).trim().toUpperCase() : '';
                if (typeof doorOpen === 'boolean' && window.__lastDoorOpen !== null && doorOpen !== window.__lastDoorOpen) {
                    showToast(doorOpen ? 'Cửa: ĐANG MỞ' : 'Cửa: ĐÃ ĐÓNG', 'success');

                    // If the door was opened using emergency PIN, notify explicitly
                    if (doorOpen === true && doorLastMethodNow === 'EMERGENCY_PIN') {
                        if (canToast('door_emergency_pin_open', 30000)) {
                            showToast('KHẨN CẤP: Cửa mở bằng mật khẩu khẩn cấp', 'error');
                        }
                    }
                }
                if (typeof doorOpen === 'boolean') window.__lastDoorOpen = doorOpen;

                // 1.1) Emergency PIN configured status (notify on change)
                window.__lastEmergencyPinSet = (typeof window.__lastEmergencyPinSet === 'boolean') ? window.__lastEmergencyPinSet : null;
                const emergencyPinSet = (data && data.devices && Object.prototype.hasOwnProperty.call(data.devices, 'door_emergency_pin_set'))
                    ? !!data.devices.door_emergency_pin_set
                    : null;
                if (typeof emergencyPinSet === 'boolean' && window.__lastEmergencyPinSet !== null && emergencyPinSet !== window.__lastEmergencyPinSet) {
                    showToast(emergencyPinSet ? 'Mật khẩu khẩn cấp: ĐÃ CÀI ĐẶT' : 'Mật khẩu khẩn cấp: CHƯA CÀI ĐẶT', 'success');
                }
                if (typeof emergencyPinSet === 'boolean') window.__lastEmergencyPinSet = emergencyPinSet;

                // 2) Fire alert
                window.__lastFireStatus = (typeof window.__lastFireStatus === 'string') ? window.__lastFireStatus : '';
                if (fireStatus && fireStatus !== window.__lastFireStatus) {
                    if (fireStatus === 'FIRE') {
                        // Immediate alert on transition
                        showToast('CẢNH BÁO KHẨN CẤP: PHÁT HIỆN LỬA (FIRE)', 'error');
                        window.__toastCooldown['fire_remind'] = nowMs;
                    }
                    window.__lastFireStatus = fireStatus;
                }

                // Reminder while FIRE persists (every 30s)
                if (fireStatus === 'FIRE') {
                    if (canToast('fire_remind', 30000)) {
                        showToast('NHẮC LẠI: Đang có BÁO CHÁY (FIRE)', 'error');
                    }
                }

                // 3) Gas thresholds (only when crossing zones)
                window.__lastGasZone = window.__lastGasZone || 'unknown';
                if (Number.isFinite(gasVal)) {
                    const zone = gasVal >= 400 ? 'danger' : (gasVal > 250 ? 'warn' : 'safe');
                    if (window.__lastGasZone !== 'unknown' && zone !== window.__lastGasZone) {
                        if (zone === 'warn') {
                            showToast('Cảnh báo: Gas tăng cao (>250): ' + gasVal, 'error');
                            window.__toastCooldown['gas_warn_remind'] = nowMs;
                        } else if (zone === 'danger') {
                            showToast('KHẨN CẤP: Gas rất cao (>=400): ' + gasVal, 'error');
                            window.__toastCooldown['gas_danger_remind'] = nowMs;
                        }
                        // Don't toast when returning to safe to avoid noise
                    }
                    window.__lastGasZone = zone;

                    // Reminder while gas stays high (every 45s)
                    if (zone === 'danger') {
                        if (canToast('gas_danger_remind', 45000)) {
                            showToast('NHẮC LẠI: Gas đang rất cao (>=400): ' + gasVal, 'error');
                        }
                    } else if (zone === 'warn') {
                        if (canToast('gas_warn_remind', 45000)) {
                            showToast('NHẮC LẠI: Gas đang cao (>250): ' + gasVal, 'error');
                        }
                    }
                }

                // 4) Presence in front of door (based on ultrasonic)
                // Hysteresis to avoid flicker: present <= 70cm, clear >= 90cm
                window.__doorPresence = (typeof window.__doorPresence === 'boolean') ? window.__doorPresence : null;
                const usVal = (data && data.ultrasonic_cm !== null && data.ultrasonic_cm !== undefined) ? Number(data.ultrasonic_cm) : NaN;
                if (Number.isFinite(usVal) && usVal > 0) {
                    let present = window.__doorPresence;
                    if (present === null) {
                        present = usVal <= 70;
                    } else {
                        if (present === false && usVal <= 70) present = true;
                        if (present === true && usVal >= 90) present = false;
                    }

                    // Optional: only care when door is closed (simple heuristic)
                    const doorIsClosed = (doorOpen === false);
                    if (doorIsClosed) {
                        if (window.__doorPresence !== null && present !== window.__doorPresence && present === true) {
                            showToast('Phát hiện người ở trước cửa', 'success');
                            window.__toastCooldown['door_presence_remind'] = nowMs;
                        }
                        if (present === true) {
                            // Reminder every 20s while someone stays in front
                            if (canToast('door_presence_remind', 20000)) {
                                showToast('NHẮC LẠI: Có người đang ở trước cửa', 'success');
                            }
                        }
                    }

                    window.__doorPresence = present;
                }

                const setText = (id, text) => {
                    const el = document.getElementById(id);
                    if (el) el.innerText = text;
                };

                if (data.temperature_c !== null && data.temperature_c !== undefined) {
                    setText('val-temp', `${Number(data.temperature_c).toFixed(1)}°C`);
                }
                if (data.humidity_percent !== null && data.humidity_percent !== undefined) {
                    setText('val-hum', `${Number(data.humidity_percent).toFixed(0)}%`);
                }
                if (data.gas_value !== null && data.gas_value !== undefined) {
                    setText('val-gas', String(data.gas_value));
                }
                if (data.fire_status) {
                    setText('val-fire', String(data.fire_status));
                }

                if (data.ultrasonic_cm !== null && data.ultrasonic_cm !== undefined) {
                    const n = Number(data.ultrasonic_cm);
                    if (Number.isFinite(n)) setText('val-ultrasonic', `${n.toFixed(0)} cm`);
                    else setText('val-ultrasonic', String(data.ultrasonic_cm));
                }
                if (data.ldr_value !== null && data.ldr_value !== undefined) {
                    setText('val-ldr', String(data.ldr_value));
                }

                // Merge top-level curtain state into devices so UI can read either schema.
                const mergedDevices = Object.assign({}, (data && data.devices && typeof data.devices === 'object') ? data.devices : {});
                if (data && Object.prototype.hasOwnProperty.call(data, 'curtain_motion')) mergedDevices.curtain_motion = data.curtain_motion;
                if (data && Object.prototype.hasOwnProperty.call(data, 'curtain_state')) mergedDevices.curtain_state = data.curtain_state;
                applyDeviceStates(mergedDevices);

                // Command verification (success/failure)
                if (pendingCommand && pendingCommand.expected) {
                    const ok = _matchesExpected(data.devices, pendingCommand.expected);
                    const ageMs = Date.now() - (pendingCommand.at || 0);
                    if (ok) {
                        showToast('Thiết bị đã đổi trạng thái thành công', 'success');
                        pendingCommand = null;
                    } else if (ageMs > 3500) {
                        const cmd = String(pendingCommand.cmd || '');
                        const isColorCmd = cmd.includes('_COLOR_');

                        // If the device payload is partial/missing keys, wait a bit longer.
                        const expKeys = Object.keys(pendingCommand.expected || {});
                        const missingKeys = expKeys.filter(k => !data.devices || !Object.prototype.hasOwnProperty.call(data.devices, k));
                        if (missingKeys.length > 0 && ageMs <= 8000) {
                            // Keep waiting (no toast)
                        } else {
                            // For color commands (including RAINBOW), don't show scary failure toasts.
                            // Color changes can be applied while boolean relay state is late to report.
                            if (!isColorCmd) {
                                showToast('Thiết bị chưa đổi trạng thái (có thể thất bại)', 'error');
                            }
                            pendingCommand = null;
                        }
                    }
                }

                document.getElementById('connection-msg').innerText = "System Online";
                document.getElementById('status-dot').className = "w-2.5 h-2.5 rounded-full bg-emerald-500 ring-4 ring-emerald-100";
            } catch (e) {
                console.warn('[SENSORS] poll failed', e);
                const base = String(ESP_IP || '').replace(/^https?:\/\//i, '');
                document.getElementById('connection-msg').innerText = base ? ("Disconnected: " + base) : "Disconnected";
                document.getElementById('status-dot').className = "w-2.5 h-2.5 rounded-full bg-rose-500 ring-4 ring-rose-100";
            }
        }

        // 4. AI Process
        let isProcessingAi = false;

        function getHomeState() {
            const readText = (id) => {
                const el = document.getElementById(id);
                return el ? String(el.innerText || '').trim() : '';
            };

            const parseNumberOrNull = (s) => {
                const cleaned = String(s || '').replace(/[^0-9.\-]/g, '');
                if (!cleaned) return null;
                const n = Number(cleaned);
                return Number.isFinite(n) ? n : null;
            };

            const tempText = readText('val-temp');
            const humText = readText('val-hum');
            const gasText = readText('val-gas');
            const fireText = readText('val-fire');

            return {
                temperature_c: parseNumberOrNull(tempText),
                humidity_percent: parseNumberOrNull(humText),
                gas_value: parseNumberOrNull(gasText) ?? (gasText || null),
                fire_status: fireText || null,
                updated_at: new Date().toISOString()
            };
        }

        async function processWithOllama(userText) {
            if (isProcessingAi) return;
            isProcessingAi = true;
            const aiBox = document.getElementById('ai-response-box');
            const aiText = document.getElementById('ai-text');

            function foldVi(s) {
                try {
                    return String(s || '')
                        .toLowerCase()
                        .normalize('NFD')
                        .replace(/[\u0300-\u036f]/g, '')
                        .replace(/đ/g, 'd')
                        .replace(/[^a-z0-9\s]/g, ' ')
                        .replace(/\s+/g, ' ')
                        .trim();
                } catch (e) {
                    return String(s || '').toLowerCase().trim();
                }
            }

            async function tryHandleCurtainDirect(text) {
                const f = foldVi(text);
                if (!f) return false;

                const isAskStatus = f.includes('rem') && (
                    f.includes('trang thai')
                    || f.includes('dang o trang thai')
                    || f.includes('dang the nao')
                    || f === 'trang thai rem'
                );

                const wantOpen = f.includes('mo rem') || f.includes('bat rem');
                const wantClose = f.includes('dong rem');
                const wantStop = f.includes('dung rem') || f.includes('stop rem');

                if (!isAskStatus && !wantOpen && !wantClose && !wantStop) return false;

                toggleVoicePanel(true);
                toggleAiResponse(true);

                if (isAskStatus) {
                    aiText.innerText = 'Đang kiểm tra trạng thái rèm...';
                    try {
                        const sensors = await apiGetJson('/sensors');
                        const msg = getCurtainStatusTextFromSensors(sensors);
                        aiText.innerText = msg;
                        await speak(msg);
                    } catch (e) {
                        const msg = 'Lỗi đọc trạng thái rèm: ' + (e && e.message ? e.message : 'mất kết nối ESP32');
                        aiText.innerText = msg;
                        showToast(msg + '. Vui lòng kiểm tra kết nối ESP32.', 'error');
                    }
                    return true;
                }

                aiText.innerText = 'Đang gửi lệnh rèm...';
                try {
                    if (wantOpen) await sendCurtainAction('open');
                    if (wantClose) await sendCurtainAction('close');
                    if (wantStop) await sendCurtainAction('stop');

                    const msg = wantOpen ? 'Đã gửi lệnh MỞ' : (wantClose ? 'Đã gửi lệnh ĐÓNG' : 'Đã gửi lệnh DỪNG');
                    aiText.innerText = msg;
                    await speak(msg);
                } catch (e) {
                    const msg = 'Lỗi điều khiển rèm: ' + (e && e.message ? e.message : 'mất kết nối ESP32');
                    aiText.innerText = msg;
                    showToast(msg + '. Vui lòng kiểm tra kết nối ESP32.', 'error');
                }
                return true;
            }

            async function sendCommandsSequentially(cmds) {
                const list = Array.isArray(cmds) ? cmds : [];
                for (const raw of list) {
                    const c = String(raw || '').trim();
                    if (!c) continue;
                    await sendCommand(c);
                    // small gap so ESP32/bridge can digest multiple commands
                    await new Promise(r => setTimeout(r, 250));
                }
            }

            toggleVoicePanel(true);
            toggleAiResponse(true);
            aiText.innerText = "Thinking...";

            try {
                const handled = await tryHandleCurtainDirect(userText);
                if (handled) return;
            } catch (e) {
                // If direct handling fails unexpectedly, fall back to AI path.
            }

            try {
                const geminiKey = localStorage.getItem('gemini_api_key') || '';

                const doRequest = async () => {
                    return fetch(`${BASE_URL}/AiController/process`, {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({
                            text: userText,
                            home_state: getHomeState(),
                            gemini_api_key: geminiKey
                        })
                    });
                };

                let response = await doRequest();
                // One retry on rate limit
                if (response && response.status === 429) {
                    await new Promise(r => setTimeout(r, 1200));
                    response = await doRequest();
                }

                const rawText = await response.text();
                let data;
                try {
                    data = rawText ? JSON.parse(rawText) : null;
                } catch (e) {
                    data = null;
                }

                if (!response.ok) {
                    const msg = (data && (data.message || data.error)) ? (data.message || data.error) : `HTTP ${response.status}`;
                    showToast('AI lỗi: ' + msg, 'error');
                    aiText.innerText = 'AI lỗi: ' + msg;
                    setAiSpeaking(false);
                    return;
                }
                
                if (data && data.status === 'success') {
                    let aiData;
                    try {
                        aiData = (data && typeof data.response === 'string') ? JSON.parse(data.response) : (data ? data.response : null);
                    } catch (e) {
                        console.warn('[AI] invalid data.response JSON', e, { rawText, data });
                        const msg = 'AI trả về dữ liệu không hợp lệ';
                        showToast(msg, 'error');
                        aiText.innerText = msg;
                        setAiSpeaking(false);
                        return;
                    }

                    if (!aiData || typeof aiData !== 'object') {
                        const msg = 'AI trả về dữ liệu không hợp lệ';
                        showToast(msg, 'error');
                        aiText.innerText = msg;
                        setAiSpeaking(false);
                        return;
                    }

                    console.log('[AI] parsed response:', aiData);
                    const replyText = (aiData.reply != null) ? String(aiData.reply) : '';
                    typeTextToElement(aiText, replyText || '');
                    if (replyText) await speak(replyText);

                    if (Array.isArray(aiData.commands) && aiData.commands.length) {
                        showToast('AI gửi ' + aiData.commands.length + ' lệnh', 'success');
                        await sendCommandsSequentially(aiData.commands);
                    } else if (aiData.command && aiData.command !== 'null') {
                        // Backward-compatible: allow semicolon-separated commands
                        const parts = String(aiData.command).split(';').map(s => s.trim()).filter(Boolean);
                        if (parts.length > 1) {
                            showToast('AI gửi ' + parts.length + ' lệnh', 'success');
                            await sendCommandsSequentially(parts);
                        } else {
                            await sendCommand(aiData.command);
                        }
                    } else {
                        // If AI returned a normal reply (Q&A), don't treat as an error.
                        if (!replyText || !String(replyText).trim()) {
                            showToast('AI chưa nhận ra lệnh điều khiển', 'error');
                        }
                    }
                } else {
                    const msg = (data && data.message) ? String(data.message) : 'Unknown error';
                    aiText.innerText = "Error: " + msg;
                    setAiSpeaking(false);
                }

            } catch (error) {
                console.error(error);
                const msg = (error && error.message) ? String(error.message) : 'Connection failed';
                aiText.innerText = "AI lỗi kết nối: " + msg;
                showToast('AI lỗi kết nối: ' + msg, 'error');
                setAiSpeaking(false);
            } finally {
                isProcessingAi = false;
            }
        }

        // 5. Speech Recognition (NO WAKE WORD)
        if ('webkitSpeechRecognition' in window || 'SpeechRecognition' in window) {
            const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
            recognition = new SpeechRecognition();
            recognition.lang = 'vi-VN';
            // Không dùng continuous để tránh nghe tạp âm liên tục
            recognition.continuous = false; 
            recognition.interimResults = false;

            recognition.onstart = () => {
                isListening = true;
                setMicButtonState(true);
            };

            recognition.onend = () => {
                isListening = false;
                setMicButtonState(false);
            };

            recognition.onresult = (event) => {
                const text = event.results[0][0].transcript;
                console.log("Recognized:", text);
                if (recognitionTarget === 'chat') {
                    const inp = document.getElementById('chat-input');
                    if (inp) {
                        inp.value = String(text || '').trim();
                        inp.focus();
                    }
                    recognitionTarget = 'voice';
                    return;
                }
                // Xử lý luôn không cần kiểm tra wake word
                processWithOllama(text);
            };

        } else {
            alert("Browser not supported");
        }

        function toggleVoice() {
            toggleVoicePanel(true);
            setVoicePanelMode('voice');
            recognitionTarget = 'voice';
            if(isListening) recognition.stop();
            else recognition.start();
        }

        function toggleSettings() { 
            document.getElementById('settings-modal').classList.toggle('hidden'); 
        }
        
        function saveSettings() {
            let ip = document.getElementById('esp-ip-input').value;
            if(ip && !ip.startsWith('http')) ip = 'http://' + ip;
            if(ip) {
                localStorage.setItem('esp_ip', ip);
                ESP_IP = normalizeBaseUrl(ip);
            }

            const gscriptUrl = document.getElementById('gscript-url-input')?.value?.trim();
            if (gscriptUrl) {
                localStorage.setItem('gscript_url', gscriptUrl);
            }

            const apiKey = document.getElementById('gemini-key-input').value?.trim();
            if (apiKey) {
                localStorage.setItem('gemini_api_key', apiKey);
                syncGeminiApiKey(apiKey);
            }
            
            const ttsKey = document.getElementById('gcp-tts-key-input').value?.trim();
            if (ttsKey) {
                localStorage.setItem('gcp_tts_api_key', ttsKey);
                syncGcpTtsApiKey(ttsKey);
            }

            toggleSettings();
        }

        // Init
        document.getElementById('esp-ip-input').value = ESP_IP;
        document.getElementById('gscript-url-input').value = localStorage.getItem('gscript_url') || '';

        // Apply mic-like effect to all buttons (UI-only, no behavior change)
        try {
            document.querySelectorAll('button').forEach((btn) => {
                if (btn && btn.dataset && btn.dataset.noMicLike === '1') return;
                btn.classList.add('mic-like');
            });
        } catch (e) {}
        const savedGeminiKey = localStorage.getItem('gemini_api_key') || '';
        if (savedGeminiKey) {
            document.getElementById('gemini-key-input').value = savedGeminiKey;
            syncGeminiApiKey(savedGeminiKey);
        }
        const savedTtsKey = localStorage.getItem('gcp_tts_api_key') || '';
        if (savedTtsKey) {
            document.getElementById('gcp-tts-key-input').value = savedTtsKey;
            syncGcpTtsApiKey(savedTtsKey);
        }

        // No theme selector anymore (dark-only). Greeting updates by real time.
        updateGreetingByTime();

        // Voice panel remembers last state
        const vpOpen = localStorage.getItem('voice_panel_open');
        if (vpOpen === '1') toggleVoicePanel(true);

        const vpMode = localStorage.getItem('voice_panel_mode') || 'voice';
        setVoicePanelMode(vpMode);

        const chatInput = document.getElementById('chat-input');
        if (chatInput) {
            chatInput.addEventListener('keydown', (e) => {
                if (e.key === 'Enter') {
                    e.preventDefault();
                    submitChat();
                }
            });
        }

        // RFID manual refresh button
        try {
            const btnRfidRefresh = document.getElementById('btn-rfid-refresh');
            if (btnRfidRefresh) {
                btnRfidRefresh.addEventListener('click', (e) => {
                    try { e.preventDefault(); } catch (_) {}
                    refreshRfid();
                });
            }
        } catch (e) {}

        pollSensorsOnce();
        setInterval(pollSensorsOnce, 2000);

        refreshRfid();
        setInterval(refreshRfid, 4000);
    </script>
</body>
</html>