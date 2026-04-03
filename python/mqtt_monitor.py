import paho.mqtt.client as mqtt
from datetime import datetime

BROKER = "broker.hivemq.com"
PORT = 1883  # plain MQTT for testing
TOPIC = "gateway/sensor/#"
CLIENT_ID = "mqtt_sensor_monitor"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"\n✅ Connected to MQTT broker {BROKER}:{PORT}\n")
        client.subscribe(TOPIC)
        print(f"Subscribed to topic: {TOPIC}\n")
    else:
        print(f"❌ Connection failed with code {rc}")

def on_message(client, userdata, msg):
    now = datetime.now().strftime("%H:%M:%S")
    sensor_name = msg.topic.split("/")[-1].capitalize()
    print(f"[{now}] {sensor_name}: {msg.payload.decode()}")

def main():
    client = mqtt.Client(client_id=CLIENT_ID)
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"Connecting to MQTT broker {BROKER}:{PORT}...\n")
    client.connect(BROKER, PORT)

    client.loop_forever()

if __name__ == "__main__":
    main()
