function save(){
 let ids=[
   'ssid','wifipass',
   'lprurl','apikey','camname','tlsverify','otaen','otapass',
   'hadeven','hbsec','pollsec',
   'photos','interval',
   'quality','brightness','contrast','saturation',
   'aec','exposure','agc','gain','awb',
   'sdlog','savefail','delupload','retrysec'
 ];

 let p=new URLSearchParams();

 ids.forEach(id=>p.append(id,v(id)));

 msg('Salvataggio...','wait');

 fetch('/save?'+p.toString())
 .then(async r=>{
   let t=await r.text();
   msg((r.ok?'✓ ':'✗ ')+t,r.ok?'ok':'err');
 })
 .catch(e=>msg('✗ '+e,'err'));
}

function sdStatus(){
 fetch('/sdstatus')
 .then(r=>r.text())
 .then(t=>alert(t));
}

function retryPending(){
 msg('Retry pending...','wait');

 fetch('/retrypending')
 .then(async r=>{
   let t=await r.text();
   msg((r.ok?'✓ ':'✗ ')+t,r.ok?'ok':'err');
 })
 .catch(e=>msg('✗ '+e,'err'));
}

function clearLogs(){
 if(confirm('Cancellare i log dalla microSD?')){
   fetch('/clearlogs')
   .then(async r=>{
     let t=await r.text();
     msg((r.ok?'✓ ':'✗ ')+t,r.ok?'ok':'err');
   });
 }
}

function reboot(){
 if(confirm('Riavviare la telecamera?')){
   fetch('/reboot');
   msg('Riavvio...','wait');
 }
}
</script>

</body>
</html>
)HTML";

  page.replace(
    "%CURRENTSSID%",
    htmlEscape(
      WiFi.SSID()
    )
  );

  page.replace(
    "%IP%",
    WiFi.localIP().toString()
  );

  page.replace(
    "%RSSI%",
    String(WiFi.RSSI())
  );

  page.replace(
    "%SSID%",
    htmlEscape(
      settings.wifiSSID
    )
  );

  page.replace(
    "%LPRURL%",
    htmlEscape(
      settings.lprBaseUrl
    )
  );

  page.replace(
    "%APIKEY%",
    htmlEscape(
      settings.apiKey
    )
  );

  page.replace(
    "%CAMNAME%",
    htmlEscape(
      settings.cameraName
    )
  );

  page.replace(
    "%PHOTOS%",
    String(settings.photoCount)
  );

  page.replace(
    "%INTERVAL%",
    String(settings.photoInterval)
  );

  page.replace(
    "%QUALITY%",
    String(settings.ocrQuality)
  );

  page.replace(
    "%BRIGHTNESS%",
    String(settings.brightness)
  );

  page.replace(
    "%CONTRAST%",
    String(settings.contrast)
  );

  page.replace(
    "%SATURATION%",
    String(settings.saturation)
  );

  page.replace(
    "%AEC%",
    String(settings.aec ? 1 : 0)
  );

  page.replace(
    "%EXPOSURE%",
    String(settings.exposure)
  );

  page.replace(
    "%AGC%",
    String(settings.agc ? 1 : 0)
  );

  page.replace(
    "%GAIN%",
    String(settings.gain)
  );

  page.replace(
    "%AWB%",
    String(settings.awb ? 1 : 0)
  );

  updateSDStats();

  page.replace(
    "%SDSTATE%",
    sdOK ? "OK" : "NON DISPONIBILE"
  );

  page.replace(
    "%SDTOTAL%",
    String((unsigned long)sdTotalMB)
  );

  page.replace(
    "%SDUSED%",
    String((unsigned long)sdUsedMB)
  );

  page.replace(
    "%PENDING%",
    String(countPendingFiles())
  );

  page.replace(
    "%SDLOG%",
    String(settings.sdLogging ? 1 : 0)
  );

  page.replace(
    "%SAVEFAIL%",
    String(settings.saveFailedPhotos ? 1 : 0)
  );

  page.replace(
    "%DELUPLOAD%",
    String(settings.deleteAfterUpload ? 1 : 0)
  );

  page.replace(
    "%RETRYSEC%",
    String(settings.retryIntervalSec)
  );

  server.send(
    200,
    "text/html",
    page
  );
}


// ============================================================
// OTA WEB + ARDUINO OTA
// ============================================================
bool otaAuth() {
  if (!settings.otaEnabled) {
    server.send(403, "text/plain", "OTA disabilitato");
    return false;
  }

  if (!server.authenticate("admin", settings.otaPassword.c_str())) {
    server.requestAuthentication();
    return false;
  }

  return true;
}

