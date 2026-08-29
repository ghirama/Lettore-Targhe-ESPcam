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
input{padding:8px;font-size:15px;width:100%;box-sizing:border-box}
button{padding:11px 14px;margin:5px;font-size:15px;border:0;border-radius:7px}
.center{text-align:center}.ok{color:#7cff92}.err{color:#ff7b7b}.wait{color:#ffd36d}.warn{color:#ffd36d}
@media(max-width:600px){.grid{grid-template-columns:1fr}}
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
<div class="grid">
<label>Nome</label><div><b>%INFONAME%</b></div>
<label>Versione firmware</label><div><b>%FWVERSION%</b></div>
<label>Build</label><div><b>%FWBUILD%</b></div>
<label>Protocollo LPR</label><div><b>%PROTOVER%</b></div>
<label>ID hardware</label><div><b>%CAMERAID%</b></div>
<label>IP</label><div><b>%INFOIP%</b></div>
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

