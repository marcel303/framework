import bluetooth
import time

from pythonosc import osc_message_builder
from pythonosc import udp_client

client = udp_client.SimpleUDPClient("255.255.255.255", 8000, True)

nearby_devices = bluetooth.discover_devices(lookup_names=False)
client.send_message("/bluetooth/count", len(nearby_devices) + 0.0)
time.sleep(2)