void handleOTAPage() {
  if (!otaAuth()) return;

  server.send(200, "text/html", R"HTML(
<!doctype html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>OTA LPR Camera</title>
<style>
body{font-family:Arial;background:#111;color:#eee;padding:20px}
.box{max-width:600px;margin:auto;background:#1d1d1d;padding:20px;border-radius:12px}
input,button{font-size:16px;padding:12px;margin:8px 0;width:100%;box-sizing:border-box}
progress{width:100%;height:24px}
a{color:#8ac7ff}
</style>
</head>
<body>
<div class="box">
<h2>Aggiornamento firmware OTA</h2>
<form id="f">
<input id="bin" type="file" accept=".bin" required>
<button>CARICA FIRMWARE</button>
</form>
<progress id="p" max="100" value="0"></progress>
<p id="s"></p>
<p><a href="/">Torna alla telecamera</a></p>
</div>
<script>
f.onsubmit=e=>{
  e.preventDefault();
  if(!bin.files.length)return;
  const d=new FormData();
  d.append('firmware',bin.files[0]);
  const x=new XMLHttpRequest();
  x.open('POST','/update');
  x.upload.onprogress=e=>{
    if(e.lengthComputable){
      const v=Math.round(e.loaded*100/e.total);
      p.value=v;s.innerText='Upload '+v+'%';
    }
  };
  x.onload=()=>s.innerText=x.responseText;
  x.onerror=()=>s.innerText='Errore upload';
  s.innerText='Avvio aggiornamento...';
  x.send(d);
};
</script>
</body>
</html>
)HTML");
}

void handleOTAUpload() {
  if (!settings.otaEnabled) return;
  if (!server.authenticate("admin", settings.otaPassword.c_str())) return;

  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    lprBusy = true;
    Serial.printf("OTA WEB START: %s\n", upload.filename.c_str());
    systemLog("INFO", "OTA WEB START " + upload.filename);

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("OTA WEB OK: %u bytes\n", upload.totalSize);
      systemLog("INFO", "OTA WEB COMPLETATO " + String(upload.totalSize) + " bytes");
    } else {
      Update.printError(Serial);
      systemLog("ERROR", "OTA WEB FALLITO");
    }
  }
  else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
    lprBusy = false;
    systemLog("ERROR", "OTA WEB ANNULLATO");
  }
}

void handleOTAComplete() {
  if (!otaAuth()) {
    lprBusy = false;
    return;
  }

  bool ok = !Update.hasError();

  server.sendHeader("Connection", "close");
  server.send(ok ? 200 : 500, "text/plain",
              ok ? "Aggiornamento completato. Riavvio..."
                 : "Aggiornamento FALLITO");

  lprBusy = false;

  if (ok) {
    delay(1200);
    ESP.restart();
  }
}

