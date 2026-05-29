#ifndef SIMULATOR
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Console.h"

static WebServer server(80);
static Console* g_console = nullptr;

const char* html_page = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Klick32 Dashboard</title>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;500;700&display=swap');
        
        :root {
            --bg: #0f172a;
            --card-bg: rgba(30, 41, 59, 0.7);
            --text: #f8fafc;
            --accent: #3b82f6;
            --accent-hover: #60a5fa;
            --glass-border: rgba(255, 255, 255, 0.1);
        }

        * { box-sizing: border-box; margin: 0; padding: 0; }
        
        body {
            font-family: 'Inter', sans-serif;
            background: linear-gradient(135deg, #0f172a 0%, #1e1b4b 100%);
            color: var(--text);
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            padding: 2rem;
        }

        h1 {
            font-size: 2.5rem;
            font-weight: 700;
            margin-bottom: 2rem;
            text-align: center;
            background: linear-gradient(to right, #38bdf8, #818cf8);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }

        .container {
            width: 100%;
            max-width: 800px;
            display: grid;
            grid-template-columns: 1fr;
            gap: 1.5rem;
        }

        @media (min-width: 768px) {
            .container { grid-template-columns: 1fr 1fr; }
        }

        .card {
            background: var(--card-bg);
            backdrop-filter: blur(16px);
            -webkit-backdrop-filter: blur(16px);
            border: 1px solid var(--glass-border);
            border-radius: 1rem;
            padding: 2rem;
            box-shadow: 0 10px 25px -5px rgba(0, 0, 0, 0.5), 0 8px 10px -6px rgba(0, 0, 0, 0.5);
            transition: transform 0.3s ease, box-shadow 0.3s ease;
        }

        .card:hover {
            transform: translateY(-5px);
            box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.5), 0 8px 10px -6px rgba(0, 0, 0, 0.5);
        }

        .card h2 {
            font-size: 1.25rem;
            font-weight: 500;
            margin-bottom: 1rem;
            border-bottom: 1px solid var(--glass-border);
            padding-bottom: 0.5rem;
            color: #94a3b8;
        }

        .metric {
            display: flex;
            justify-content: space-between;
            margin-bottom: 0.75rem;
            font-size: 1rem;
        }
        
        .metric-value {
            font-weight: 700;
            color: #38bdf8;
        }

        .btn {
            display: inline-block;
            width: 100%;
            padding: 0.75rem 1rem;
            margin-top: 1rem;
            background: rgba(56, 189, 248, 0.1);
            border: 1px solid var(--accent);
            color: var(--accent);
            border-radius: 0.5rem;
            font-weight: 500;
            text-align: center;
            text-decoration: none;
            transition: all 0.2s ease;
            cursor: pointer;
        }

        .btn:hover {
            background: var(--accent);
            color: #fff;
        }
        
        .progress-bar-container {
            width: 100%;
            height: 10px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 5px;
            margin-top: 10px;
            overflow: hidden;
            display: none;
        }
        
        .progress-bar {
            height: 100%;
            width: 0%;
            background: linear-gradient(90deg, #38bdf8, #818cf8);
            transition: width 0.3s ease;
        }
    </style>
</head>
<body>
    <h1>Klick32 Dashboard</h1>
    
    <div class="container">
        <div class="card">
            <h2>System Information</h2>
            <div class="metric"><span>Firmware Version:</span> <span class="metric-value">v1.1.0</span></div>
            <div class="metric"><span>CPU Speed:</span> <span class="metric-value">240 MHz</span></div>
            <div class="metric"><span>Free Heap:</span> <span class="metric-value" id="heap-val">Loading...</span></div>
            <div class="metric"><span>Uptime:</span> <span class="metric-value" id="uptime-val">Loading...</span></div>
            <div class="metric"><span>Battery Volts:</span> <span class="metric-value" id="batt-val">Loading...</span></div>
        </div>

        <div class="card">
            <h2>OTA Firmware Update</h2>
            <p style="margin-bottom: 1rem; color: #cbd5e1; font-size: 0.9rem;">Upload a compiled `.bin` file to instantly flash your console over Wi-Fi without a USB cable.</p>
            <form id="upload_form" enctype="multipart/form-data">
                <input type="file" name="update" id="file" style="margin-bottom: 1rem; color: #fff; width: 100%;" accept=".bin">
                <button type="submit" class="btn">Flash Firmware</button>
            </form>
            <div class="progress-bar-container" id="progress-container">
                <div class="progress-bar" id="progress-bar"></div>
            </div>
            <div id="status" style="margin-top: 10px; text-align: center; font-size: 0.9rem;"></div>
        </div>
    </div>

    <script>
        // Fetch System Stats periodically
        function fetchStats() {
            fetch('/api/stats')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('heap-val').innerText = (data.heap / 1024).toFixed(1) + " KB";
                    document.getElementById('uptime-val').innerText = data.uptime + " s";
                    document.getElementById('batt-val').innerText = data.batt + " V";
                });
        }
        setInterval(fetchStats, 2000);
        fetchStats();

        // OTA Upload Logic
        const form = document.getElementById('upload_form');
        form.addEventListener('submit', e => {
            e.preventDefault();
            const file = document.getElementById('file').files[0];
            if(!file) return alert('Select a .bin file first!');
            
            const formData = new FormData();
            formData.append('update', file);
            
            const xhr = new XMLHttpRequest();
            xhr.open('POST', '/update', true);
            
            document.getElementById('progress-container').style.display = 'block';
            const progressBar = document.getElementById('progress-bar');
            const statusText = document.getElementById('status');
            
            xhr.upload.onprogress = (e) => {
                if(e.lengthComputable) {
                    const percent = Math.round((e.loaded / e.total) * 100);
                    progressBar.style.width = percent + '%';
                    statusText.innerText = "Flashing: " + percent + "%";
                }
            };
            
            xhr.onload = () => {
                if(xhr.status === 200) {
                    statusText.innerText = "Success! Console is restarting...";
                    progressBar.style.background = "#22c55e"; // Green
                } else {
                    statusText.innerText = "Error: " + xhr.statusText;
                    progressBar.style.background = "#ef4444"; // Red
                }
            };
            
            xhr.send(formData);
        });
    </script>
</body>
</html>
)rawliteral";

