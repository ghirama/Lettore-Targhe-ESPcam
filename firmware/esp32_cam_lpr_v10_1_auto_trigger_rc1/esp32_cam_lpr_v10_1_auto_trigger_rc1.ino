#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <FS.h>
#include <SD_MMC.h>
#include <time.h>
#include <Update.h>
#include <ArduinoOTA.h>
#include "esp_camera.h"

// ============================================================
// VERSIONE / PROTOCOLLO
// ============================================================
static const char* FIRMWARE_VERSION = "10.1.0-rc1";
static const char* FIRMWARE_BUILD = "Auto vehicle trigger + GPIO13 + HA + OTA";
static const int LPR_PROTOCOL_VERSION = 1;


// ============================================================
// LET'S ENCRYPT - ISRG ROOT X1
// Trust anchor per verifica TLS DuckDNS/NGINX.
// ============================================================
static const char ISRG_ROOT_X1[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";


// ============================================================
// DEFAULT DI PRIMO AVVIO
// Tutti i parametri vengono poi salvati in Preferences.
// ============================================================
const char* DEFAULT_WIFI_SSID = "Violetta";
const char* DEFAULT_WIFI_PASSWORD = "";  // Configurare dal portale setup

const char* AP_PASSWORD = "CHANGE_ME_AP";  // Cambiare prima dell'uso in produzione
const byte DNS_PORT = 53;

// ============================================================
// AI THINKER / COMPATIBILE OV2640
// ============================================================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASH_LED_PIN      4

WebServer server(80);
Preferences prefs;
DNSServer dnsServer;

// ============================================================
// CONFIGURAZIONE
// ============================================================
struct Settings {
  String wifiSSID = DEFAULT_WIFI_SSID;
  String wifiPassword = DEFAULT_WIFI_PASSWORD;

  // Es.: http://192.168.1.100:8099
  // oppure https://lpr.miodominio.it
  String lprBaseUrl = "http://192.168.1.100:8099";

  String apiKey = "CAMBIA_QUESTA_CHIAVE";
  String cameraName = "Cancello";

  int photoCount = 3;
  int photoInterval = 250;

  int ocrQuality = 8;
  int brightness = 0;
  int contrast = 1;
  int saturation = 0;
  bool aec = true;
  int exposure = 300;
  bool agc = true;
  int gain = 0;
  bool awb = true;

  // HTTPS: verifica certificato server tramite ISRG Root X1.
  bool httpsVerifyCertificate = true;

  bool otaEnabled = true;
  String otaPassword = "CHANGE_ME_OTA";

  // Home Assistant device bridge
  bool haDeviceEnabled = true;
  int heartbeatIntervalSec = 30;
  int commandPollIntervalSec = 3;

  // MicroSD / log / coda offline
  bool sdLogging = true;
  bool saveFailedPhotos = true;
  bool deleteAfterUpload = true;
  int retryIntervalSec = 60;
};

Settings settings;

uint32_t eventCounter = 0;
bool lprBusy = false;
bool configAP = false;
String apSSID;

bool sdOK = false;
uint64_t sdTotalMB = 0;
uint64_t sdUsedMB = 0;
uint32_t lastPendingRetry = 0;
uint32_t lastHeartbeat = 0;
uint32_t lastCommandPoll = 0;
bool resetSequenceArmed = false;
uint32_t resetSequenceBootMillis = 0;
static const uint8_t WIFI_RESET_COUNT = 5;
static const uint32_t WIFI_RESET_WINDOW_MS = 8000;

// ============================================================
// UTILITA
// ============================================================
String htmlEscape(const String& input) {
  String s = input;
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  s.replace("'", "&#39;");
  return s;
}

String normalizeBaseUrl(String url) {
  url.trim();
  while (url.endsWith("/")) {
    url.remove(url.length() - 1);
  }
  return url;
}

String deviceSuffix() {
  uint64_t chipid = ESP.getEfuseMac();
  char buf[9];
  snprintf(buf, sizeof(buf), "%06X", (uint32_t)(chipid & 0xFFFFFF));
  return String(buf);
}

// ============================================================
// PREFERENCES
// ============================================================
void saveSettings() {
  prefs.begin("lprv6", false);

  prefs.putString("ssid", settings.wifiSSID);
  prefs.putString("wifipass", settings.wifiPassword);

  prefs.putString("lprurl", settings.lprBaseUrl);
  prefs.putString("apikey", settings.apiKey);
  prefs.putString("camname", settings.cameraName);

  prefs.putInt("photos", settings.photoCount);
  prefs.putInt("interval", settings.photoInterval);

  prefs.putInt("quality", settings.ocrQuality);
  prefs.putInt("bright", settings.brightness);
  prefs.putInt("contrast", settings.contrast);
  prefs.putInt("sat", settings.saturation);
  prefs.putBool("aec", settings.aec);
  prefs.putInt("exposure", settings.exposure);
  prefs.putBool("agc", settings.agc);
  prefs.putInt("gain", settings.gain);
  prefs.putBool("awb", settings.awb);

  prefs.putBool("tlsverify", settings.httpsVerifyCertificate);
  prefs.putBool("otaen", settings.otaEnabled);
  prefs.putString("otapass", settings.otaPassword);

  prefs.putBool("hadeven", settings.haDeviceEnabled);
  prefs.putInt("hbsec", settings.heartbeatIntervalSec);
  prefs.putInt("pollsec", settings.commandPollIntervalSec);

  prefs.putBool("sdlog", settings.sdLogging);
  prefs.putBool("savefail", settings.saveFailedPhotos);
  prefs.putBool("delupload", settings.deleteAfterUpload);
  prefs.putInt("retrysec", settings.retryIntervalSec);

  prefs.end();
}

void loadSettings() {
  prefs.begin("lprv6", true);

  settings.wifiSSID =
    prefs.getString("ssid", DEFAULT_WIFI_SSID);

  settings.wifiPassword =
    prefs.getString("wifipass", DEFAULT_WIFI_PASSWORD);

  settings.lprBaseUrl =
    prefs.getString("lprurl", "http://192.168.1.100:8099");

  settings.apiKey =
    prefs.getString("apikey", "CAMBIA_QUESTA_CHIAVE");

  settings.cameraName =
    prefs.getString("camname", "Cancello");

  settings.photoCount =
    prefs.getInt("photos", 3);

  settings.photoInterval =
    prefs.getInt("interval", 250);

  settings.ocrQuality =
    prefs.getInt("quality", 8);

  settings.brightness =
    prefs.getInt("bright", 0);

  settings.contrast =
    prefs.getInt("contrast", 1);

  settings.saturation =
    prefs.getInt("sat", 0);

  settings.aec =
    prefs.getBool("aec", true);

  settings.exposure =
    prefs.getInt("exposure", 300);

  settings.agc =
    prefs.getBool("agc", true);

  settings.gain =
    prefs.getInt("gain", 0);

  settings.awb =
    prefs.getBool("awb", true);

  settings.httpsVerifyCertificate =
    prefs.getBool("tlsverify", true);

  settings.otaEnabled = prefs.getBool("otaen", true);
  settings.otaPassword = prefs.getString("otapass", "CHANGE_ME_OTA");

  settings.sdLogging =
    prefs.getBool("sdlog", true);

  settings.saveFailedPhotos =
    prefs.getBool("savefail", true);

  settings.deleteAfterUpload =
    prefs.getBool("delupload", true);

  settings.retryIntervalSec =
    prefs.getInt("retrysec", 60);

  prefs.end();

  settings.lprBaseUrl =
    normalizeBaseUrl(settings.lprBaseUrl);
}


// ============================================================
// DATA / ORA
// ============================================================
String timestampNow() {
  struct tm timeinfo;

  if (getLocalTime(&timeinfo, 20)) {
    char buf[30];

    strftime(
      buf,
      sizeof(buf),
      "%Y-%m-%dT%H:%M:%S",
      &timeinfo
    );

    return String(buf);
  }

  return
    "UPTIME_"
    + String(millis() / 1000)
    + "s";
}

String safeName(String input) {
  input.trim();

  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];

    if (
      !(
        (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9')
        || c == '-'
        || c == '_'
      )
    ) {
      input.setCharAt(i, '_');
    }
  }

  return input;
}

