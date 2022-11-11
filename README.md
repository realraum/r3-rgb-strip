# r3-rgb-strip
A simple non-addressable RGB LED strip controller for the ESP8266.

# MQTT Messages
```
# send to r3 mqtt -> action/rgb-ledstrip/status

{"r":255,"g":255,"b":0,"br":255}

(red, green, blue, brightness)
```