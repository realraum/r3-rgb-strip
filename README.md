# r3-rgb-strip
A simple non-addressable RGB LED strip controller for the ESP8266.

# MQTT Messages
```
# send to r3 mqtt -> action/ducttape-ledstrip/status

{"r":255,"g":255,"b":0,"br":255,"mode":0}

(red, green, blue, brightness, mode; 0-255)


# send to r3 mqtt -> action/ducttape-ledstrip/light (licht.realraum.at api)

{"r":1000,"g":1000,"b":0}

(red, green, blue; 0-1000)
```