// ============================================================
// MICRO SD
// ESP32-CAM AI Thinker, SD_MMC 1-bit
// usa principalmente GPIO 14, 15, 2.
// ============================================================
void updateSDStats() {
  if (!sdOK) {
    sdTotalMB = 0;
    sdUsedMB = 0;
    return;
  }

  sdTotalMB =
    SD_MMC.totalBytes()
    / (1024ULL * 1024ULL);

  sdUsedMB =
    SD_MMC.usedBytes()
    / (1024ULL * 1024ULL);
}

void appendFileLine(
  const char* path,
  const String& line
) {
  if (
    !sdOK
    || !settings.sdLogging
  ) {
    return;
  }

  File f =
    SD_MMC.open(
      path,
      FILE_APPEND
    );

  if (!f) {
    Serial.printf(
      "SD: impossibile aprire %s\n",
      path
    );

    return;
  }

  f.println(line);
  f.close();
}

void systemLog(
  const String& level,
  const String& message
) {
  String line =
    timestampNow()
    + " ["
    + level
    + "] "
    + message;

  Serial.println(line);

  appendFileLine(
    "/logs/system.log",
    line
  );
}

void ensureEventsHeader() {
  if (!sdOK) {
    return;
  }

  if (
    !SD_MMC.exists(
      "/logs/events.csv"
    )
  ) {
    File f =
      SD_MMC.open(
        "/logs/events.csv",
        FILE_WRITE
      );

    if (f) {
      f.println(
        "timestamp,event_id,camera,photo,photo_count,result,http_or_state,rssi,bytes"
      );

      f.close();
    }
  }
}

