  return false;
}

void handleRetryPending() {
  if (!sdOK) {
    server.send(
      500,
      "text/plain",
      "SD non disponibile"
    );

    return;
  }

  bool ok =
    retryOnePending();

  server.send(
    200,
    "text/plain",
    ok
      ? "Un file pending inviato correttamente"
      : "Nessun file inviato"
  );
}

void handleSDStatus() {
  updateSDStats();

  String txt;

  txt +=
    "SD: "
    + String(sdOK ? "OK" : "NON DISPONIBILE")
    + "\n";

  txt +=
    "Totale MB: "
    + String((unsigned long)sdTotalMB)
    + "\n";

  txt +=
    "Usati MB: "
    + String((unsigned long)sdUsedMB)
    + "\n";

  txt +=
    "Pending: "
    + String(countPendingFiles())
    + "\n";

  server.send(
    200,
    "text/plain",
    txt
  );
}

void sendSDFile(
  const char* path,
  const char* contentType,
  const char* downloadName
) {
  if (
    !sdOK
    || !SD_MMC.exists(path)
  ) {
    server.send(
      404,
      "text/plain",
      "File non disponibile"
    );

    return;
  }

  File f =
    SD_MMC.open(
      path,
      FILE_READ
    );

  if (!f) {
    server.send(
      500,
      "text/plain",
      "Errore apertura file"
    );

    return;
  }

  server.sendHeader(
    "Content-Disposition",
    String("attachment; filename=\"")
    + downloadName
    + "\""
  );

  server.streamFile(
    f,
    contentType
  );

  f.close();
}

void handleClearLogs() {
  if (!sdOK) {
    server.send(
      500,
      "text/plain",
      "SD non disponibile"
    );

    return;
  }

  SD_MMC.remove(
    "/logs/system.log"
  );

  SD_MMC.remove(
    "/logs/events.csv"
  );

  ensureEventsHeader();

  server.send(
    200,
    "text/plain",
    "Log cancellati"
  );
}


// ============================================================
// IDENTITA DISPOSITIVO
// ============================================================
String cameraUniqueId() {
  uint64_t chipid = ESP.getEfuseMac();
  char buf[17];

  snprintf(
    buf,
    sizeof(buf),
    "%04X%08X",
    (uint16_t)(chipid >> 32),
    (uint32_t)chipid
  );

  return String(buf);
}

String jsonEscape(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", "\\n");
  s.replace("\r", "");
  return s;
}

// ============================================================
// JSON HTTPS/HTTP VERSO LPR READER
// ============================================================
bool postJsonToLPR(
  const String& endpoint,
  const String& json
) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  String url =
    normalizeBaseUrl(settings.lprBaseUrl)
    + endpoint;

  HTTPClient http;
  http.setTimeout(10000);

  bool began = false;
  WiFiClient plainClient;
  WiFiClientSecure secureClient;

  if (isHttpsUrl(url)) {
    if (!waitForValidTime()) {
      return false;
    }

    if (settings.httpsVerifyCertificate) {
      secureClient.setCACert(ISRG_ROOT_X1);
    } else {
      secureClient.setInsecure();
    }

    began = http.begin(
      secureClient,
      url
    );
  } else {
    began = http.begin(
      plainClient,
      url
    );
  }

  if (!began) {
    return false;
  }

  http.addHeader(
    "Content-Type",
    "application/json"
  );

  http.addHeader(
    "X-API-Key",
    settings.apiKey
  );

  http.addHeader(
    "X-Camera-ID",
    cameraUniqueId()
  );

  int code = http.POST(json);

  http.end();

  return (
    code >= 200
    && code < 300
  );
}

