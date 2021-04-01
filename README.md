# bonsai-sense

## BLE Sensor

Upload src/sensor/sensor.ino to your Nano 33 BLE Sense board. 

Every time the power LED is on, the board is in awake mode and polling for radio
events.

Upon startup, the board will blink twice a second, indicating it's open for 
incoming connections. As soon as a client establishes a connection and 
subscribes to all three characteristics (Temperature, Humidity, and Pressure),
the blinking will switch to once in 5 seconds, saving power.

## Collector

To have access to the `hci0` bluetooth adapter, you need to run the app locally.

To have it automatically start, it's useful to have it as a system service.

### Install as a system service

1. Create a file `/etc/systemd/system/bonsai-sense.service`

```ini
[Unit]
Description=Bonsai Sense Collector Server

[Service]
User=root
WorkingDirectory=/home/pi/bonsai-sense
ExecStart=/home/pi/bonsai-sense/bonsai --urls http://+:9111
Restart=always

[Install]
WantedBy=multi-user.target
```

2. Run `sudo systemctl daemon-reload`

3. Run `sudo systemctl enable bonsai-sense.service`
