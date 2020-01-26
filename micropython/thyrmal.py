import machine, esp32
import onewire, ds18x20
import time
import network
import ubinascii
from umqtt.simple import MQTTClient


def get_esp32_temp():
    """ The esp32 internal temp reported varies from part to part; adjust the value by the calibration amount.
        Unfortunately this feature has been removed/deprecated on newer esp32 hardware. (should be back on esp32-s2) """
    calibration = 52
    return esp32.raw_temperature() - calibration

def get_probe_temp():
    """ Get the temperature from the ds18b20 probe """
    pin = machine.Pin(4)
    sensor = ds18x20.DS18X20(onewire.OneWire(pin))
    rom = sensor.scan().pop() # We only have one ds18b20 probe, just pop the first one off of the list
    sensor.convert_temp()
    return sensor.read_temp(rom) * 1.8 + 32

def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect('iot-gadgets', '!QAZ2wsx')

def disconnect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.disconnect()
    wlan.active(False)

def publish(topic, data):
    client_id = ubinascii.hexlify(machine.unique_id())
    mqtt = MQTTClient(client_id, 'srosti.hopto.org')
    mqtt.connect()
    # mqtt has no notion of data types, convert float to a string
    if(isinstance(data, float)):
        data = str(data)
    mqtt.publish(topic, data)
    mqtt.disconnect()

while (True):
    esp_temp = get_esp32_temp()
    probe_temp = get_probe_temp()
    print(probe_temp)
    print(esp_temp)

    connect_wifi()
    publish('test', probe_temp)
    disconnect_wifi()
    machine.deepsleep(5000)