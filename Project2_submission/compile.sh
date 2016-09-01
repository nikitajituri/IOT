gcc -c src/util.c -o util
gcc src/util.c src/gateway.c -lpthread -o gateway
gcc src/util.c src/sensor.c -lpthread -o sensor
gcc src/util.c src/device.c -lpthread -o device 
gcc src/util.c src/backend.c -lpthread -o backend


