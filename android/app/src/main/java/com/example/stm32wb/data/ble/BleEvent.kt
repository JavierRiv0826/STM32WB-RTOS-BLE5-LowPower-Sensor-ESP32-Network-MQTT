package com.example.stm32wb.data.ble

import java.util.UUID

sealed class BleEvent {
    object Connecting : BleEvent()
    object Connected : BleEvent()
    object Disconnected : BleEvent()
    data class Notification(val uuid: UUID, val data: ByteArray) : BleEvent()
    data class Error(val message: String) : BleEvent()
}