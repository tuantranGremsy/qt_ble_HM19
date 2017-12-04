# qt_ble_HM19
This is BLE C++ class to communicate with HM19 BLE module. The class will be used in Gremsy mobile apps to configure gimbals via bluetooth connection

### Scanning device from QML, this class filters Gremsy gimbals by name and custom service UUID of HM19 module, so other devices will be ignored
scanBleDevice();
### Select device from device list
selectDevice(int index);
### Connect to selected device
connect_to_BleDevice();

### Data packing 
protocolMessageReadyToSend(const char *_buf, unsigned int _len);

### Emit signal to parse data
protocolMessageReadyToProcess(QByteArray msg)

