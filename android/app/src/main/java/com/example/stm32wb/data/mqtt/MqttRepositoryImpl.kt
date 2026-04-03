package com.example.stm32wb.data.mqtt

import android.content.Context
import com.example.stm32wb.domain.model.ConnectionState
import com.example.stm32wb.domain.model.MotionEvent
import com.example.stm32wb.domain.model.SensorData
import com.example.stm32wb.domain.repository.SensorRepository
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.withContext
import org.eclipse.paho.android.service.MqttAndroidClient
import org.eclipse.paho.client.mqttv3.IMqttActionListener
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken
import org.eclipse.paho.client.mqttv3.IMqttToken
import org.eclipse.paho.client.mqttv3.MqttCallback
import org.eclipse.paho.client.mqttv3.MqttClient
import org.eclipse.paho.client.mqttv3.MqttConnectOptions
import org.eclipse.paho.client.mqttv3.MqttMessage
import timber.log.Timber
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import javax.inject.Inject
import javax.inject.Singleton

@Singleton
class MqttRepositoryImpl @Inject constructor(
    @ApplicationContext private val context: Context
) : SensorRepository {

    private val _connectionState = MutableStateFlow<ConnectionState>(ConnectionState.Idle)
    override val connectionState: StateFlow<ConnectionState> = _connectionState.asStateFlow()

    private val _sensorData = MutableStateFlow(SensorData())
    override val sensorData: Flow<SensorData> = _sensorData

    private val _motionEvents = MutableStateFlow<List<MotionEvent>>(emptyList())
    override val motionEvents: StateFlow<List<MotionEvent>> = _motionEvents.asStateFlow()

    private val broker = "wss://broker.hivemq.com:8884/mqtt"
    private val clientId = MqttClient.generateClientId()
    private val topicSub = "gateway/sensor/#"

    private val client = MqttAndroidClient(context, broker, clientId)

    override suspend fun startConnection() {
        withContext(Dispatchers.Main) {  // Android client needs Main thread for callbacks
            _connectionState.value = ConnectionState.ConnectingMqtt

            val options = MqttConnectOptions().apply {
                isAutomaticReconnect = true
                isCleanSession = true
            }

            client.setCallback(object : MqttCallback {
                override fun connectionLost(cause: Throwable?) {
                    Timber.e(cause, "MQTT: Connection lost")
                    _connectionState.value = ConnectionState.MqttDisconnected
                }

                override fun messageArrived(topic: String?, message: MqttMessage?) {
                    Timber.d("MQTT: Message arrived on $topic")
                    message?.let { handleMessage(topic ?: "", String(it.payload)) }
                }

                override fun deliveryComplete(token: IMqttDeliveryToken?) {
                    Timber.d("MQTT: Delivery complete")
                }
            })

            client.connect(options, null, object : IMqttActionListener {
                override fun onSuccess(asyncActionToken: IMqttToken?) {
                    Timber.d("MQTT: Connected successfully")
                    _connectionState.value = ConnectionState.MqttConnected
                    client.subscribe(topicSub, 0)
                    Timber.d("MQTT: Subscribed to $topicSub")
                }

                override fun onFailure(asyncActionToken: IMqttToken?, exception: Throwable?) {
                    Timber.e(exception, "MQTT: Connection failed")
                    _connectionState.value = ConnectionState.Error("MQTT failed: ${exception?.message}")
                }
            })
        }
    }

    override suspend fun disconnect() {
        withContext(Dispatchers.IO) {
            try {
                client.disconnect()
                Timber.d("MQTT: Disconnected")
            } catch (e: Exception) {
                Timber.e(e, "MQTT: Disconnect error")
            } finally {
                _connectionState.value = ConnectionState.MqttDisconnected
                _motionEvents.value = emptyList()
            }
        }
    }

    private fun handleMessage(topic: String, payload: String) {
        Timber.d("MQTT: Handling message -> topic=$topic, payload=$payload")
        val lowerTopic = topic.lowercase(Locale.getDefault())

        val current = _sensorData.value
        val newData = when {
            lowerTopic.endsWith("temperature") -> current.copy(temperature = payload.toFloatOrNull())
            lowerTopic.endsWith("humidity") -> current.copy(humidity = payload.toFloatOrNull())
            lowerTopic.endsWith("pressure") -> current.copy(pressure = payload.toIntOrNull())
            lowerTopic.endsWith("battery") -> current.copy(battery = payload.toIntOrNull())
            else -> current
        }
        _sensorData.value = newData

        if (lowerTopic.endsWith("motion")) {
            val event = MotionEvent(
                timestamp = SimpleDateFormat("hh:mm:ss a", Locale.getDefault()).format(Date())
            )
            Timber.d("MQTT: Motion event detected at ${event.timestamp}")
            _motionEvents.value = listOf(event) + _motionEvents.value
        }
    }
}