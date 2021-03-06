# Garage Door IR Sensor

This is a little garage door state sensor based on Sharp IR sensors, an ADS1115, & an ESP-01. I originally planned to hack this together with parts I had around in an afternoon. Two months later, here were are. 😉 

The boards were made for a CT clamp current sensor but work great for reading in any voltage from 0-3.3v.

 A schematic is below. The IR sensors hook up to ports 3  & 4 of the ADS1115.
![Schematic](img/V1CurrentSensor.png)

The IR sensors need to be placed between the garage door rail and the wall (see pictures below). When the door is down, the sensors should read around 1.5 volts and near-zero when the door is up.

![Overview](./img/overview.jpg)

![Looking in at sensor with door up](./img/sensor1.jpg)
![Looking out at door with door up](./img/sensor2.jpg)
![Looking out at door with door closed](./img/sensor3.jpg)


## Home assistant 

There are examples for creating MQTT covers &  notifications working with home assistant in the `ha` directory.