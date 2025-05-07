#include <WiFiS3.h>
#include <ArduinoJson.h>
#include "Arduino_LED_Matrix.h"

char ssid[] = "ArduinoAP";      // Nome della rete creata
char pass[] = "12345678";       // Password della rete (min 8 caratteri)

WiFiServer server(80);          // Crea un server sulla porta 80
JsonDocument doc;
ArduinoLEDMatrix matrix;

struct messaggio
{
  String payload;
  String istruzione;
  int offset_X;
  int offset_y;
};

int comPin1 = 5;  //MSB
int comPin2 = 6;
int comPin3 = 7;  //LSB

String urlDecode(String input) {
  String decoded = "";
  char temp[] = "0x00";
  unsigned int len = input.length();
  unsigned int i = 0;

  while (i < len) {
    char c = input.charAt(i);
    if (c == '+') {
      decoded += ' ';
    } else if (c == '%' && i + 2 < len) {
      temp[2] = input.charAt(i + 1);
      temp[3] = input.charAt(i + 2);
      decoded += (char) strtol(temp, NULL, 16);
      i += 2;
    } else {
      decoded += c;
    }
    i++;
  }

  return decoded;
}

const uint32_t frecciaSU[] = {
  0x6009,
  0x1680600,
  0x60060000
};

const uint32_t frecciaSIN[] = {
  0x10020,
  0x5FE2001,
  0x0
};

const uint32_t frecciaGIU[] = {
  0x6006,
  0x601680,
  0x90060000
};

const uint32_t frecciaDES[] = {
  0x800,
  0x47FA0040,
  0x8000000
};

const uint32_t centro[] = {
  0x6,
  0xF00F00,
  0x60000000
};

bool convers(int intero){
  if (intero == 0)
  {
    return LOW;
  } else
  {
    return HIGH;
  }
  
  
}


void msgPLC(int i1, int i2, int i3){
  digitalWrite(comPin1, convers(i1));
  digitalWrite(comPin2, convers(i2));
  digitalWrite(comPin3, convers(i3));
  Serial.println("ping");
}

void setup() {
  pinMode(comPin1, OUTPUT);
  pinMode(comPin2, OUTPUT);
  pinMode(comPin3, OUTPUT);
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Avvio Access Point...");
  int status = WiFi.beginAP(ssid, pass);

  if (status != WL_AP_LISTENING) {
    Serial.println("Errore nell'avvio dell'AP");
    while (true);
  }

  IPAddress ip = WiFi.localIP();
  Serial.print("AP creato. IP del server: ");
  Serial.println(ip);

  matrix.begin();
  server.begin();
  msgPLC(0,0,0);
  delay(100);
  msgPLC(1,1,1);
}

void loop() {
  messaggio msg;
  WiFiClient client = server.available(); // Attende una connessione
  if (client) {
    Serial.println("Client connesso");
    // Attendi che il client invii dei dati
    while (client.connected() && !client.available()) {
      delay(1);
    }

    // Leggi la richiesta del client
    String req = "";
    while (client.available()) {
      char c = client.read();
      req += c;
    }
    Serial.println("Richiesta completa:");
    Serial.println(req);

    if (req.indexOf("GET /?data=") != -1) {
      int jsonStart = req.indexOf("data=") + 5;
      String jsonEncoded = req.substring(jsonStart, req.indexOf(" ", jsonStart));
      String jsonString = urlDecode(jsonEncoded);  // Decodifica URL -> JSON

      Serial.println("JSON decodificato:");
      Serial.println(jsonString);

    
      DeserializationError error = deserializeJson(doc, jsonString);
      if (!error) {
        msg.payload = doc["payload"].as<String>();
        msg.istruzione = doc["istruzione"].as<String>();
        msg.offset_X = doc["offset_x"].as<int>();
        msg.offset_y = doc["offset_y"].as<int>();
        Serial.println("Ricevuto JSON valido");
        Serial.print("Istruzione: ");
        Serial.println(msg.istruzione);

        if (msg.istruzione == "Alto")
        {
          matrix.loadFrame(frecciaSU);
          msgPLC(0,1,1);
        } else if (msg.istruzione == "Basso")
        {
          matrix.loadFrame(frecciaGIU);
          msgPLC(1,0,0);
        } else if (msg.istruzione == "Sinistra")
        {
          matrix.loadFrame(frecciaSIN);
          msgPLC(0,1,0);
        } else if (msg.istruzione == "Destra")
        {
          matrix.loadFrame(frecciaDES);
          msgPLC(0,0,1);
        }  else if (msg.istruzione == "Centro")
        {
          matrix.loadFrame(centro);
          msgPLC(1,0,1);
        }
        
      } else {
        Serial.println("Errore parsing JSON");
      }
    }
    
    // Risposta HTTP
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html><html><body>");
    client.println("<h1>Ricevuto dati: " + req + "</h1>");
    client.println("</body></html>");

    delay(10);
    // client.stop();
    // Serial.println("Client disconnesso");
  }
}
