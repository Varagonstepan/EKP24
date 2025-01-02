#include <Wire.h>
#include <SensirionI2CSen5x.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#else
#include <WiFi.h>
#include <WebServer.h>
#endif

// Replace with your network credentials
const char* ssid = "SensorionAP";
const char* password = "Sensor1234";

SensirionI2CSen5x sen5x;

#ifdef ESP8266
ESP8266WebServer server(80);
#else
WebServer server(80);
#endif

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 600000; // 10 minutes

#define BUFFER_SIZE 144 // 24 hours of data (6 measurements/hour)
float vocBuffer[BUFFER_SIZE] = {0};
int bufferIndex = 0;

String getSensorData() {
  float massConcentrationPm1p0;
  float massConcentrationPm2p5;
  float massConcentrationPm4p0;
  float massConcentrationPm10p0;
  float ambientHumidity;
  float ambientTemperature;
  float vocIndex;
  float noxIndex;

  uint16_t error = sen5x.readMeasuredValues(massConcentrationPm1p0, massConcentrationPm2p5,
                                            massConcentrationPm4p0, massConcentrationPm10p0,
                                            ambientHumidity, ambientTemperature, vocIndex,
                                            noxIndex);

  if (error) {
    return "Error reading sensor values";
  }

  // Store VOC index in buffer
  vocBuffer[bufferIndex] = vocIndex;
  bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;

  String vocEvaluation;
  String color;
  if (vocIndex < 100) {
    vocEvaluation = "Improving air quality";
    color = "green";
  } else if (vocIndex == 100) {
    vocEvaluation = "Baseline (Stable air quality)";
    color = "blue";
  } else if (vocIndex <= 199) {
    vocEvaluation = "Slight increase";
    color = "yellow";
  } else if (vocIndex <= 249) {
    vocEvaluation = "Moderate increase";
    color = "orange";
  } else if (vocIndex <= 349) {
    vocEvaluation = "Significant increase";
    color = "red";
  } else {
    vocEvaluation = "Severe increase";
    color = "purple";
  }

  String html = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='10'><style> body { font-family: Arial, sans-serif; text-align: center; margin: 50px; } .status { font-size: 24px; font-weight: bold; color: " + color + "; } canvas { display: block; margin: 20px auto; border: 1px solid #ccc; } </style></head><body>";

  html += "<h1>Sensor Data</h1>";
  html += "<p>MassConcentrationPm1.0: " + String(mass Concentration Pm1p0) + " &micro;g/m&sup3; (Particles size: 0-1&micro;m)</p>";
  html += "<p>MassConcentrationPm2.5: " + String(mass Concentration Pm2p5) + " &micro;g/m&sup3; (Particles size: 0-2.5&micro;m)</p>";
  html += "<p>MassConcentrationPm4.0: " + String(mass Concentration Pm4p0) + " &micro;g/m&sup3; (Particles size: 0-4&micro;m)</p>";
  html += "<p>MassConcentrationPm10.0: " + String(mass Concentration Pm10p0) + " &micro;g/m&sup3; (Particles size: 0-10&micro;m)</p>";
  html += "<p>Ambient Humidity: " + String(Ambient Humidity) + " %RH</p>";
  html += "<p>Ambient Temperature: " + String(Ambient Temperature) + " &deg;C</p>";
  html += "<p>VOC Index: " + String(VOC Index) + "</p>";
  html += "<p class='status'>VOC Status: " + vocEvaluation + "</p>";

  // Add graph
  html += "<canvas id='vocGraph' width='600' height='400'></canvas>";
  html += "<script> const canvas = document.getElementById('vocGraph'); const ctx = canvas.getContext('2d'); const vocData = [";
  for (int i = 0; i < BUFFER_SIZE; i++) {
    html += String(vocBuffer[(bufferIndex + i) % BUFFER_SIZE]) + ",";
  }
  html += "0]; ctx.clearRect(0, 0, canvas.width, canvas.height); ctx.beginPath(); const xStep = canvas.width / vocData.length; ctx.moveTo(0, canvas.height - (vocData[0] / 500 * canvas.height)); for (let i = 1; i < vocData.length; i++) { ctx.lineTo(i * xStep, canvas.height - (vocData[i] / 500 * canvas.height)); } ctx.strokeStyle = 'blue'; ctx.stroke(); // Draw axes ctx.beginPath(); ctx.moveTo(0, 0); ctx.lineTo(0, canvas.height); ctx.lineTo(canvas.width, canvas.height); ctx.strokeStyle = 'black'; ctx.stroke(); // Add labels ctx.font = '12px Arial'; ctx.fillStyle = 'black'; ctx.fillText('0', 5, canvas.height - 5); ctx.fillText('500', 5, 10); ctx.fillText('Time (24h)', canvas.width - 50, canvas.height - 5); </script>";

  html += "</body></html>";

  return html;
}

void handleRoot() {
  server.send(200, "text/html", getSensorData());
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {} // Wait for Serial interface to be ready

  Wire.begin();

  uint16_t error;
  char errorMessage[256];

  sen5x.begin(Wire);

  error = sen5x.deviceReset();
  if (error) {
    Serial.print("Error trying to execute deviceReset(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  error = sen5x.startMeasurement();
  if (error) {
    Serial.print("Error trying to execute deviceStart(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  Serial.print("Starting Access Point with SSID: ");
  Serial.println(ssid);
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  if (IP) {
    Serial.println("Access Point started successfully.");
    Serial.print("AP IP address: ");
    Serial.println(IP);
  } else {
    Serial.println("Failed to start Access Point.");
  }

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentTime;
  }
  server.handleClient();
}

