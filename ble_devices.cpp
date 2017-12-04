#include "ble_devices.h"
#include <QDir>
#include <QStandardPaths>
#include <QJsonObject>
#include <QFile>

Ble_Devices::Ble_Devices(QObject *parent) : QObject(parent), m_BleController(0), m_service(0)
{
    init();
    m_DeviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    m_DeviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(5000);
}

void Ble_Devices::scanBleDevice()
{
    qDebug()<<"Scanning devices";
    m_DeviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    connect(m_DeviceDiscoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(deviceDiscovered(QBluetoothDeviceInfo)));
    connect(m_DeviceDiscoveryAgent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)),
            this, SLOT(onDeviceDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error)));
    connect(m_DeviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &Ble_Devices::onDeviceDiscoveryFinished);
    connect(m_DeviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this, &Ble_Devices::onDeviceDiscoveryCanceled);
}

void Ble_Devices::selectDevice(int index)
{
    m_selectedDevice = (QBluetoothDeviceInfo)(*m_BleDevices.at(index));
    qDebug()<< "Selected device: "<<m_selectedDevice.name()<<" UUID: "<< m_selectedDevice.serviceUuids();
}

void Ble_Devices::connect_to_BleDevice()
{
    m_BleController = new QLowEnergyController(m_selectedDevice, this);

    qDebug("Creating the LBE controller");

    connect(m_BleController, SIGNAL(connected()), this, SLOT(scanService()));
    connect(m_BleController, SIGNAL(disconnected()), this, SLOT(deviceDisconnected()));
    connect(m_BleController, static_cast<void (QLowEnergyController::*)(QLowEnergyController::Error)>(&QLowEnergyController::error),
              this, [this](QLowEnergyController::Error error) {
          Q_UNUSED(error);
          qDebug("Cannot connect to remote device.");
      });
    connect(m_BleController, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(serviceDiscovered(QBluetoothUuid)));
    connect(m_BleController, SIGNAL(discoveryFinished()), this, SLOT(serviceScanDone()));
    m_BleController->connectToDevice();
}

void Ble_Devices::deviceDiscovered(const QBluetoothDeviceInfo &deviceInfo)
{
    if (deviceInfo.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        if (deviceInfo.name().contains("Gremsy")&&deviceInfo.serviceUuids().contains(QBluetoothUuid(QUuid("0000ffe0-0000-1000-8000-00805f9b34fb")))) {
            qDebug()<<"Found Ble Gremsy gimbal: "<< deviceInfo.name() ;
            m_BleDevices.append(new QBluetoothDeviceInfo(deviceInfo));
        }
    }
}
void Ble_Devices::onDeviceDiscoveryFinished()
{
    qDebug() << "Finished scanning devices";

    if (m_BleDevices.isEmpty()) {
        qDebug() << "No Ble gimbal found";
    } else {
        qDebug() << m_BleDevices.size() << " Ble gimbals found";
    }

}

void Ble_Devices::onDeviceDiscoveryCanceled()
{
    if (m_BleDevices.isEmpty()) {
        qDebug() << "No Ble gimbal found";
    } else {
        qDebug() <<  m_BleDevices.size() << " Ble gimbals found";
    }
}

void Ble_Devices::onDeviceDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    QString errorString;
    errorString = ((QBluetoothDeviceDiscoveryAgent*)sender())->errorString();
    qWarning() << "Error: " << errorString << " | " << error;
    QString errorMessage;
    switch (error) {
    case QBluetoothDeviceDiscoveryAgent::PoweredOffError:
        qDebug() << "DeviceScanError: Bluetooth is OFF";
        // iOS lets the user go to settings and switch ON
        // Android user must do this manually
        break;
    case QBluetoothDeviceDiscoveryAgent::InputOutputError:
        qDebug() << "DeviceScanError: r/w from device error";
        break;
    case QBluetoothDeviceDiscoveryAgent::InvalidBluetoothAdapterError:
        qDebug() << "DeviceScanError: InvalidBluetoothAdapterError";
        break;
    case QBluetoothDeviceDiscoveryAgent::UnsupportedPlatformError:
        qDebug() << "DeviceScanError: UnsupportedPlatformError";
        break;
    case QBluetoothDeviceDiscoveryAgent::UnsupportedDiscoveryMethod:
                qDebug() << "DeviceScanError: UnsupportedDiscoveryMethod";
        break;
    case QBluetoothDeviceDiscoveryAgent::UnknownError:
        qDebug() << "DeviceScanError: UnknownError";
        break;
    default:
        qWarning() << "DeviceScanError: unhandled error: " << error;
        break;
    }

}

void Ble_Devices::serviceDiscovered(const QBluetoothUuid &gatt)
{

    if(gatt==QBluetoothUuid(QUuid(CUSTOM_SERVICE_UUID))){
        qDebug()<<"Found custom service";
        m_foundUARTService = true;
        qDebug()<<"Custom service UUID: "<< gatt.toString();
    } else{
        qDebug()<<"No custom service";
    }
}

void Ble_Devices::scanService()
{
    qDebug()<<"Start scanning service";
    m_BleController->discoverServices();
}

