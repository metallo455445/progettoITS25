import network
import usocket
import time
import sensor
import image
import json

def url_encode(s):
    # Solo codifica basilare per i caratteri speciali usati nel JSON
    return s.replace(" ", "%20")\
            .replace("{", "%7B")\
            .replace("}", "%7D")\
            .replace("\"", "%22")\
            .replace(":", "%3A")\
            .replace(",", "%2C")\
            .replace("!", "%21")

# Dati di rete
SSID = 'ArduinoAP'  # SSID della rete Wi-Fi
PASSWORD = '12345678'  # Password della rete Wi-Fi

# Dati del server
SERVER_IP = '192.168.4.1'  # IP del server Arduino
SERVER_PORT = 80  # Porta HTTP (default 80)

# Connessione Wi-Fi
wlan = network.WLAN(network.STA_IF)
wlan.active(True)

print("Connessione alla rete Wi-Fi...")
wlan.connect(SSID, PASSWORD)

# Attendi che la connessione sia stabilita
while not wlan.isconnected():
    time.sleep(1)

print("Connesso alla rete Wi-Fi!")
print("Indirizzo IP:", wlan.ifconfig()[0])

# Connessione al server HTTP tramite socket
sock = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)
try:
    sock.connect((SERVER_IP, SERVER_PORT))
    print("Connessione al server riuscita.")
except Exception as e:
    print("Connessione al server fallita:", e)

# Setup camera
sensor.reset()
sensor.set_pixformat(sensor.GRAYSCALE)  # Usa scala di grigi
sensor.set_framesize(sensor.QVGA)       # 320x240
sensor.skip_frames(time=2000)

# Dimensioni immagine
IMG_WIDTH = 320
IMG_HEIGHT = 240
IMG_CENTER_X = IMG_WIDTH // 2
IMG_CENTER_Y = IMG_HEIGHT // 2

while True:
    # Prendi uno snapshot dalla fotocamera
    img = sensor.snapshot()

    # Rilevamento QR code
    qr_codes = img.find_qrcodes()

    if qr_codes:
        for qr in qr_codes:
            rect = qr.rect()
            qr_data = qr.payload()

            # Calcolo del centro del QR code
            qr_cx = rect[0] + rect[2] // 2
            qr_cy = rect[1] + rect[3] // 2

            # Calcola offset dal centro dell'immagine
            offset_x = qr_cx - IMG_CENTER_X
            offset_y = qr_cy - IMG_CENTER_Y

            # Disegna rettangolo e croce
            img.draw_rectangle(rect, color=255)
            img.draw_cross(qr_cx, qr_cy, color=255)
            
            # Logica di centraggio (debug)
            if abs(offset_x) < 25 and abs(offset_y) < 25:
                print("CENTRO")
                ist = "Centro"
            elif offset_x > 25:
                print("Vai a destra")
                ist = "Destra"
            elif offset_x < -25:
                print("Vai a sinistra")
                ist = "Sinistra"
            elif offset_y > 25:
                print("Vai in basso")
                ist = "Basso"
            elif offset_y < -25:
                print("Vai in alto")
                ist = "Alto"

            # Prepara messaggio JSON-like con i dati QR e offset
            msg = {
                "payload":qr_data,
                "offset_x":offset_x,
                "offset_y":offset_y,
                "istruzione":ist
            }
            msg_json = json.dumps(msg) + "\n"

            # Invia messaggio al server
            try:
                msg_json_encoded = url_encode(msg_json)
                
                # Invia la richiesta HTTP (GET) al server con i dati codificati
                request = "GET /?data={} HTTP/1.1\r\n".format(msg_json_encoded)
                request += "Host: {}\r\n".format(SERVER_IP)
                request += "Connection: close\r\n\r\n"
                sock.send(request.encode())

                # Leggi la risposta del server
                response = sock.recv(1024)
                print("Risposta del server:", response.decode())

            except Exception as e:
                print("Errore nell'invio dei dati:", e)


    time.sleep(1)

# Chiudi la connessione
sock.close()
