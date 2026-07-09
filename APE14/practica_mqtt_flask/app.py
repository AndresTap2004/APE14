from flask import Flask, jsonify, render_template
import paho.mqtt.client as mqtt

app = Flask(__name__)

ultima_temperatura = "Sin datos"

MQTT_BROKER = "localhost"
MQTT_PORT = 1883
TOPICO_TEMPERATURA = "esp32/temperatura"

def on_connect(client, userdata, flags, rc):
    print("[MQTT] Conectado al broker Mosquitto")
    client.subscribe(TOPICO_TEMPERATURA)

def on_message(client, userdata, msg):
    global ultima_temperatura
    ultima_temperatura = msg.payload.decode()
    print(f"[MQTT] Recibido en {msg.topic}: {ultima_temperatura}")

mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/api/temperatura")
def api_temperatura():
    return jsonify({"temperatura": ultima_temperatura})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)