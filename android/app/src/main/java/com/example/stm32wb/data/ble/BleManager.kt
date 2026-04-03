package com.example.stm32wb.data.ble

import android.bluetooth.*
import android.bluetooth.le.*
import android.content.Context
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.callbackFlow
import timber.log.Timber
import java.util.*
import javax.inject.Inject
import javax.inject.Singleton

@SuppressWarnings("MissingPermission")
@Singleton
class BleManager @Inject constructor(
    @ApplicationContext private val context: Context
) {

    companion object {
        private const val TAG = "BleManager"
        private const val DEVICE_NAME_KEYWORD = "STM32WB"

        val TEMP_UUID: UUID =
            UUID.fromString("0000A001-0000-1000-8000-00805f9b34fb")
        val HUM_UUID: UUID =
            UUID.fromString("0000A002-0000-1000-8000-00805f9b34fb")
        val PRESS_UUID: UUID =
            UUID.fromString("0000A003-0000-1000-8000-00805f9b34fb")
        val MOTION_UUID: UUID =
            UUID.fromString("0000A004-0000-1000-8000-00805f9b34fb")
        val BATT_UUID: UUID =
            UUID.fromString("00002A19-0000-1000-8000-00805f9b34fb")

        private val SERVICE_UUID: UUID =
            UUID.fromString("0000a000-0000-1000-8000-00805f9b34fb")

        private val CCCD_UUID: UUID =
            UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
    }

    sealed class BleEvent {
        object Connecting : BleEvent()
        object Connected : BleEvent()
        object Disconnected : BleEvent()
        data class Notification(val uuid: UUID, val data: ByteArray) : BleEvent()
        data class Error(val message: String) : BleEvent()
    }

    private var currentGatt: BluetoothGatt? = null

    private val bluetoothManager =
        context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager

    private val bluetoothAdapter: BluetoothAdapter?
        get() = bluetoothManager.adapter

    private val notificationQueue: Queue<BluetoothGattCharacteristic> =
        LinkedList()

    private var isWritingDescriptor = false

    fun connect(): Flow<BleEvent> = callbackFlow {

        Timber.tag(TAG).d("connect() called")

        if (bluetoothAdapter?.isEnabled != true) {
            Timber.tag(TAG).e("Bluetooth disabled")
            trySend(BleEvent.Error("Bluetooth disabled"))
            close()
            return@callbackFlow
        }

        trySend(BleEvent.Connecting)
        Timber.tag(TAG).d("Starting BLE scan...")

        val scanner = bluetoothAdapter?.bluetoothLeScanner
            ?: run {
                Timber.tag(TAG).e("BLE Scanner not available")
                trySend(BleEvent.Error("BLE Scanner not available"))
                close()
                return@callbackFlow
            }

        val gattCallback = object : BluetoothGattCallback() {

            override fun onConnectionStateChange(
                gatt: BluetoothGatt,
                status: Int,
                newState: Int
            ) {
                Timber.tag(TAG).d(
                    "onConnectionStateChange status=$status newState=$newState"
                )

                if (newState == BluetoothProfile.STATE_CONNECTED) {
                    Timber.tag(TAG).d("BLE CONNECTED")
                    currentGatt = gatt
                    trySend(BleEvent.Connected)
                    gatt.discoverServices()
                } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                    Timber.tag(TAG).d("BLE DISCONNECTED")
                    trySend(BleEvent.Disconnected)
                    close()
                }
            }

            override fun onServicesDiscovered(
                gatt: BluetoothGatt,
                status: Int
            ) {
                Timber.tag(TAG).d("onServicesDiscovered status=$status")

                if (status != BluetoothGatt.GATT_SUCCESS) {
                    Timber.tag(TAG).e("Service discovery failed")
                    return
                }

                val service = gatt.getService(SERVICE_UUID)
                if (service == null) {
                    Timber.tag(TAG).e("Custom service not found")
                    return
                }

                Timber.tag(TAG).d("Custom service found")

                val characteristics = listOf(
                    TEMP_UUID,
                    HUM_UUID,
                    PRESS_UUID,
                    MOTION_UUID,
                    BATT_UUID
                )

                characteristics.forEach { uuid ->
                    service.getCharacteristic(uuid)?.let {
                        Timber.tag(TAG).d("Queue notify for $uuid")
                        notificationQueue.add(it)
                    } ?: Timber.tag(TAG).w("Characteristic $uuid not found")
                }

                writeNextDescriptor(gatt)
            }

            override fun onCharacteristicChanged(
                gatt: BluetoothGatt,
                characteristic: BluetoothGattCharacteristic
            ) {
                Timber.tag(TAG).d(
                    "Notification from ${characteristic.uuid} size=${characteristic.value.size}"
                )

                trySend(
                    BleEvent.Notification(
                        characteristic.uuid,
                        characteristic.value
                    )
                )
            }

            override fun onDescriptorWrite(
                gatt: BluetoothGatt,
                descriptor: BluetoothGattDescriptor,
                status: Int
            ) {
                Timber.tag(TAG).d(
                    "Descriptor written for ${descriptor.characteristic.uuid}"
                )

                isWritingDescriptor = false
                writeNextDescriptor(gatt)
            }
        }

        val scanCallback = object : ScanCallback() {
            override fun onScanResult(
                callbackType: Int,
                result: ScanResult
            ) {
                val device = result.device
                val name = device.name ?: return

                Timber.tag(TAG).d("Found device: $name")

                if (name.contains(DEVICE_NAME_KEYWORD)) {
                    Timber.tag(TAG).d("Target STM32WB found: ${device.address}")
                    scanner.stopScan(this)

                    Timber.tag(TAG).d("Connecting GATT...")
                    device.connectGatt(context, false, gattCallback)
                }
            }
        }

        scanner.startScan(scanCallback)

        awaitClose {
            Timber.tag(TAG).d("Flow closed → stopping scan & closing GATT")
            scanner.stopScan(scanCallback)
            currentGatt?.disconnect()
            currentGatt?.close()
            currentGatt = null
        }
    }

    fun disconnect() {
        Timber.tag(TAG).d("disconnect() called")
        currentGatt?.disconnect()
        currentGatt?.close()
        currentGatt = null
    }

    private fun writeNextDescriptor(gatt: BluetoothGatt) {
        if (isWritingDescriptor) {
            Timber.tag(TAG).d("Descriptor write in progress, waiting...")
            return
        }

        val characteristic = notificationQueue.poll() ?: run {
            Timber.tag(TAG).d("All notifications enabled")
            return
        }

        val descriptor = characteristic.getDescriptor(CCCD_UUID)
            ?: run {
                Timber.tag(TAG).w("CCCD not found for ${characteristic.uuid}")
                return
            }

        Timber.tag(TAG).d("Enabling notification for ${characteristic.uuid}")

        gatt.setCharacteristicNotification(characteristic, true)

        val isIndication =
            characteristic.properties and
                    BluetoothGattCharacteristic.PROPERTY_INDICATE != 0

        descriptor.value =
            if (isIndication)
                BluetoothGattDescriptor.ENABLE_INDICATION_VALUE
            else
                BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE

        isWritingDescriptor = true
        gatt.writeDescriptor(descriptor)
    }
}