void Ble_Devices::serviceScanDone()
{
    if(m_foundUARTService){
        qDebug() << "Connecting to UART service...";
        m_service = m_BleController->createServiceObject(QBluetoothUuid(QUuid(CUSTOM_SERVICE_UUID)),this);
    }

    if(m_service){
        qDebug() <<"UART service found";
    } else {
        qDebug() <<"UART service not found";
    }

    /* 3 Step: Service Discovery */
    connect(m_service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(serviceStateChanged(QLowEnergyService::ServiceState)));
    connect(m_service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)),this, SLOT(updateData(QLowEnergyCharacteristic,QByteArray)));
    m_service->discoverDetails();

}

void Ble_Devices::serviceStateChanged(QLowEnergyService::ServiceState state)
{
    qDebug() << "Going to service state changed slot";
    // A descriptoc can only be written if the service is in the ServiceDiscovered state
    switch (state) {
    case QLowEnergyService::ServiceDiscovered:
    {

        //looking for the TX characteristic
        const QLowEnergyCharacteristic TxChar = m_service->characteristic(QBluetoothUuid(QUuid(CHAR_UUID)));
        if (!TxChar.isValid()){
            qDebug() << "Tx characteristic not found";
            break;
        }
        else{
            qDebug() << "Tx characteristic found";
        }


        // Bluetooth LE spec Where a characteristic can be notified, a Client Characteristic Configuration descriptor
        // shall be included in that characteristic as required by the Bluetooth Core Specification
        // Tx notify is enabled
        const QLowEnergyDescriptor m_notificationDescTx = TxChar.descriptor(
                    QBluetoothUuid::ClientCharacteristicConfiguration);
        if (m_notificationDescTx.isValid()) {
            // enable notification
            m_service->writeDescriptor(m_notificationDescTx, QByteArray::fromHex("0100"));
        }

        break;
    }
    default:
        //nothing for now
        break;
    }
}

void Ble_Devices::deviceDisconnected()
{
    qDebug() << "UART service disconnected";
}

void Ble_Devices::updateData(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    qDebug()<<"start updating data";
    // ignore any other characteristic change
    if (c.uuid() != QBluetoothUuid(QUuid(CHAR_UUID)))
           return;
    emit protocolMessageReadyToProcess(value);
}

void Ble_Devices::protocolMessageReadyToSend(const char *_buf, unsigned int _len)
{
    qDebug("BLE writes data");
    QByteArray data;
    for(int i =0; i<_len; i++){
        data[i]=  *(_buf + i);
    }
    const QLowEnergyCharacteristic  TxChar = m_service->characteristic(QBluetoothUuid(QUuid(CHAR_UUID)));
    m_service->writeCharacteristic(TxChar, data, QLowEnergyService::WriteWithResponse);
}

// Ble setting

void Ble_Devices::init()
{
    // Android: HomeLocation works, iOS: not writable
    // Android: AppDataLocation works out of the box, iOS you must create the DIR first !!
    mDataRoot = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).value(0);
    qDebug() << "Data Root:" << mBluetoothSettingsPath;
    bool ok = checkDirs();
    if(!ok) {
        qFatal("App won't work - cannot access root directory");
    }
    mBluetoothSettingsPath = mDataRoot + "/bt_settings.json";
    qDebug() << "Bluetooth Settings Path:" << mBluetoothSettingsPath;
    // now read cached settings file - if already existing
    readSettings();
}

bool Ble_Devices::checkDirs()
{
    QDir myDir;
    bool exists;
    exists = myDir.exists(mDataRoot);
    if (!exists) {
        bool ok = myDir.mkpath(mDataRoot);
        if(!ok) {
            qWarning() << "Couldn't create mDataRoot " << mDataRoot;
            return false;
        }
        qDebug() << "created directory mDataRoot " << mDataRoot;
    } else {
        qDebug() << "check dirs: all is OK - Data Root exists";
    }
    return true;
}

void Ble_Devices::readSettings()
{
    mBluetoothSettingsMap.clear();
    QFile settingsFile(mBluetoothSettingsPath);
    // check if already something cached
    if (!settingsFile.exists()) {
        qDebug() << "No Bluetooth Settings exists yet";
        return;
    }
    if (!settingsFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open file: " << mBluetoothSettingsPath;
        return;
    }
    jda = QJsonDocument::fromJson(settingsFile.readAll());
    settingsFile.close();
    if(!jda.isObject()) {
        qWarning() << "Couldn't create JSON Object from file: " << mBluetoothSettingsPath;
    }
    mBluetoothSettingsMap = jda.toVariant().toMap();
    qDebug() << "Bluetooth Settings Map successfully created";
}

void Ble_Devices::cacheSettings()
{
    // convert Settings Map into JSONDocument and store to app data
    QJsonDocument jda = QJsonDocument::fromVariant(mBluetoothSettingsMap);
    QFile settingsFile(mBluetoothSettingsPath);
    if (!settingsFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Couldn't open file to write " << mBluetoothSettingsPath;
        return;
    }
    qint64 bytesWritten = settingsFile.write(jda.toJson());
    settingsFile.close();
    qDebug() << "Bluetooth Settings Bytes written: " << bytesWritten;
}
