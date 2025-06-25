#!/usr/bin/perl

print "Content-type: text/html; charset=utf-8\r\n\r\n";

# Get current time for high score
my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
$year += 1900;
$mon += 1;
my $timestamp = sprintf("%04d-%02d-%02d %02d:%02d:%02d", $year, $mon, $mday, $hour, $min, $sec);

# Generate random "score" based on time
my $score = ($hour * 1000) + ($min * 10) + $sec;

print <<"END";
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🕹️ RETRO ARCADE - HELLO WORLD</title>
    <style>
        \@import url('https://fonts.googleapis.com/css2?family=Press+Start+2P&display=swap');

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Press Start 2P', monospace;
            background: linear-gradient(135deg, #000428 0%, #004e92 100%);
            background-size: 400% 400%;
            animation: retro-gradient 8s ease-in-out infinite;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
            overflow-x: hidden;
            position: relative;
            color: #00FF00;
        }

        \@keyframes retro-gradient {
            0%, 100% { background-position: 0% 50%; }
            50% { background-position: 100% 50%; }
        }

        /* Scanlines effect */
        body::before {
            content: '';
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: repeating-linear-gradient(
                0deg,
                transparent,
                transparent 2px,
                rgba(0, 255, 0, 0.03) 2px,
                rgba(0, 255, 0, 0.03) 4px
            );
            pointer-events: none;
            z-index: 1000;
        }

        /* Floating pixels */
        .pixel {
            position: absolute;
            width: 4px;
            height: 4px;
            background: #00FF00;
            animation: float-pixels 10s linear infinite;
            pointer-events: none;
        }

        \@keyframes float-pixels {
            0% { transform: translateY(100vh) translateX(0); opacity: 1; }
            100% { transform: translateY(-100px) translateX(50px); opacity: 0; }
        }

        .arcade-container {
            max-width: 800px;
            width: 100%;
            background: linear-gradient(135deg, #1a1a1a 0%, #2d2d2d 100%);
            border-radius: 20px;
            padding: 40px;
            text-align: center;
            box-shadow:
                0 0 50px rgba(0, 255, 0, 0.3),
                inset 0 0 50px rgba(0, 0, 0, 0.5);
            border: 4px solid #00FF00;
            position: relative;
            overflow: hidden;
        }

        .arcade-container::before {
            content: '';
            position: absolute;
            top: -50%;
            left: -50%;
            width: 200%;
            height: 200%;
            background: conic-gradient(from 0deg, transparent, #00FF00, transparent);
            animation: arcade-spin 12s linear infinite;
            pointer-events: none;
            opacity: 0.1;
        }

        \@keyframes arcade-spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }

        .screen-border {
            border: 6px solid #333;
            border-radius: 15px;
            padding: 30px;
            background: #000;
            position: relative;
            z-index: 1;
        }

        .game-title {
            font-size: 2rem;
            color: #FF00FF;
            margin: 20px 0;
            text-shadow:
                0 0 10px #FF00FF,
                0 0 20px #FF00FF,
                0 0 30px #FF00FF;
            animation: neon-flicker 2s ease-in-out infinite;
            position: relative;
            z-index: 1;
        }

        \@keyframes neon-flicker {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.8; }
        }

        .hello-display {
            font-size: 3rem;
            color: #00FF00;
            margin: 30px 0;
            animation: hello-bounce 1.5s ease-in-out infinite;
            text-shadow:
                0 0 10px #00FF00,
                0 0 20px #00FF00;
            position: relative;
            z-index: 1;
            line-height: 1.2;
        }

        \@keyframes hello-bounce {
            0%, 100% { transform: translateY(0) scale(1); }
            50% { transform: translateY(-10px) scale(1.05); }
        }

        .world-display {
            font-size: 3rem;
            color: #FFFF00;
            margin: 30px 0;
            animation: world-pulse 2s ease-in-out infinite;
            text-shadow:
                0 0 10px #FFFF00,
                0 0 20px #FFFF00;
            position: relative;
            z-index: 1;
        }

        \@keyframes world-pulse {
            0%, 100% { transform: scale(1); }
            50% { transform: scale(1.1); }
        }

        .score-display {
            background: linear-gradient(135deg, #333 0%, #111 100%);
            border: 3px solid #00FFFF;
            border-radius: 10px;
            padding: 20px;
            margin: 25px 0;
            position: relative;
            z-index: 1;
        }

        .score-label {
            color: #00FFFF;
            font-size: 0.8rem;
            margin-bottom: 10px;
            text-shadow: 0 0 10px #00FFFF;
        }

        .score-value {
            color: #FFFF00;
            font-size: 1.5rem;
            text-shadow: 0 0 15px #FFFF00;
            animation: score-glow 1s ease-in-out infinite;
        }

        \@keyframes score-glow {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.7; }
        }

        .game-stats {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
            gap: 15px;
            margin: 25px 0;
            position: relative;
            z-index: 1;
        }

        .stat-box {
            background: #1a1a1a;
            border: 2px solid #FF6600;
            border-radius: 8px;
            padding: 15px;
            transition: all 0.3s ease;
        }

        .stat-box:hover {
            border-color: #FFFF00;
            transform: scale(1.05);
            box-shadow: 0 0 20px rgba(255, 255, 0, 0.3);
        }

        .stat-label {
            color: #FF6600;
            font-size: 0.6rem;
            margin-bottom: 8px;
        }

        .stat-value {
            color: #FFFF00;
            font-size: 0.8rem;
        }

        .controls {
            background: #2a2a2a;
            border: 3px solid #00FF00;
            border-radius: 10px;
            padding: 20px;
            margin: 25px 0;
            position: relative;
            z-index: 1;
        }

        .control-buttons {
            display: flex;
            gap: 15px;
            justify-content: center;
            flex-wrap: wrap;
            margin: 20px 0;
        }

        .btn {
            padding: 12px 25px;
            border: 3px solid;
            border-radius: 8px;
            font-size: 0.8rem;
            font-weight: bold;
            cursor: pointer;
            text-decoration: none;
            display: inline-block;
            transition: all 0.3s ease;
            font-family: inherit;
            background: #000;
            text-transform: uppercase;
        }

        .btn-start {
            border-color: #00FF00;
            color: #00FF00;
        }

        .btn-start:hover {
            background: #00FF00;
            color: #000;
            box-shadow: 0 0 20px #00FF00;
        }

        .btn-select {
            border-color: #FF00FF;
            color: #FF00FF;
        }

        .btn-select:hover {
            background: #FF00FF;
            color: #000;
            box-shadow: 0 0 20px #FF00FF;
        }

        .btn-home {
            border-color: #FFFF00;
            color: #FFFF00;
        }

        .btn-home:hover {
            background: #FFFF00;
            color: #000;
            box-shadow: 0 0 20px #FFFF00;
        }

        .game-info {
            color: #888;
            font-size: 0.6rem;
            margin-top: 20px;
            position: relative;
            z-index: 1;
            line-height: 1.4;
        }

        /* Responsive design */
        \@media (max-width: 768px) {
            .arcade-container {
                padding: 20px;
                margin: 10px;
            }

            .game-title {
                font-size: 1.2rem;
            }

            .hello-display, .world-display {
                font-size: 1.8rem;
            }

            .control-buttons {
                flex-direction: column;
                align-items: center;
            }

            .btn {
                width: 100%;
                max-width: 200px;
            }
        }
    </style>
