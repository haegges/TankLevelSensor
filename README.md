# TankLevelSensor
This is my little project for the development of a tank level sensor based on Wemos D1 mini.
Intention is to measure the water level of my rainwater cistern. 

Rough "specification":
First shot:
- Measurement with ultrasonic
- Connection via Wi-Fi to home LAN
- Water level is shown in browser (HTTP)
Extension with:
- Transfer of level via MQTT to smart-home
- Control of pump via relay to stop the pump below a specific level
- Control of valve to fill up again if below a specific level
- Trigger a measurement with a manual switch and show level on simple 7-digit-display
- ...
