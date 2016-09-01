gcc -c /home/nikita/workspace/HomeSensors/src/util.c -o util
gcc /home/nikita/workspace/HomeSensors/src/util.c /home/nikita/workspace/HomeSensors/src/gateway.c -lpthread -o gateway
gcc /home/nikita/workspace/HomeSensors/src/util.c /home/nikita/workspace/HomeSensors/src/sensor.c -lpthread -o sensor
gcc /home/nikita/workspace/HomeSensors/src/util.c /home/nikita/workspace/HomeSensors/src/device.c -lpthread -o device 
gcc /home/nikita/workspace/HomeSensors/src/util.c /home/nikita/workspace/HomeSensors/src/backend.c -lpthread -o backend
./gateway Files/GatewayConfigurationFile.txt Files/GatewayOutputFile.txt

