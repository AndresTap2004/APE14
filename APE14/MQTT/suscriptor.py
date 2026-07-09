import paho.mqtt.client as mqtt

broker = "broker.hivemq.com"

def conectar(client, userdata, flags, rc):
    print("Conectado")

    client.subscribe("laboratorio/temperatura")
    client.subscribe("laboratorio/led")

def mensaje(client, userdata, msg):

    print(f"[Python] Recibido en {msg.topic}: {msg.payload.decode()}")

cliente = mqtt.Client()

cliente.on_connect = conectar
cliente.on_message = mensaje

cliente.connect(broker,1883)

cliente.loop_forever()