#include <U8g2lib.h>
#include "InputManager.h"
#include <Update.h>
#include <DNSServer.h>
#include <ESPmDNS.h>


static DNSServer dnsServer;

void startWifiDashboard(U8G2& disp, InputManager& input) {
    // Set AP mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Klick32", ""); // No password for easy access
    
    // Start DNS Server for Captive Portal (Route ALL DNS requests to our IP)
    dnsServer.start(53, "*", WiFi.softAPIP());
    
    // Start mDNS Responder (Allows connecting via http://klick32.local)
    if (MDNS.begin("klick32")) {
        MDNS.addService("http", "tcp", 80);
    }

    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", html_page);
    });

    server.on("/api/stats", HTTP_GET, []() {
        char json[128];
        snprintf(json, sizeof(json), "{\"heap\":%u,\"uptime\":%u,\"batt\":3.7}", 
                 ESP.getFreeHeap(), (uint32_t)(millis() / 1000));
        server.send(200, "application/json", json);
    });

    // Captive Portal Catch-All Redirect
    server.onNotFound([]() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
    });

    // OTA Update handler
    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        delay(500);
        ESP.restart();
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("Update: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // start with max available size
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            // Flashing
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) { // true to set the size to the current progress
                Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });

    server.begin();

    // Infinite loop for dashboard mode
    uint32_t lastDraw = 0;
    while(true) {
        dnsServer.processNextRequest();
        server.handleClient();

        // Draw basic UI on screen
        if (millis() - lastDraw > 100) {
            disp.clearBuffer();
            
            disp.setDrawColor(1);
            disp.setFont(u8g2_font_ncenB08_tr);
            disp.drawStr(12, 16, "Wi-Fi Server Mode");
            
            disp.setFont(u8g2_font_5x7_tf);
            disp.drawStr(12, 32, "SSID: Klick32");
            disp.drawStr(12, 44, "IP: klick32.local");
            disp.drawStr(12, 60, "Connect your phone!");

            disp.sendBuffer();
            lastDraw = millis();
        }

        // Check if user wants to exit
        input.update();
        if (input.held(Btn::A)) {
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_OFF);
            ESP.restart(); // Easiest way to safely exit and restart OS
        }
        
        delay(2); // yield
    }
}
#endif
