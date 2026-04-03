package com.example.stm32wb.domain.model

sealed class ConnectionState {
    object Idle : ConnectionState()
    object ConnectingBle : ConnectionState()
    object BleConnected : ConnectionState()
    object BleDisconnected : ConnectionState()
    object ConnectingMqtt : ConnectionState()
    object MqttConnected : ConnectionState()
    object MqttDisconnected : ConnectionState()
    data class Error(val message: String) : ConnectionState()
}