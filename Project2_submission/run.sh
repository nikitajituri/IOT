gnome-terminal -e "bash -c \"./gatewaysh.sh; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./backendsh.sh; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./devicesh.sh; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./doorsh.sh; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./motionsh.sh; exec bash\""
sleep 4
gnome-terminal -e "bash -c \"./keychainsh.sh; exec bash\""

