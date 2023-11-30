#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>

#include <ArduinoJson.h>
#include "Audio.h"
HardwareSerial uart(2); // Use UART2 (pins 16 and 17 on most ESP32 boards)

const char* ssid = "";
const char* password = "";
const char* chatgpt_token = "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";
const char* temperature = "0.5";
const char* max_tokens = "25";
String Question;

#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26

Audio audio;

void setup()
{
  Serial.begin(115200);
  uart.begin(115200, SERIAL_8N1); // Configure UART with 115200 baud rate, 8 data bits, no parity, 1 stop bit
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();


  while (!Serial);

  // wait for WiFi connection

  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(100);
}

void loop()
{
  while (!uart.available()) {
      audio.loop();
    }
  
  if (uart.available()) {
    String rawInput = uart.readStringUntil('\n'); // Read until newline character
    
    // Remove non-printable characters
    String cleanedInput = "";
    for (int i = 0; i < rawInput.length(); i++) {
      if (isPrintable(rawInput.charAt(i))) {
        cleanedInput += rawInput.charAt(i);
      }
    }
    
    // Trim leading and trailing whitespace
    cleanedInput.trim();
    
    // Use cleanedInput for your API request
    Question = "\"" + cleanedInput + "\"";

    Serial.println(Question); // Print the cleaned question
    delay(5000);

    // Continue with the rest of your code for API request and audio processing
  }
  HTTPClient https;

  //Serial.print("[HTTPS] begin...\n");
  if (https.begin("https://api.openai.com/v1/completions")) {  // HTTPS

    https.addHeader("Content-Type", "application/json");
    String token_key = String("Bearer ") + chatgpt_token;
    https.addHeader("Authorization", token_key);

    String payload = String("{\"model\": \"text-davinci-003\", \"prompt\": ") + Question + String(", \"temperature\": ") + temperature + String(", \"max_tokens\": ") + max_tokens + String("}"); //Instead of TEXT as Payload, can be JSON as Paylaod

    //Serial.print("[HTTPS] GET...\n");

    // start connection and send HTTP header
    int httpCode = https.POST(payload);

    // httpCode will be negative on error
    // file found at server
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      String payload = https.getString();
      //Serial.println(payload);

      DynamicJsonDocument doc(1024);


      deserializeJson(doc, payload);
      String Answer = doc["choices"][0]["text"];
      Answer = Answer.substring(2);
      Serial.print("Answer : "); Serial.println(Answer);
      audio.connecttospeech(Answer.c_str(), "en");

    }
    else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  }
  else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }

  Question = "";
}

void audio_info(const char *info) {
  Serial.print("audio_info: "); Serial.println(info);
}