void eventLog(
  uint32_t eventID,
  int photoNumber,
  int photoCount,
  const String& result,
  const String& state,
  size_t bytes
) {
  if (
    !sdOK
    || !settings.sdLogging
  ) {
    return;
  }

  String line =
    timestampNow()
    + ","
    + String(eventID)
    + ","
    + safeName(settings.cameraName)
    + ","
    + String(photoNumber)
    + ","
    + String(photoCount)
    + ","
    + result
    + ","
    + state
    + ","
    + String(WiFi.RSSI())
    + ","
    + String(bytes);

  appendFileLine(
    "/logs/events.csv",
    line
  );
}

bool initSDCard() {
  // true = 1-bit mode
  if (
    !SD_MMC.begin(
      "/sdcard",
      true
    )
  ) {
    Serial.println(
      "SD: mount fallito"
    );

    sdOK = false;
    return false;
  }

  uint8_t type =
    SD_MMC.cardType();

  if (
    type == CARD_NONE
  ) {
    Serial.println(
      "SD: nessuna scheda"
    );

    sdOK = false;
    return false;
  }

  sdOK = true;

  SD_MMC.mkdir(
    "/logs"
  );

  SD_MMC.mkdir(
    "/pending"
  );

  ensureEventsHeader();
  updateSDStats();

  systemLog(
    "INFO",
    "MicroSD inizializzata: "
    + String(sdTotalMB)
    + " MB"
  );

  return true;
}

String pendingFileName(
  uint32_t eventID,
  int photoNumber,
  int photoCount
) {
  return
    "/pending/"
    + String(eventID)
    + "_"
    + String(photoNumber)
    + "_"
    + String(photoCount)
    + ".jpg";
}

