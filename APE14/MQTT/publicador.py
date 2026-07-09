import paho.mqtt.client as mqtt
import random
import time

broker = "broker.hivemq.com"
puerto = 1883
topico = "laboratorio/temperatura"

cliente = mqtt.Client()
cliente.connect(broker, puerto)

while True:
    temperatura = round(random.uniform(20,35),2)

    cliente.publish(topico, temperatura)

    print("[Python] Enviado:", temperatura)

    time.sleep(2)