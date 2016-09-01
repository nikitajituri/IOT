1. We have assumed that the Primary Co ordinator will issue a connect request to the Device/Sensor if it is already serving one set of Devices like (SecurityDevice 1, Door 1, Keychain 1, M otion Sensor 1). So to test for the first condition of load balance, you will need 2 Sets. So we have added another set with the same name like DoorInput2.txt etc. Request you to add and check we have not made changes as per our req as you asked to strictly follow.

2. For Load balance the Primary coordinator will send a msg to devices to connect to its replica along with the replica ip and the port.

3. 