bool savePendingPhoto(
  camera_fb_t* fb,
  uint32_t eventID,
  int photoNumber,
  int photoCount
) {
  if (
    !sdOK
    || !settings.saveFailedPhotos
    || !fb
  ) {
    return false;
  }

  String path =
    pendingFileName(
      eventID,
      photoNumber,
      photoCount
    );

  File f =
    SD_MMC.open(
      path.c_str(),
      FILE_WRITE
    );

  if (!f) {
    systemLog(
      "ERROR",
      "Impossibile creare "
      + path
    );

    return false;
  }

  size_t written =
    f.write(
      fb->buf,
      fb->len
    );

  f.close();

  bool ok =
    written == fb->len;

  if (ok) {
    systemLog(
      "WARN",
      "Foto salvata pending: "
      + path
    );
  } else {
    systemLog(
      "ERROR",
      "Scrittura pending incompleta: "
      + path
    );
  }

  updateSDStats();

  return ok;
}

int countPendingFiles() {
  if (!sdOK) {
    return 0;
  }

  File root =
    SD_MMC.open(
      "/pending"
    );

  if (!root) {
    return 0;
  }

  int count = 0;

  File file =
    root.openNextFile();

  while (file) {
    if (
      !file.isDirectory()
      && String(file.name()).endsWith(".jpg")
    ) {
      count++;
    }

    file.close();

    file =
      root.openNextFile();
  }

  root.close();

  return count;
}

