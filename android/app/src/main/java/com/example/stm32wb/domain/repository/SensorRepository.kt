package com.example.stm32wb.domain.repository

import com.example.stm32wb.domain.model.*
import kotlinx.coroutines.flow.*

interface SensorRepository {
    val sensorData: Flow<SensorData>
    val motionEvents: StateFlow<List<MotionEvent>>
    val connectionState: StateFlow<ConnectionState>

    suspend fun startConnection()
    suspend fun disconnect()
}