package com.example.stm32wb.ui.sensor

import android.bluetooth.BluetoothManager
import android.content.Context
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.stm32wb.di.BleRepo
import com.example.stm32wb.di.MqttRepo
import com.example.stm32wb.domain.model.ConnectionState
import com.example.stm32wb.domain.model.MotionEvent
import com.example.stm32wb.domain.model.SensorData
import com.example.stm32wb.domain.repository.SensorRepository
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.*
import kotlinx.coroutines.launch
import javax.inject.Inject

@HiltViewModel
class SensorViewModel @Inject constructor(
    @ApplicationContext private val context: Context,
    @BleRepo private val bleRepository: SensorRepository,
    @MqttRepo private val mqttRepository: SensorRepository
) : ViewModel() {

    // -------------------------
    // Mode
    // -------------------------
    private val _mode = MutableStateFlow(ConnectionMode.BLE)
    val mode: StateFlow<ConnectionMode> = _mode.asStateFlow()

    // -------------------------
    // Current active repository
    // -------------------------
    private var currentRepo: SensorRepository = bleRepository

    // -------------------------
    // Connection state
    // -------------------------
    private val _connectionState = MutableStateFlow<ConnectionState>(ConnectionState.Idle)
    val connectionState: StateFlow<ConnectionState> = _connectionState.asStateFlow()

    // -------------------------
    // Sensor data
    // -------------------------
    private val _sensorData = MutableStateFlow(SensorData())
    val sensorData: StateFlow<SensorData> = _sensorData.asStateFlow()

    // -------------------------
    // Motion events
    // -------------------------
    private val _motionEvents = MutableStateFlow<List<MotionEvent>>(emptyList())
    val motionEvents: StateFlow<List<MotionEvent>> = _motionEvents.asStateFlow()

    // -------------------------
    // Jobs
    // -------------------------
    private var connectionJob: Job? = null
    private var sensorJob: Job? = null
    private var motionJob: Job? = null

    init {
        if (isBluetoothEnabled()) {
            _mode.value = ConnectionMode.BLE
            currentRepo = bleRepository
        } else {
            _mode.value = ConnectionMode.MQTT
            currentRepo = mqttRepository
        }

        startCurrentConnection()
    }

    private val _toastMessage = MutableSharedFlow<String>()
    val toastMessage = _toastMessage.asSharedFlow()

    // -------------------------
    // Switch mode
    // -------------------------
    fun setMode(newMode: ConnectionMode) {

        if (_mode.value != newMode) {
            if (newMode == ConnectionMode.BLE && !isBluetoothEnabled()) {
                viewModelScope.launch {
                    _toastMessage.emit("¡Bluetooth is turned off!")
                }
                return
            }

            viewModelScope.launch {
                // Disconnect current repo
                currentRepo.disconnect()

                // Switch repository
                currentRepo = if (newMode == ConnectionMode.BLE) bleRepository else mqttRepository
                _mode.value = newMode

                // Start new connection
                startCurrentConnection()
            }
        }
    }

    private fun isBluetoothEnabled(): Boolean {
        val manager =
            context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        return manager.adapter?.isEnabled == true
    }

    // -------------------------
    // Start collecting from current repo
    // -------------------------
    private fun startCurrentConnection() {

        connectionJob?.cancel()
        sensorJob?.cancel()
        motionJob?.cancel()

        connectionJob = viewModelScope.launch {
            currentRepo.connectionState.collect {
                _connectionState.value = it
            }
        }

        sensorJob = viewModelScope.launch {
            currentRepo.sensorData.collect {
                _sensorData.value = it
            }
        }

        motionJob = viewModelScope.launch {
            currentRepo.motionEvents.collect {
                _motionEvents.value = it
            }
        }

        viewModelScope.launch {
            currentRepo.startConnection()
        }
    }
}