bool parsePendingName(
  const String& name,
  uint32_t& eventID,
  int& photoNumber,
  int& photoCount
) {
  String base = name;

  int slash =
    base.lastIndexOf('/');

  if (slash >= 0) {
    base =
      base.substring(
        slash + 1
      );
  }

  if (!base.endsWith(".jpg")) {
    return false;
  }

  base.remove(
    base.length() - 4
  );

  int p1 =
    base.indexOf('_');

  int p2 =
    base.indexOf(
      '_',
      p1 + 1
    );

  if (
    p1 < 1
    || p2 < 0
  ) {
    return false;
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

  if (n <= 0) {
    return
      "<option value=''>Nessuna rete trovata</option>";
  }

  for (
    int i = 0;
    i < n;
    i++
  ) {
    String ssid =
      WiFi.SSID(i);

    String safe =
      htmlEscape(ssid);

    out +=
      "<option value=\"" +
      safe +
      "\">" +
      safe +
      " (" +
      String(WiFi.RSSI(i)) +
      " dBm)</option>";
  }

  WiFi.scanDelete();

  return out;
}

// ============================================================
// PAGINA CONFIG AP
// ============================================================
void handleSetupPage() {
  String networks =
    wifiScanOptions();

  String page = R"HTML(
<!doctype html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LPR Camera Setup</title>
<style>
body{font-family:Arial;background:#111;color:#eee;padding:18px}
.box{max-width:620px;margin:auto;background:#1d1d1d;padding:20px;border-radius:12px}
input,select{width:100%;padding:11px;margin:6px 0 15px;box-sizing:border-box;font-size:16px}
button{padding:12px;font-size:17px;width:100%;margin-top:8px}
small{color:#aaa}
</style>
</head>
<body>
<div class="box">
<h2>Configurazione LPR Camera</h2>

<form action="/setupsave" method="POST">

<label>Reti Wi-Fi trovate</label>
<select id="networks" onchange="document.getElementById('ssid').value=this.value">
%NETWORKS%
</select>

<label>SSID</label>
<input id="ssid" name="ssid" value="%SSID%" required>

<label>Password Wi-Fi</label>
<input name="password" type="password" value="%PASSWORD%">

<hr>

<label>URL base LPR Reader</label>
<input name="lprurl" value="%LPRURL%" placeholder="https://lpr.example.it" required>
<small>Esempio: https://lpr.miodominio.it</small>

<br><br>

<label>API key</label>
<input name="apikey" value="%APIKEY%" required>

<label>Nome camera</label>
<input name="camname" value="%CAMNAME%" required>

<button type="submit">SALVA E RIAVVIA</button>

</form>

</div>
</body>
</html>
)HTML";

  page.replace(
    "%NETWORKS%",
    networks
  );

  page.replace(
    "%SSID%",
    htmlEscape(
      settings.wifiSSID
    )
  );

  page.replace(
    "%PASSWORD%",
    htmlEscape(
      settings.wifiPassword
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
    "%TLSVERIFY%",
    String(settings.httpsVerifyCertificate ? 1 : 0)
  );

  page.replace(
    "%OTAEN%",
    String(settings.otaEnabled ? 1 : 0)
  );

  page.replace(
    "%HADEVEN%",
    String(settings.haDeviceEnabled ? 1 : 0)
  );

  page.replace(
    "%HBSEC%",
    String(settings.heartbeatIntervalSec)
  );

  page.replace(
    "%POLLSEC%",
    String(settings.commandPollIntervalSec)
  );

  page.replace(
    "%INFONAME%",
    htmlEscape(settings.cameraName)
  );

  page.replace(
    "%FWVERSION%",
    String(FIRMWARE_VERSION)
  );

  page.replace(
    "%FWBUILD%",
    String(FIRMWARE_BUILD)
  );

  page.replace(
    "%PROTOVER%",
    String(LPR_PROTOCOL_VERSION)
  );

  page.replace(
    "%CAMERAID%",
    cameraUniqueId()
  );

  page.replace(
    "%INFOIP%",
    WiFi.localIP().toString()
  );

  server.send(
    200,
    "text/html",
    page
  );
}

void handleSetupSave() {
  if (
    !server.hasArg("ssid")
    || !server.hasArg("lprurl")
  ) {
    server.send(
      400,
      "text/plain",
      "Parametri mancanti"
    );
    return;
  }

  settings.wifiSSID =
    server.arg("ssid");

  settings.wifiPassword =
    server.arg("password");

  settings.lprBaseUrl =
    normalizeBaseUrl(
      server.arg("lprurl")
    );

  if (server.hasArg("apikey")) {
    settings.apiKey =
      server.arg("apikey");
  }

  if (server.hasArg("camname")) {
    settings.cameraName =
      server.arg("camname");
  }

  saveSettings();

  server.send(
    200,
    "text/html",
    "<html><body style='font-family:Arial'>"
    "<h2>Configurazione salvata.</h2>"
    "<p>La telecamera si riavvia...</p>"
    "</body></html>"
  );

  delay(1500);
  ESP.restart();
}

// ============================================================
// SALVA DA PAGINA NORMALE
// ============================================================
void handleSave() {
  bool networkChanged = false;

  if (server.hasArg("ssid")) {
    String newSSID =
      server.arg("ssid");

    if (
      newSSID
      != settings.wifiSSID
    ) {
      networkChanged = true;
    }

    settings.wifiSSID =
      newSSID;
  }

  if (
    server.hasArg("wifipass")
    && server.arg("wifipass").length() > 0
  ) {
    settings.wifiPassword =
      server.arg("wifipass");

    networkChanged = true;
  }

  if (server.hasArg("lprurl")) {
    settings.lprBaseUrl =
      normalizeBaseUrl(
        server.arg("lprurl")
      );
  }

  if (server.hasArg("apikey")) {
    settings.apiKey =
      server.arg("apikey");
  }

  if (server.hasArg("camname")) {
    settings.cameraName =
      server.arg("camname");
  }

  if (server.hasArg("photos")) {
    settings.photoCount =
      constrain(
        (int)server.arg("photos").toInt(),
        1,
        10
      );
  }

  if (server.hasArg("interval")) {
    settings.photoInterval =
      constrain(
        (int)server.arg("interval").toInt(),
        0,
        5000
      );
  }

  if (server.hasArg("quality")) {
    settings.ocrQuality =
      constrain(
        (int)server.arg("quality").toInt(),
        4,
        30
      );
  }

  if (server.hasArg("brightness")) {
    settings.brightness =
      constrain(
        (int)server.arg("brightness").toInt(),
        -2,
        2
      );
  }

  if (server.hasArg("contrast")) {
    settings.contrast =
      constrain(
        (int)server.arg("contrast").toInt(),
        -2,
        2
      );
  }

  if (server.hasArg("saturation")) {
    settings.saturation =
      constrain(
        (int)server.arg("saturation").toInt(),
        -2,
        2
      );
  }

  if (server.hasArg("aec")) {
    settings.aec =
      server.arg("aec").toInt()
      == 1;
  }

  if (server.hasArg("exposure")) {
    settings.exposure =
      constrain(
        (int)server.arg("exposure").toInt(),
        0,
        1200
      );
  }

  if (server.hasArg("agc")) {
    settings.agc =
      server.arg("agc").toInt()
      == 1;
  }

  if (server.hasArg("gain")) {
    settings.gain =
      constrain(
        (int)server.arg("gain").toInt(),
        0,
        30
      );
  }

  if (server.hasArg("awb")) {
    settings.awb =
      server.arg("awb").toInt()
      == 1;
  }

  if (server.hasArg("tlsverify")) {
    settings.httpsVerifyCertificate =
      server.arg("tlsverify").toInt()
      == 1;
  }

  if (server.hasArg("otaen")) {
    settings.otaEnabled = server.arg("otaen").toInt() == 1;
  }

  if (server.hasArg("otapass") && server.arg("otapass").length() >= 8) {
    settings.otaPassword = server.arg("otapass");
  }

  if (server.hasArg("sdlog")) {
    settings.sdLogging =
      server.arg("sdlog").toInt()
      == 1;
  }

  if (server.hasArg("savefail")) {
    settings.saveFailedPhotos =
      server.arg("savefail").toInt()
      == 1;
  }

  if (server.hasArg("delupload")) {
    settings.deleteAfterUpload =
      server.arg("delupload").toInt()
      == 1;
  }

  if (server.hasArg("retrysec")) {
    settings.retryIntervalSec =
      constrain(
        (int)server.arg("retrysec").toInt(),
        10,
        3600
      );
  }

  saveSettings();

  applyImageSettings();
  setPreviewMode();

  server.send(
    200,
    "text/plain",
    networkChanged
      ? "Salvato. Riavvia la telecamera per applicare il nuovo WiFi."
      : "Configurazione salvata"
  );
}

// ============================================================
// PAGINA PRINCIPALE
// ============================================================
void handleRoot() {
  if (configAP) {
    handleSetupPage();
    return;
  }

  String page = R"HTML(
<!doctype html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32-CAM LPR v10 RC2</title>
<style>
body{font-family:Arial;background:#111;color:#eee;margin:0;padding:14px}
.card{max-width:760px;margin:12px auto;background:#1d1d1d;padding:16px;border-radius:12px}
h2,h3{text-align:center}
img{display:block;width:100%;max-width:640px;min-height:120px;object-fit:contain;margin:auto;background:#080808;border-radius:8px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;align-items:center}
.info-grid{display:grid;grid-template-columns:minmax(135px,0.7fr) minmax(0,1.3fr);gap:10px 18px;align-items:start}
.info-label{color:#bbb}
.info-value{font-weight:700;overflow-wrap:anywhere;word-break:break-word;min-width:0}
.info-value.mono{font-family:monospace;font-size:14px}
input{padding:8px;font-size:15px;width:100%;box-sizing:border-box}
button{padding:11px 14px;margin:5px;font-size:15px;border:0;border-radius:7px}
.center{text-align:center}.ok{color:#7cff92}.err{color:#ff7b7b}.wait{color:#ffd36d}.warn{color:#ffd36d}
@media(max-width:600px){.grid{grid-template-columns:1fr}.info-grid{grid-template-columns:1fr;gap:3px}.info-label{margin-top:9px}.info-value{padding-bottom:3px}}
</style>
</head>
<body>

<div class="card">
<h2>ESP32-CAM LPR v10 RC2</h2>
<p class="center">
WiFi: <b>%CURRENTSSID%</b><br>
IP: <b>%IP%</b><br>
RSSI: <b>%RSSI% dBm</b>
</p>

<img id="photo">

<div class="center">
<button onclick="photo()">AGGIORNA FOTO</button>
<button onclick="flash(1)">FLASH ON</button>
<button onclick="flash(0)">FLASH OFF</button>
<br>
<button onclick="trigger()">TEST LETTURA TARGA</button>
<p id="status"></p>
</div>
</div>

<div class="card">
<h3>INFO DISPOSITIVO</h3>
<div class="info-grid">
<div class="info-label">Nome</div><div class="info-value">%INFONAME%</div>
<div class="info-label">Versione firmware</div><div class="info-value mono">%FWVERSION%</div>
<div class="info-label">Build</div><div class="info-value">%FWBUILD%</div>
<div class="info-label">Protocollo LPR</div><div class="info-value mono">%PROTOVER%</div>
<div class="info-label">ID hardware</div><div class="info-value mono">%CAMERAID%</div>
<div class="info-label">IP</div><div class="info-value mono">%INFOIP%</div>
</div>
</div>

<div class="card">
<h3>RETE WIFI</h3>
<div class="grid">

<label>SSID</label>
<input id="ssid" value="%SSID%">

<label>Nuova password</label>
<input id="wifipass" type="password" placeholder="Lascia vuoto per non cambiarla">

</div>
<p class="warn">
Se il nuovo WiFi non funziona, al riavvio la telecamera crea automaticamente
la rete LPR-CAMERA-XXXX per riconfigurarla.
</p>
<p class="warn">
Reset hardware WiFi: premi RESET 5 volte entro 8 secondi.
Vengono cancellati solo SSID e password.
</p>
</div>

<div class="card">
<h3>SERVER LPR</h3>
<div class="grid">

<label>URL base</label>
<input id="lprurl" value="%LPRURL%" placeholder="https://lpr.example.it">

<label>API key</label>
<input id="apikey" value="%APIKEY%">

<label>Nome camera</label>
<input id="camname" value="%CAMNAME%">

<label>Verifica certificato HTTPS 0/1</label>
<input id="tlsverify" type="number" min="0" max="1" value="%TLSVERIFY%">

<label>Numero foto</label>
<input id="photos" type="number" value="%PHOTOS%">

<label>Intervallo foto ms</label>
<input id="interval" type="number" value="%INTERVAL%">

</div>

<div class="center">
<button onclick="testLPR()">TEST SERVER LPR</button>
</div>
</div>

<div class="card">
<h3>HOME ASSISTANT</h3>
<div class="grid">
<label>Dispositivo HA abilitato 0/1</label>
<input id="hadeven" type="number" min="0" max="1" value="%HADEVEN%">
<label>Heartbeat secondi</label>
<input id="hbsec" type="number" value="%HBSEC%">
<label>Polling comandi secondi</label>
<input id="pollsec" type="number" value="%POLLSEC%">
</div>
<p>
Il nome del dispositivo in Home Assistant segue il campo <b>Nome camera</b>.
</p>
</div>

<div class="card">
<h3>AGGIORNAMENTO FIRMWARE OTA</h3>
<div class="grid">
<label>OTA abilitato 0/1</label>
<input id="otaen" type="number" min="0" max="1" value="%OTAEN%">
<label>Nuova password OTA</label>
<input id="otapass" type="password" placeholder="Lascia vuoto per non cambiarla">
</div>
<p>Utente OTA web: <b>admin</b></p>
<div class="center">
<button onclick="location.href='/ota'">APRI AGGIORNAMENTO OTA</button>
</div>
</div>

<div class="card">
<h3>MICRO SD / LOG</h3>

<p>
Stato SD: <b>%SDSTATE%</b><br>
Totale: <b>%SDTOTAL% MB</b><br>
Usati: <b>%SDUSED% MB</b><br>
Foto pending: <b>%PENDING%</b>
</p>

<div class="grid">

<label>Log su SD 0/1</label>
<input id="sdlog" type="number" min="0" max="1" value="%SDLOG%">

<label>Salva foto se upload fallisce 0/1</label>
<input id="savefail" type="number" min="0" max="1" value="%SAVEFAIL%">

<label>Cancella pending dopo upload OK 0/1</label>
<input id="delupload" type="number" min="0" max="1" value="%DELUPLOAD%">

<label>Retry automatico secondi</label>
<input id="retrysec" type="number" value="%RETRYSEC%">

</div>

<div class="center">
<button onclick="sdStatus()">AGGIORNA STATO SD</button>
<button onclick="retryPending()">RITENTA PENDING</button>
<br>
<button onclick="location.href='/log/system'">SCARICA SYSTEM.LOG</button>
<button onclick="location.href='/log/events'">SCARICA EVENTS.CSV</button>
<button onclick="clearLogs()">SVUOTA LOG</button>
</div>

</div>

<div class="card">
<h3>CAMERA OCR</h3>
<div class="grid">

<label>Qualita JPEG</label>
<input id="quality" type="number" value="%QUALITY%">

<label>Luminosita</label>
<input id="brightness" type="number" value="%BRIGHTNESS%">

<label>Contrasto</label>
<input id="contrast" type="number" value="%CONTRAST%">

<label>Saturazione</label>
<input id="saturation" type="number" value="%SATURATION%">

<label>AEC 0/1</label>
<input id="aec" type="number" value="%AEC%">

<label>Esposizione</label>
<input id="exposure" type="number" value="%EXPOSURE%">

<label>AGC 0/1</label>
<input id="agc" type="number" value="%AGC%">

<label>Gain</label>
<input id="gain" type="number" value="%GAIN%">

<label>AWB 0/1</label>
<input id="awb" type="number" value="%AWB%">

</div>

<div class="center">
<button onclick="save()">SALVA CONFIGURAZIONE</button>
<button onclick="reboot()">RIAVVIA</button>
</div>
</div>

<script>
const statusEl=document.getElementById('status');

function msg(t,c){
 statusEl.className=c||'';
 statusEl.innerText=t;
}

function photo(){
 const im=document.getElementById('photo');
 msg('Caricamento foto...','wait');

 im.onload=()=>msg('Foto caricata','ok');
 im.onerror=()=>msg('Errore caricamento foto','err');

 im.src='/jpg?t='+Date.now();
}

function flash(v){
 fetch(v?'/flashon':'/flashoff');
}

function trigger(){
 msg('Avvio lettura targa...','wait');

 fetch('/trigger')
 .then(async r=>{
   let t=await r.text();
   msg((r.ok?'✓ ':'✗ ')+t,r.ok?'ok':'err');
 })
 .catch(e=>msg('✗ '+e,'err'));
}

function testLPR(){
 msg('Test server LPR...','wait');

 fetch('/testlpr')
 .then(async r=>{
   let t=await r.text();
   msg((r.ok?'✓ ':'✗ ')+t,r.ok?'ok':'err');
 })
 .catch(e=>msg('✗ '+e,'err'));
}

function v(id){
 return document.getElementById(id).value;
}

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

  page.replace("%INFONAME%", htmlEscape(settings.cameraName));
  page.replace("%FWVERSION%", String(FIRMWARE_VERSION));
  page.replace("%FWBUILD%", String(FIRMWARE_BUILD));
  page.replace("%PROTOVER%", String(LPR_PROTOCOL_VERSION));
  page.replace("%CAMERAID%", htmlEscape(cameraUniqueId()));
  page.replace("%INFOIP%", WiFi.localIP().toString());

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

  String otaResult;
  if (ok) {
    otaResult = "Aggiornamento completato. Riavvio...";
  } else {
    otaResult = "Aggiornamento FALLITO - errore ";
    otaResult += String(Update.getError());
    otaResult += ": ";
    otaResult += Update.errorString();
    Serial.println(otaResult);
    systemLog("ERROR", otaResult);
  }

  server.send(ok ? 200 : 500, "text/plain", otaResult);

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

      if (
        countPendingFiles() > 0
        && WiFi.status() == WL_CONNECTED
      ) {
        retryOnePending();
      }
    }

    // Riconnessione automatica se il WiFi cade.
    static uint32_t lastReconnectCheck = 0;

    if (
      millis() - lastReconnectCheck
      > 10000
    ) {
      lastReconnectCheck =
        millis();

      if (
        WiFi.status()
        != WL_CONNECTED
      ) {
        Serial.println(
          "WiFi perso: tentativo riconnessione"
        );

        WiFi.reconnect();
      }
    }
  }

  delay(1);
}
