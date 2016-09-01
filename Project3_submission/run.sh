gnome-terminal -e "bash -c \"./gatewaysh.sh ConfigFiles/PrimaryGatewayConfig.txt PrimaryGatewayOutput.log; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./gatewaysh2.sh ConfigFiles/SecondaryGatewayConfig.txt SecondaryGatewayOutput.log; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./backendsh.sh ConfigFiles/PrimaryBackendConfig.txt PrimaryBackendOutput.log; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./backendsh2.sh ConfigFiles/SecondaryBackendConfig.txt SecondaryBackendOutput.log; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./motionsh.sh ConfigFiles/MotionConfig.txt InputFiles/MotionInput.txt MotionOutput.log; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./devicesh.sh ConfigFiles/SecurityDeviceConfig.txt SecurityDeviceOutput.log; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./doorsh.sh ConfigFiles/DoorConfig.txt InputFiles/DoorInput.txt DoorOutput.log; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./keychainsh.sh ConfigFiles/KeychainConfig.txt InputFiles/KeychainInput.txt KeychainOutput.log; exec bash\""
