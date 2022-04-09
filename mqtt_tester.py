#!/usr/bin/env python3

"""Quick hacky script to test mqtt auto discovery in home assistant"""

from paho.mqtt import client as mqtt_client
import os
import yaml
import json
import time

secrets_path = os.path.expanduser("~/.secrets.yaml")

with open(secrets_path) as file:
    secrets_yaml = yaml.load(file, Loader=yaml.FullLoader)


broker = secrets_yaml['esp']['DP_MQTT_HOST']
port = 1883

config_topic = "homeassistant/cover/garage01/config"
avil_topic = "homeassistant/cover/garage01/availability"
state_topic = "homeassistant/cover/garage01/state"

# generate client ID with pub prefix randomly
client_id = f'python-mqtt-1'
username = secrets_yaml['esp']['DP_MQTT_USER']
password = secrets_yaml['esp']['DP_MQTT_PASS']

def connect_mqtt() -> mqtt_client:
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    client = mqtt_client.Client(client_id)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client


def publish_config(client):
    config = {
        'name':'garage01_cover',
        'unique_id':'garage01_cover',
        'state_topic': 'homeassistant/cover/garage01/state',
        'availability': {'topic' :'homeassistant/cover/garage01/availability'},
        'payload_available': 'Online',
        'payload_not_available': 'Offline',
        'state_opening': 'opening',
        'state_closing': 'closing',
        'state_open': 'open',
        'state_closed': 'closed'
    }

    msg = json.dumps(config)
    print(len(msg))
    result = client.publish(config_topic, msg)

    status = result[0]
    if status == 0:
        print(f"Send `{msg}` to topic `{config_topic}`")
    else:
        print(f"Failed to send message to topic {config_topic}")


def publish_avil(client):
    msg = "Online"
    result = client.publish(avil_topic, msg)

    status = result[0]
    if status == 0:
        print(f"Send `{msg}` to topic `{avil_topic}`")
    else:
        print(f"Failed to send message to topic {avil_topic}")

def publish_on(client):
    msg = "closed"
    result = client.publish(state_topic, msg)

    status = result[0]
    if status == 0:
        print(f"Send `{msg}` to topic `{state_topic}`")
    else:
        print(f"Failed to send message to topic {state_topic}")

    

if __name__ == '__main__':
    client = connect_mqtt()
    publish_config(client)
    time.sleep(6)
    publish_avil(client)
    publish_on(client)
    #client.loop_forever()