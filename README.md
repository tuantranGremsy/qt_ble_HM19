# qt_ble_HM19
This is BLE C++ class to communicate with HM19 BLE module. The class will be used in Gremsy mobile apps to configure gimbals via bluetooth connection

### Scanning device from QML
scanBleDevice();
### Select device from device list
selectDevice(int index);
### Connect to selected device
connect_to_BleDevice();

### Data packing 
protocolMessageReadyToSend(const char *_buf, unsigned int _len);

### Emit signal to parse data
protocolMessageReadyToProcess(QByteArray msg)

