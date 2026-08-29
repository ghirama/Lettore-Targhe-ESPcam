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
static const char* FIRMWARE_VERSION = "10.0.0-rc2";
static const char* FIRMWARE_BUILD = "HA Device + OTA + 5x RESET WiFi";
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
