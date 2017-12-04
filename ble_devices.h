#ifndef BLE_DEVICES_H
#define BLE_DEVICES_H

#include <QObject>
#include <QDebug>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QJsonDocument>


#define CUSTOM_SERVICE_UUID "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHAR_UUID "0000ffe1-0000-1000-8000-00805f9b34fb"


class Ble_Devices : public QObject
{
    Q_OBJECT
public:
    explicit Ble_Devices(QObject *parent = nullptr);
    // Initialize this class
    void init();
    Q_INVOKABLE
    void scanBleDevice();
    Q_INVOKABLE
    void connect_to_BleDevice();

    Q_INVOKABLE
    void selectDevice(int index);
signals:
    // trigger signal to process data received
    void protocolMessageReadyToProcess(QByteArray msg);

public slots:

    void serviceDiscovered(const QBluetoothUuid &gatt);
    void scanService();
    void serviceScanDone();
    void serviceStateChanged(QLowEnergyService::ServiceState state);
    void deviceDisconnected();
    void updateData(const QLowEnergyCharacteristic &c,const QByteArray &value);
    void protocolMessageReadyToSend(const char *_buf, unsigned int _len);  // send message to system

private slots:
    void onDeviceDiscoveryFinished();
    void onDeviceDiscoveryCanceled();
    void onDeviceDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error);
    void deviceDiscovered(const QBluetoothDeviceInfo &deviceInfo);

private:
    QBluetoothDeviceDiscoveryAgent *m_DeviceDiscoveryAgent;
    QLowEnergyController *m_BleController;
    QBluetoothDeviceInfo m_selectedDevice;
    QLowEnergyService *m_service;
    bool m_foundUARTService;
    QList<QBluetoothDeviceInfo*> m_BleDevices;

    // Ble setting
    QString mDataRoot;
    QString mBluetoothSettingsPath;
    bool checkDirs();
    QVariantMap mBluetoothSettingsMap;
    void readSettings();
    void cacheSettings();
    QJsonDocument jda;
};

#endif // BLE_DEVICES_H
