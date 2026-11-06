#include <Arduino.h>
#include <Wire.h>
#include <lcdController.h>
#include <wifiClient.h>
#include <clockController.h>

uint32_t lastDisplayUpdate = 0;
String previousLine1;
String previousLine2;

void renderDisplay(String line1, String line2)
{
  if (line1 != previousLine1 || line2 != previousLine2)
  {
    setDisplayLines(line1, line2);
    previousLine1 = line1;
    previousLine2 = line2;
  }
}

void updateDisplay()
{
  String line1;
  String line2;
  int64_t remaining = 0;
  bool validTime = getSecondsRemaining(remaining);

  if (wifiConfigPortalActive())
  {
    line1 = "AP: ManaTI-Setup";
    line2 = "Abre 192.168.4.1";
  }
  else if (!wifiIsConnected())
  {
    if (validTime)
    {
      line1 = "Sin WiFi";
      line2 = formatCountdown(remaining);
    }
    else
    {
      line1 = "Buscando WiFi...";
      line2 = "Reintentando...";
    }
  }
  else if (!validTime)
  {
    if (clockSyncHasTimedOut())
    {
      line1 = "Error NTP";
      line2 = "Reintentando...";
    }
    else
    {
      line1 = "Buscando NTP...";
      line2 = "Hora no valida";
    }
  }
  else
  {
    line1 = clockSyncIsStale() ? "Error NTP" : "Hackathon ManaTI";
    line2 = formatCountdown(remaining);
  }

  if (validTime && remaining <= 0)
  {
    line1 = "Good job!";
    line2 = "ManaTI Boys";
    renderDisplay(centerText(line1), centerText(line2));
    return;
  }

  renderDisplay(line1, line2);
}

void setup()
{
  Serial.begin(115200);
  Wire.begin(21, 22);
  lcdSetup();
  renderDisplay("Buscando WiFi...", "Iniciando...");
  wifiInitialize();
}

void loop()
{
  wifiLoop();
  clockLoop();

  uint32_t now = millis();
  if ((uint32_t)(now - lastDisplayUpdate) >= 1000)
  {
    lastDisplayUpdate = now;
    updateDisplay();
  }
}