// ============================================================
// HEARTBEAT DISPOSITIVO
// ============================================================
void sendCameraHeartbeat() {
  if (
    !settings.haDeviceEnabled
    || WiFi.status() != WL_CONNECTED
  ) {
    return;
  }

  updateSDStats();

  String json = "{";

  json += "\"camera_id\":\""
       + cameraUniqueId()
       + "\",";

  json += "\"name\":\""
       + jsonEscape(settings.cameraName)
       + "\",";

  json += "\"local_ip\":\""
       + WiFi.localIP().toString()
       + "\",";

  json += "\"rssi\":"
       + String(WiFi.RSSI())
       + ",";

  json += "\"firmware\":\""
       + String(FIRMWARE_VERSION)
       + "\",";

  json += "\"build\":\""
       + jsonEscape(String(FIRMWARE_BUILD))
       + "\",";

  json += "\"protocol_version\":"
       + String(LPR_PROTOCOL_VERSION)
       + ",";

  json += "\"uptime_s\":"
       + String(millis() / 1000UL)
       + ",";

  json += "\"sd_ok\":"
       + String(sdOK ? "true" : "false")
       + ",";

  json += "\"sd_total_mb\":"
       + String((unsigned long)sdTotalMB)
       + ",";

  json += "\"sd_used_mb\":"
       + String((unsigned long)sdUsedMB)
       + ",";

  json += "\"pending\":"
       + String(countPendingFiles());

  json += "}";

  bool ok =
    postJsonToLPR(
      "/camera/heartbeat",
      json
    );

  Serial.printf(
    "Heartbeat dispositivo: %s\n",
    ok ? "OK" : "FAIL"
  );
}

// ============================================================
// COMANDI DA HOME ASSISTANT
// ============================================================
void pollCameraCommand() {
  if (
    !settings.haDeviceEnabled
    || WiFi.status() != WL_CONNECTED
    || lprBusy
  ) {
    return;
  }

  String url =
    normalizeBaseUrl(settings.lprBaseUrl)
    + "/camera/command/poll?camera_id="
    + cameraUniqueId();

  HTTPClient http;
  http.setTimeout(8000);

  bool began = false;
  WiFiClient plainClient;
  WiFiClientSecure secureClient;

  if (isHttpsUrl(url)) {
    if (!waitForValidTime()) {
      return;
    }

    if (settings.httpsVerifyCertificate) {
      secureClient.setCACert(ISRG_ROOT_X1);
    } else {
      secureClient.setInsecure();
    }

    began =
      http.begin(
        secureClient,
        url
      );
  } else {
    began =
      http.begin(
        plainClient,
        url
      );
  }

  if (!began) {
    return;
  }

  http.addHeader(
    "X-API-Key",
    settings.apiKey
  );

  http.addHeader(
    "X-Camera-ID",
    cameraUniqueId()
  );

  int code = http.GET();

  if (
    code >= 200
    && code < 300
  ) {
    String body =
      http.getString();

    if (
      body.indexOf(
        "\"command\":\"capture\""
      ) >= 0
    ) {
      Serial.println(
        "Comando HA: SCATTA FOTO"
      );

      lprBusy = true;
    }
  }

  http.end();
}

// ============================================================
// RESET CREDENZIALI WIFI TRAMITE 5 RESET RAPIDI
// ============================================================
void clearWiFiCredentials() {
  prefs.begin("lprv6", false);
  prefs.putString("ssid", "");
  prefs.putString("wifipass", "");
  prefs.putUChar("rstcount", 0);
  prefs.end();
}

bool handleRapidResetSequence() {
  prefs.begin("lprv6", false);
  uint8_t count = prefs.getUChar("rstcount", 0);
  count++;
  prefs.putUChar("rstcount", count);
  prefs.end();

  Serial.printf("Sequenza reset WiFi: %u/%u\\n", count, WIFI_RESET_COUNT);

  if (count >= WIFI_RESET_COUNT) {
    Serial.println("5 reset rapidi rilevati: cancello SOLO credenziali WiFi");
    clearWiFiCredentials();
    resetSequenceArmed = false;
    return true;
  }

  resetSequenceArmed = true;
  resetSequenceBootMillis = millis();
  return false;
}

void clearRapidResetCounterIfStable() {
  if (!resetSequenceArmed) return;
  if (millis() - resetSequenceBootMillis < WIFI_RESET_WINDOW_MS) return;

  prefs.begin("lprv6", false);
  prefs.putUChar("rstcount", 0);
  prefs.end();

  resetSequenceArmed = false;
  Serial.println("Finestra reset terminata: contatore reset WiFi azzerato");
}

