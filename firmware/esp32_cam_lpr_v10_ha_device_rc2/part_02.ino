  }

  eventID =
    (uint32_t)base
    .substring(0, p1)
    .toInt();

  photoNumber =
    base
    .substring(
      p1 + 1,
      p2
    )
    .toInt();

  photoCount =
    base
    .substring(
      p2 + 1
    )
    .toInt();

  return (
    eventID > 0
    && photoNumber > 0
    && photoCount > 0
  );
}

// ============================================================
// CAMERA - BASE V5 STABILE
// ============================================================
void applyImageSettings() {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) return;

  s->set_brightness(s, settings.brightness);
  s->set_contrast(s, settings.contrast);
  s->set_saturation(s, settings.saturation);

  s->set_exposure_ctrl(s, settings.aec);
  if (!settings.aec) {
    s->set_aec_value(s, settings.exposure);
  }

  s->set_gain_ctrl(s, settings.agc);
  if (!settings.agc) {
    s->set_agc_gain(s, settings.gain);
  }

  s->set_whitebal(s, settings.awb);
}

void setPreviewMode() {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) return;

  s->set_framesize(s, FRAMESIZE_VGA);
  s->set_quality(s, 15);
}

void setOCRMode() {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) return;

  s->set_framesize(s, FRAMESIZE_UXGA);
  s->set_quality(s, settings.ocrQuality);

  applyImageSettings();

  delay(120);

  camera_fb_t* stale =
    esp_camera_fb_get();

  if (stale) {
    esp_camera_fb_return(stale);
  }
}

bool initCamera() {
  camera_config_t config = {};

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  // Valore che si è dimostrato stabile sulla tua board.
  config.xclk_freq_hz = 8000000;

  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = settings.ocrQuality;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  if (psramFound()) {
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 15;
  }

  esp_err_t err =
    esp_camera_init(&config);

  if (err != ESP_OK) {
    Serial.printf(
      "CAMERA INIT FALLITA: 0x%x\n",
      err
    );
    return false;
  }

  applyImageSettings();
  setPreviewMode();

  return true;
}

// ============================================================
// WIFI
// ============================================================
bool connectStoredWiFi(uint32_t timeoutMs = 15000) {
  if (settings.wifiSSID.length() == 0) {
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  Serial.printf(
    "Connessione WiFi a '%s'",
    settings.wifiSSID.c_str()
  );

  WiFi.begin(
    settings.wifiSSID.c_str(),
    settings.wifiPassword.c_str()
  );

  uint32_t start =
    millis();

  while (
    WiFi.status() != WL_CONNECTED
    && millis() - start < timeoutMs
  ) {
    delay(300);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi OK");

    Serial.printf(
      "IP: %s\n",
      WiFi.localIP().toString().c_str()
    );

    Serial.printf(
      "RSSI: %d dBm\n",
      WiFi.RSSI()
    );

    return true;
  }

  Serial.println(
    "Connessione WiFi fallita"
  );

  WiFi.disconnect(true);
  delay(200);

  return false;
}

void startConfigAP() {
  configAP = true;

  WiFi.mode(WIFI_AP);

  apSSID =
    "LPR-CAMERA-" + deviceSuffix();

  bool ok =
    WiFi.softAP(
      apSSID.c_str(),
      AP_PASSWORD
    );

  if (!ok) {
    Serial.println(
      "ERRORE avvio Access Point"
    );
    return;
  }

  IPAddress ip =
    WiFi.softAPIP();

  dnsServer.start(
    DNS_PORT,
    "*",
    ip
  );

  Serial.println();
  Serial.println(
    "=== MODALITA CONFIGURAZIONE ==="
  );

  Serial.printf(
    "WiFi: %s\n",
    apSSID.c_str()
  );

  Serial.printf(
    "Password AP: %s\n",
    AP_PASSWORD
  );

  Serial.printf(
    "Apri: http://%s\n",
    ip.toString().c_str()
  );
}


// ============================================================
// ORA VALIDA PER TLS
// La verifica dei certificati richiede un orologio corretto.
// ============================================================
bool waitForValidTime(uint32_t timeoutMs = 15000) {
  time_t now = time(nullptr);

  // 2025-01-01: sufficiente per capire se NTP è sincronizzato.
  const time_t MIN_VALID_TIME = 1735689600;

  if (now >= MIN_VALID_TIME) {
    return true;
  }

  Serial.print("Attesa sincronizzazione NTP");

  uint32_t start = millis();

  while (
    millis() - start < timeoutMs
    && time(nullptr) < MIN_VALID_TIME
  ) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();

  bool ok =
    time(nullptr) >= MIN_VALID_TIME;

  if (!ok) {
    systemLog(
      "ERROR",
      "Ora non valida: TLS verificato impossibile"
    );
  }

  return ok;
}

// ============================================================
// HTTP / HTTPS LPR
// ============================================================
bool isHttpsUrl(const String& url) {
  return url.startsWith("https://");
}

bool postJPEG(
  const String& url,
  camera_fb_t* fb,
  uint32_t eventID,
  int photoNumber,
  int photoCount,
  bool test,
  int* httpCodeOut = nullptr
) {
  if (
    !fb
    || WiFi.status() != WL_CONNECTED
  ) {
    return false;
  }

  HTTPClient http;
  http.setTimeout(30000);

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
      // Solo per diagnostica temporanea.
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
    Serial.println(
      "HTTP begin fallito"
    );

    return false;
  }

  http.addHeader(
    "Content-Type",
    "image/jpeg"
  );

  http.addHeader(
    "X-API-Key",
    settings.apiKey
  );

  http.addHeader(
    "X-Camera",
    settings.cameraName
  );

  http.addHeader(
    "X-Camera-ID",
    cameraUniqueId()
  );

  http.addHeader(
    "X-Event-ID",
    String(eventID)
  );

  http.addHeader(
    "X-Photo-Number",
    String(photoNumber)
  );

  http.addHeader(
    "X-Photo-Count",
    String(photoCount)
  );

  http.addHeader(
    "X-Test",
    test ? "1" : "0"
  );

  uint32_t t0 =
    millis();

  int code =
    http.POST(
      fb->buf,
      fb->len
    );

  if (httpCodeOut) {
    *httpCodeOut = code;
  }

  uint32_t elapsed =
    millis() - t0;

  Serial.printf(
    "Upload %d/%d -> HTTP %d, %u byte, %lu ms\n",
    photoNumber,
    photoCount,
    code,
    fb->len,
    (unsigned long)elapsed
  );

  if (code > 0) {
    String response =
      http.getString();

    Serial.println(response);
  }

  http.end();

  return (
    code >= 200
    && code < 300
  );
}