void setupArduinoOTA() {
  if (!settings.otaEnabled || WiFi.status() != WL_CONNECTED) return;

  String hostname = "lpr-camera-" + deviceSuffix();

  ArduinoOTA.setHostname(hostname.c_str());
  ArduinoOTA.setPassword(settings.otaPassword.c_str());

  ArduinoOTA.onStart([]() {
    lprBusy = true;
    Serial.println("ARDUINO OTA START");
    systemLog("INFO", "ARDUINO OTA START");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nARDUINO OTA OK");
    systemLog("INFO", "ARDUINO OTA COMPLETATO");
    lprBusy = false;
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (total > 0) {
      Serial.printf("OTA: %u%%\r", (progress * 100U) / total);
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("\nARDUINO OTA ERROR: %u\n", error);
    systemLog("ERROR", "ARDUINO OTA ERROR " + String(error));
    lprBusy = false;
  });

  ArduinoOTA.begin();
  Serial.printf("ArduinoOTA attivo: %s\n", hostname.c_str());
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println();
  Serial.println(
    "ESP32-CAM LPR v10 HA DEVICE RC2"
  );

  loadSettings();

  // 5 reset rapidi entro 8 secondi = reset SOLO WiFi.
  bool wifiResetRequested = handleRapidResetSequence();

  pinMode(
    FLASH_LED_PIN,
    OUTPUT
  );

  digitalWrite(
    FLASH_LED_PIN,
    LOW
  );

  Serial.printf(
    "PSRAM: %s, totale %u, libera %u\n",
    psramFound() ? "SI" : "NO",
    ESP.getPsramSize(),
    ESP.getFreePsram()
  );

  if (!initCamera()) {
    while (true) {
      delay(1000);
    }
  }

  bool wifiOK = false;

  if (!wifiResetRequested) {
    wifiOK = connectStoredWiFi();
  }

  if (wifiResetRequested || !wifiOK) {
    startConfigAP();
  } else {
    // Fuso orario Italia con cambio automatico ora legale/solare.
    setenv(
      "TZ",
      "CET-1CEST,M3.5.0/2,M10.5.0/3",
      1
    );

    tzset();

    configTime(
      0,
      0,
      "pool.ntp.org",
      "time.google.com"
    );

    // Non blocca l'avvio: la prima chiamata HTTPS attenderà NTP se necessario.
    Serial.println("NTP configurato per verifica TLS");
  }

  initSDCard();

  systemLog(
    "INFO",
    "BOOT firmware " + String(FIRMWARE_VERSION) + " - " + String(FIRMWARE_BUILD)
  );

  // Route comuni.
  server.on(
    "/",
    HTTP_GET,
    handleRoot
  );

  server.on(
    "/setup",
    HTTP_GET,
    handleSetupPage
  );

  server.on(
    "/setupsave",
    HTTP_POST,
    handleSetupSave
  );

  server.on(
    "/save",
    HTTP_GET,
    handleSave
  );

  server.on(
    "/jpg",
    HTTP_GET,
    handleJpg
  );

  server.on(
    "/trigger",
    HTTP_GET,
    handleTrigger
  );

  server.on(
    "/testlpr",
    HTTP_GET,
    []() {
      bool ok =
        testLPRHealth();

      server.send(
        ok ? 200 : 500,
        "text/plain",

        ok
        ? "LPR Reader raggiungibile e API key valida"
        : "LPR Reader non raggiungibile / API key errata"
      );
    }
  );

  server.on(
    "/flashon",
    HTTP_GET,
    []() {
      digitalWrite(
        FLASH_LED_PIN,
        HIGH
      );

      server.send(
        200,
        "text/plain",
        "FLASH ON"
      );
    }
  );

  server.on(
    "/flashoff",
    HTTP_GET,
    []() {
      digitalWrite(
        FLASH_LED_PIN,
        LOW
      );

      server.send(
        200,
        "text/plain",
        "FLASH OFF"
      );
    }
  );

  server.on(
    "/sdstatus",
    HTTP_GET,
    handleSDStatus
  );

  server.on(
    "/retrypending",
    HTTP_GET,
    handleRetryPending
  );

  server.on(
    "/log/system",
    HTTP_GET,
    []() {
      sendSDFile(
        "/logs/system.log",
        "text/plain",
        "system.log"
      );
    }
  );

  server.on(
    "/log/events",
    HTTP_GET,
    []() {
      sendSDFile(
        "/logs/events.csv",
        "text/csv",
        "events.csv"
      );
    }
  );

  server.on(
    "/clearlogs",
    HTTP_GET,
    handleClearLogs
  );

  server.on("/ota", HTTP_GET, handleOTAPage);

  server.on(
    "/update",
    HTTP_POST,
    handleOTAComplete,
    handleOTAUpload
  );


  server.on(
    "/reboot",
    HTTP_GET,
    []() {
      server.send(
        200,
        "text/plain",
        "Riavvio"
      );

      delay(500);
      ESP.restart();
    }
  );

  // Captive portal: qualunque URL va alla pagina setup.
  server.onNotFound(
    []() {
      if (configAP) {
        server.sendHeader(
          "Location",
          "http://192.168.4.1/",
          true
        );

        server.send(
          302,
          "text/plain",
          ""
        );
      } else {
        server.send(
          404,
          "text/plain",
          "404"
        );
      }
    }
  );

  server.begin();

  Serial.println(
    "Webserver pronto"
  );

  if (!configAP) {
    setupArduinoOTA();
  }
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  clearRapidResetCounterIfStable();

  if (configAP) {
    dnsServer.processNextRequest();
  }

  server.handleClient();

  if (!configAP && settings.otaEnabled && WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
  }

  if (!configAP) {
    executeLPREvent();

    if (
      settings.haDeviceEnabled
      && WiFi.status() == WL_CONNECTED
    ) {
      if (
        millis() - lastHeartbeat
        >=
        (uint32_t)settings.heartbeatIntervalSec
        * 1000UL
      ) {
        lastHeartbeat = millis();
        sendCameraHeartbeat();
      }

      if (
        millis() - lastCommandPoll
        >=
        (uint32_t)settings.commandPollIntervalSec
        * 1000UL
      ) {
        lastCommandPoll = millis();
        pollCameraCommand();
      }
    }

    // Retry automatico coda offline: un file per intervallo.
    if (
      sdOK
      && settings.saveFailedPhotos
      && settings.retryIntervalSec >= 10
      && millis() - lastPendingRetry
         >= (uint32_t)settings.retryIntervalSec * 1000UL
    ) {
      lastPendingRetry =
        millis();