// ============================================================
// PREVIEW
// ============================================================
void handleJpg() {
  if (lprBusy) {
    server.send(
      503,
      "text/plain",
      "Camera occupata con lettura targa"
    );

    return;
  }

  uint32_t t0 =
    millis();

  camera_fb_t* fb =
    esp_camera_fb_get();

  if (!fb) {
    server.send(
      500,
      "text/plain",
      "Errore acquisizione camera"
    );

    return;
  }

  WiFiClient client =
    server.client();

  client.setNoDelay(true);

  client.print(
    "HTTP/1.1 200 OK\r\n"
  );

  client.print(
    "Content-Type: image/jpeg\r\n"
  );

  client.print(
    "Content-Length: "
  );

  client.print(
    fb->len
  );

  client.print(
    "\r\nCache-Control: no-store\r\n"
  );

  client.print(
    "Connection: close\r\n\r\n"
  );

  size_t sent = 0;
  const size_t CHUNK = 4096;

  while (
    sent < fb->len
    && client.connected()
  ) {
    size_t remaining =
      fb->len - sent;

    size_t n =
      remaining > CHUNK
      ? CHUNK
      : remaining;

    size_t written =
      client.write(
        fb->buf + sent,
        n
      );

    if (written == 0) {
      delay(1);
      continue;
    }

    sent += written;
    yield();
  }

  Serial.printf(
    "PREVIEW %u/%u byte, %lu ms\n",
    sent,
    fb->len,
    (unsigned long)(
      millis() - t0
    )
  );

  esp_camera_fb_return(fb);

  client.stop();
}

// ============================================================
// EVENTO LPR
// ============================================================
void handleTrigger() {
  if (configAP) {
    server.send(
      409,
      "text/plain",
      "Configura prima il WiFi"
    );

    return;
  }

  if (lprBusy) {
    server.send(
      409,
      "text/plain",
      "Lettura gia in corso"
    );

    return;
  }

  lprBusy = true;

  server.send(
    202,
    "text/plain",
    "Lettura targa avviata"
  );
}

void executeLPREvent() {
  static bool running = false;

  if (
    !lprBusy
    || running
  ) {
    return;
  }

  running = true;

  eventCounter++;

  uint32_t id =
    eventCounter;

  Serial.printf(
    "\n=== EVENTO LPR %lu ===\n",
    (unsigned long)id
  );

  setOCRMode();

  for (
    int i = 1;
    i <= settings.photoCount;
    i++
  ) {
    camera_fb_t* fb =
      esp_camera_fb_get();

    if (!fb) {
      Serial.printf(
        "Foto %d FALLITA\n",
        i
      );
    } else {
      Serial.printf(
        "Foto %d: %ux%u, %u byte\n",
        i,
        fb->width,
        fb->height,
        fb->len
      );

      int httpCode = 0;

      bool uploaded =
        uploadToLPR(
          fb,
          id,
          i,
          settings.photoCount,
          false,
          &httpCode
        );

      if (uploaded) {
        eventLog(
          id,
          i,
          settings.photoCount,
          "UPLOAD_OK",
          String(httpCode),
          fb->len
        );
      } else {
        eventLog(
          id,
          i,
          settings.photoCount,
          "UPLOAD_FAIL",
          String(httpCode),
          fb->len
        );

        savePendingPhoto(
          fb,
          id,
          i,
          settings.photoCount
        );
      }

      esp_camera_fb_return(
        fb
      );
    }

    if (
      i < settings.photoCount
    ) {
      delay(
        settings.photoInterval
      );
    }
  }

  setPreviewMode();

  Serial.printf(
    "=== EVENTO %lu COMPLETATO ===\n\n",
    (unsigned long)id
  );

  running = false;
  lprBusy = false;
}

// ============================================================
// SCANSIONE WIFI
// ============================================================
String wifiScanOptions() {
  String out;

  int n =
    WiFi.scanNetworks(
      false,
      true
    );