bool uploadToLPR(
  camera_fb_t* fb,
  uint32_t eventID,
  int photoNumber,
  int photoCount,
  bool test,
  int* httpCodeOut = nullptr
) {
  String url =
    normalizeBaseUrl(
      settings.lprBaseUrl
    )
    + "/upload";

  return postJPEG(
    url,
    fb,
    eventID,
    photoNumber,
    photoCount,
    test,
    httpCodeOut
  );
}

bool testLPRHealth() {
  if (
    WiFi.status()
    != WL_CONNECTED
  ) {
    return false;
  }

  String url =
    normalizeBaseUrl(
      settings.lprBaseUrl
    )
    + "/health";

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
      // Solo per diagnostica temporanea.
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
    return false;
  }

  http.addHeader(
    "X-API-Key",
    settings.apiKey
  );

  int code =
    http.GET();

  if (code > 0) {
    Serial.printf(
      "LPR health HTTP %d\n",
      code
    );

    Serial.println(
      http.getString()
    );
  }

  http.end();

  return (
    code >= 200
    && code < 300
  );
}


// ============================================================
// RITENTA CODA OFFLINE
// Ritenta al massimo un file per chiamata per non bloccare troppo.
// ============================================================
bool retryOnePending() {
  if (
    !sdOK
    || WiFi.status() != WL_CONNECTED
  ) {
    return false;
  }

  File root =
    SD_MMC.open(
      "/pending"
    );

  if (!root) {
    return false;
  }

  File file =
    root.openNextFile();

  while (file) {
    if (
      !file.isDirectory()
      && String(file.name()).endsWith(".jpg")
    ) {
      String path =
        String(file.name());

      uint32_t eventID = 0;
      int photoNumber = 0;
      int photoCount = 0;

      if (
        !parsePendingName(
          path,
          eventID,
          photoNumber,
          photoCount
        )
      ) {
        file.close();
        file = root.openNextFile();
        continue;
      }

      size_t len =
        file.size();

      uint8_t* buffer =
        (uint8_t*)ps_malloc(
          len
        );

      if (!buffer) {
        file.close();
        root.close();

        systemLog(
          "ERROR",
          "PSRAM insufficiente per retry pending"
        );

        return false;
      }

      size_t readBytes =
        file.read(
          buffer,
          len
        );

      file.close();

      if (
        readBytes != len
      ) {
        free(buffer);
        root.close();

        systemLog(
          "ERROR",
          "Lettura pending incompleta: "
          + path
        );

        return false;
      }

      camera_fb_t fakeFb = {};
      fakeFb.buf = buffer;
      fakeFb.len = len;

      int httpCode = 0;

      bool ok =
        uploadToLPR(
          &fakeFb,
          eventID,
          photoNumber,
          photoCount,
          false,
          &httpCode
        );

      free(buffer);

      if (ok) {
        if (
          settings.deleteAfterUpload
        ) {
          SD_MMC.remove(
            path.c_str()
          );
        }

        eventLog(
          eventID,
          photoNumber,
          photoCount,
          "RETRY_OK",
          String(httpCode),
          len
        );

        systemLog(
          "INFO",
          "Pending inviato: "
          + path
        );

        updateSDStats();
      } else {
        eventLog(
          eventID,
          photoNumber,
          photoCount,
          "RETRY_FAIL",
          String(httpCode),
          len
        );
      }

      root.close();
      return ok;
    }

    file.close();

    file =
      root.openNextFile();
  }

  root.close();

