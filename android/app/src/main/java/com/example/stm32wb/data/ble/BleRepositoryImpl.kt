package com.example.stm32wb.data.ble

import com.example.stm32wb.domain.model.ConnectionState
import com.example.stm32wb.domain.model.MotionEvent
import com.example.stm32wb.domain.model.SensorData
import com.example.stm32wb.domain.repository.SensorRepository
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.flatMapLatest
import kotlinx.coroutines.flow.onStart
import kotlinx.coroutines.flow.transform
import timber.log.Timber
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import javax.inject.Inject
import javax.inject.Singleton

@Singleton
class BleRepositoryImpl @Inject constructor(
    private val bleManager: BleManager
) : SensorRepository {

    private val TAG = "BleRepository"

    private val _connectionState =
        MutableStateFlow<ConnectionState>(ConnectionState.Idle)
    override val connectionState: StateFlow<ConnectionState> =
        _connectionState.asStateFlow()

    private val _motionEvents =
        MutableStateFlow<List<MotionEvent>>(emptyList())
    override val motionEvents: StateFlow<List<MotionEvent>> =
        _motionEvents.asStateFlow()

    private val connectionTrigger = MutableSharedFlow<Unit>()

    override suspend fun startConnection() {
        Timber.tag(TAG).d("startConnection() called")
        connectionTrigger.emit(Unit)
    }

    override suspend fun disconnect() {
        Timber.tag(TAG).d("disconnect() called")
        bleManager.disconnect()
        _connectionState.value = ConnectionState.BleDisconnected
        _motionEvents.value = emptyList()
    }

    override val sensorData: Flow<SensorData> =
        connectionTrigger.flatMapLatest {

            Timber.tag(TAG).d("Connection trigger received")

            bleManager.connect()
                .onStart {
                    Timber.tag(TAG).d("BLE flow started")
                    _connectionState.value = ConnectionState.ConnectingBle
                }
                .transform { event ->

                    Timber.tag(TAG).d("Received event: $event")

                    when (event) {

                        is BleManager.BleEvent.Connecting -> {
                            _connectionState.value =
                                ConnectionState.ConnectingBle
                        }

                        is BleManager.BleEvent.Connected -> {
                            Timber.tag(TAG).d("BLE Connected")
                            _connectionState.value =
                                ConnectionState.BleConnected
                        }

                        is BleManager.BleEvent.Disconnected -> {
                            Timber.tag(TAG).d("BLE Disconnected")
                            _connectionState.value =
                                ConnectionState.BleDisconnected
                        }

                        is BleManager.BleEvent.Notification -> {
                            val mapped = mapToSensorData(event)
                            Timber.tag(TAG).d("Mapped SensorData: $mapped")
                            emit(mapped)
                        }

                        is BleManager.BleEvent.Error -> {
                            Timber.tag(TAG).e("BLE Error: ${event.message}")
                            _connectionState.value =
                                ConnectionState.Error(event.message)
                        }
                    }
                }
        }

    private var currentState = SensorData()

    private fun mapToSensorData(
        event: BleManager.BleEvent.Notification
    ): SensorData {

        val uuid = event.uuid
        val data = event.data

        Timber.tag(TAG).d("Mapping UUID: $uuid")

        currentState = when (uuid) {

            BleManager.TEMP_UUID -> {
                val raw = (data[1].toInt() shl 8) or
                        (data[0].toInt() and 0xFF)
                currentState.copy(temperature = raw / 100f)
            }

            BleManager.HUM_UUID -> {
                val raw = (data[1].toInt() shl 8) or
                        (data[0].toInt() and 0xFF)
                currentState.copy(humidity = raw / 100f)
            }

            BleManager.PRESS_UUID -> {
                val raw = ByteBuffer.wrap(data)
                    .order(ByteOrder.LITTLE_ENDIAN)
                    .int
                currentState.copy(pressure = raw)
            }

            BleManager.MOTION_UUID -> {
                Timber.tag(TAG).d("Motion detected!")

                val eventMotion = MotionEvent(
                    timestamp = SimpleDateFormat(
                        "hh:mm:ss a",
                        Locale.getDefault()
                    ).format(Date())
                )

                _motionEvents.value =
                    listOf(eventMotion) + _motionEvents.value

                currentState
            }

            BleManager.BATT_UUID -> {
                currentState.copy(battery = data[0].toInt())
            }

            else -> currentState
        }

        return currentState
    }
}