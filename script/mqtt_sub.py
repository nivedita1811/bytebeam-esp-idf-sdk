# python3.6

import random

from paho.mqtt import client as mqtt_client

client_id = '2'
broker = 'cloud.bytebeam.io'
port = 1883
topic = "mqtt/hello"
# username = 'emqx'
# password = 'public'

def connect_mqtt() -> mqtt_client:
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to {}:{}".format(broker,port))
        else:
            print("Failed to connect, return code %d\n", rc)

    client = mqtt_client.Client(client_id)
    # client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client

def on_message(client, userdata, msg):
    print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")

def subscribe(client: mqtt_client):
    client.subscribe(topic)
    client.on_message = on_message
    print('Subscribed to {}'.format(topic))

def run():
    client = connect_mqtt()
    subscribe(client)
    client.loop_forever()


if __name__ == '__main__':
    run() 
