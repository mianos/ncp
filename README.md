# Numerically Controlled Pump

```
http://131.84.1.95/pump -H "Content-Type: application/json" -d '{"speed": 4500}'
```


To test the pump handler using your specified IP address (131.84.1.123), you can use the following curl commands.

1. Set Speed and Start the Pump


    curl -X POST http://131.84.1.123/pump \
    -H "Content-Type: application/json" \
    -d '{"speed": 100, "command": "start"}'

2. Stop the Pump


    curl -X POST http://131.84.1.123/pump \
    -H "Content-Type: application/json" \
    -d '{"command": "stop"}'

3. Set Speed Only (without starting or stopping)

    curl -X POST http://131.84.1.123/pump \
    -H "Content-Type: application/json" \
    -d '{"speed": 150}'