</head>
<body>
    <!-- Floating pixels -->
    <div class="pixel" style="left: 10%; animation-delay: 0s;"></div>
    <div class="pixel" style="left: 30%; animation-delay: 1s;"></div>
    <div class="pixel" style="left: 60%; animation-delay: 2s;"></div>
    <div class="pixel" style="left: 85%; animation-delay: 3s;"></div>

    <div class="arcade-container">
        <div class="screen-border">
            <div class="game-title">
                🕹️ RETRO ARCADE SYSTEM 🕹️
            </div>

            <div class="hello-display">
                HELLO
            </div>

            <div class="world-display">
                WORLD!
            </div>

            <div class="score-display">
                <div class="score-label">HIGH SCORE</div>
                <div class="score-value">$score</div>
            </div>

            <div class="game-stats">
                <div class="stat-box">
                    <div class="stat-label">LEVEL</div>
                    <div class="stat-value">PERL</div>
                </div>
                <div class="stat-box">
                    <div class="stat-label">LIVES</div>
                    <div class="stat-value">∞</div>
                </div>
                <div class="stat-box">
                    <div class="stat-label">POWER</div>
                    <div class="stat-value">MAX</div>
                </div>
                <div class="stat-box">
                    <div class="stat-label">STATUS</div>
                    <div class="stat-value">READY</div>
                </div>
            </div>

            <div class="controls">
                <div style="color: #00FFFF; font-size: 0.8rem; margin-bottom: 15px;">
                    ↑ CONTROLS ↓
                </div>

                <div class="control-buttons">
                    <a href="/cgi-bin/helloWorld.pl" class="btn btn-start">
                        🎮 RESTART
                    </a>
                    <a href="/web/playground.html" class="btn btn-select">
                        🔙 SELECT
                    </a>
                    <a href="/" class="btn btn-home">
                        🏠 HOME
                    </a>
                </div>
            </div>

            <div class="game-info">
                SYSTEM: PERL v5.x • RETRO MODE: ON<br>
                TIMESTAMP: $timestamp<br>
                PRESS ANY BUTTON TO CONTINUE...
            </div>
        </div>
    </div>

    <script>
        // Create pixel effects
        function createPixelBurst() {
            for (let i = 0; i < 10; i++) {
                setTimeout(() => {
                    const pixel = document.createElement('div');
                    pixel.style.cssText = `
                        position: fixed;
                        left: \${Math.random() * window.innerWidth}px;
                        top: \${Math.random() * window.innerHeight}px;
                        width: \${2 + Math.random() * 4}px;
                        height: \${2 + Math.random() * 4}px;
                        background: \${['#00FF00', '#FF00FF', '#FFFF00', '#00FFFF'][Math.floor(Math.random() * 4)]};
                        pointer-events: none;
                        z-index: 1000;
                        animation: pixel-explosion 2s ease-out forwards;
                    `;

                    document.body.appendChild(pixel);
                    setTimeout(() => pixel.remove(), 2000);
                }, i * 50);
            }
        }

        // Retro beep sounds (using Web Audio API)
        function playRetroBeep() {
            if ('AudioContext' in window || 'webkitAudioContext' in window) {
                const audioContext = new (window.AudioContext || window.webkitAudioContext)();
                const oscillator = audioContext.createOscillator();
                const gainNode = audioContext.createGain();

                oscillator.connect(gainNode);
                gainNode.connect(audioContext.destination);

                oscillator.frequency.setValueAtTime(800, audioContext.currentTime);
                gainNode.gain.setValueAtTime(0.1, audioContext.currentTime);
                gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.1);

                oscillator.start(audioContext.currentTime);
                oscillator.stop(audioContext.currentTime + 0.1);
            }
        }

        // Add button sound effects
        document.querySelectorAll('.btn').forEach(btn => {
            btn.addEventListener('mouseenter', playRetroBeep);
            btn.addEventListener('click', () => {
                createPixelBurst();
                playRetroBeep();
            });
        });

        // Periodic pixel effects
        setInterval(createPixelBurst, 3000);

        // Add CSS animations
        const style = document.createElement('style');
        style.textContent = `
            \@keyframes pixel-explosion {
                0% { opacity: 1; transform: scale(1) rotate(0deg); }
                100% { opacity: 0; transform: scale(3) rotate(360deg); }
            }
        `;
        document.head.appendChild(style);

        // Initialize retro system
        document.addEventListener('DOMContentLoaded', function() {
            console.log('🕹️ RETRO ARCADE SYSTEM INITIALIZED');
            console.log('PERL ENGINE: READY');
            console.log('GRAPHICS: 8-BIT MODE');
            console.log('SOUND: CHIPTUNE ENABLED');

            // Welcome beep sequence
            setTimeout(() => {
                playRetroBeep();
                setTimeout(() => {
                    playRetroBeep();
                    setTimeout(playRetroBeep, 200);
                }, 200);
            }, 500);
        });
    </script>
</body>
</html>
END