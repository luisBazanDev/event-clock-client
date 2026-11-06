#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

constexpr char WIFI_CONFIG_SSID[] = "ManaTI-Setup";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000UL;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 10000UL;

WebServer wifiConfigServer(80);
Preferences wifiPreferences;

String savedWifiSsid;
String savedWifiPassword;
bool wifiPortalRunning = false;
bool wifiHasConnected = false;
bool wifiWasConnected = false;
uint32_t wifiConnectStartedAt = 0;
uint32_t wifiDisconnectedAt = 0;
uint32_t wifiNextRetryAt = 0;

String escapeHtml(const String &text)
{
  String escaped;
  for (size_t i = 0; i < text.length(); i++)
  {
    switch (text[i])
    {
    case '&': escaped += "&amp;"; break;
    case '<': escaped += "&lt;"; break;
    case '>': escaped += "&gt;"; break;
    case '"': escaped += "&quot;"; break;
    case '\'': escaped += "&#39;"; break;
    default: escaped += text[i]; break;
    }
  }
  return escaped;
}

void handleWifiConfigPage()
{
  int networkCount = WiFi.scanNetworks();
  String page = F("<!doctype html><html lang='es'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                  "<meta charset='utf-8'><title>Configurar ManaTI</title>"
                  "<style>body{font:16px sans-serif;max-width:30em;margin:2em auto;padding:0 1em}"
                  "input,select,button{box-sizing:border-box;width:100%;padding:.7em;margin:.4em 0}"
                  "button{background:#1769aa;color:white;border:0}</style>"
                  "<h1>Configurar Wi-Fi</h1><p>Elige tu red e ingresa la contraseña.</p>"
                  "<form method='post' action='/save'><label>Red Wi-Fi</label>"
                  "<select name='ssid' required><option value=''>Selecciona una red</option>");

  for (int i = 0; i < networkCount; i++)
  {
    String ssid = WiFi.SSID(i);
    if (!ssid.isEmpty())
      page += "<option value='" + escapeHtml(ssid) + "'>" + escapeHtml(ssid) +
              " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
  }

  page += F("</select><label>Contraseña</label><input name='password' type='password' autocomplete='current-password'>"
            "<button type='submit'>Guardar y conectar</button></form><p>Si no ves tu red, actualiza la página.</p></html>");
  wifiConfigServer.send(200, "text/html; charset=utf-8", page);
  WiFi.scanDelete();
}

void handleWifiConfigSave()
{
  String ssid = wifiConfigServer.arg("ssid");
  String password = wifiConfigServer.arg("password");
  ssid.trim();

  if (ssid.isEmpty())
  {
    wifiConfigServer.send(400, "text/plain; charset=utf-8", "Selecciona una red Wi-Fi.");
    return;
  }

  wifiPreferences.putString("ssid", ssid);
  wifiPreferences.putString("password", password);
  savedWifiSsid = ssid;
  savedWifiPassword = password;

  wifiConfigServer.send(200, "text/html; charset=utf-8",
                        "<!doctype html><meta charset='utf-8'><meta name='viewport' content='width=device-width'>"
                        "<h1>Datos guardados</h1><p>Intentando conectar. Puedes cerrar esta página.</p>");

  WiFi.disconnect(false, false);
  WiFi.begin(savedWifiSsid.c_str(), savedWifiPassword.c_str());
  wifiConnectStartedAt = millis();
  wifiWasConnected = false;
  wifiNextRetryAt = wifiConnectStartedAt + WIFI_RETRY_INTERVAL_MS;
}

void startWifiConfigPortal()
{
  if (wifiPortalRunning)
    return;

  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAP(WIFI_CONFIG_SSID))
    Serial.println("Failed to start Wi-Fi configuration access point");

  wifiConfigServer.on("/", HTTP_GET, handleWifiConfigPage);
  wifiConfigServer.on("/save", HTTP_POST, handleWifiConfigSave);
  wifiConfigServer.onNotFound([]() {
    wifiConfigServer.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
    wifiConfigServer.send(302, "text/plain", "");
  });
  wifiConfigServer.begin();
  wifiPortalRunning = true;

  Serial.print("Wi-Fi setup access point: ");
  Serial.print(WIFI_CONFIG_SSID);
  Serial.print(" at ");
  Serial.println(WiFi.softAPIP());
}

void stopWifiConfigPortal()
{
  if (!wifiPortalRunning)
    return;

  wifiConfigServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  wifiPortalRunning = false;
}

void wifiInitialize()
{
  wifiPreferences.begin("wifi", false);
  savedWifiSsid = wifiPreferences.getString("ssid", "");
  savedWifiPassword = wifiPreferences.getString("password", "");

  if (savedWifiSsid.isEmpty())
  {
    startWifiConfigPortal();
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(savedWifiSsid.c_str(), savedWifiPassword.c_str());
  wifiConnectStartedAt = millis();
  wifiNextRetryAt = wifiConnectStartedAt + WIFI_RETRY_INTERVAL_MS;
  Serial.println("Connecting to saved Wi-Fi network");
}

void wifiLoop()
{
  if (wifiPortalRunning)
    wifiConfigServer.handleClient();

  uint32_t now = millis();
  if (WiFi.status() == WL_CONNECTED)
  {
    wifiHasConnected = true;
    wifiWasConnected = true;
    if (wifiPortalRunning)
      stopWifiConfigPortal();
    return;
  }

  if (wifiWasConnected)
  {
    wifiWasConnected = false;
    wifiDisconnectedAt = now;
    wifiNextRetryAt = now;
    Serial.println("Wi-Fi connection lost; retrying");
  }

  if (savedWifiSsid.isEmpty())
  {
    startWifiConfigPortal();
    return;
  }

  if ((int32_t)(now - wifiNextRetryAt) >= 0)
  {
    WiFi.reconnect();
    wifiNextRetryAt = now + WIFI_RETRY_INTERVAL_MS;
  }

  bool timedOut = wifiHasConnected
                      ? (uint32_t)(now - wifiDisconnectedAt) >= WIFI_CONNECT_TIMEOUT_MS
                      : (uint32_t)(now - wifiConnectStartedAt) >= WIFI_CONNECT_TIMEOUT_MS;
  if (timedOut)
    startWifiConfigPortal();
}

bool wifiIsConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

bool wifiConfigPortalActive()
{
  return wifiPortalRunning;
}
