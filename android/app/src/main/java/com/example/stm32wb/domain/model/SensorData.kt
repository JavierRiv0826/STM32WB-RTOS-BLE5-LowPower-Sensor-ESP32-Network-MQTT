package com.example.stm32wb.domain.model

data class SensorData(
    val temperature: Float? = null,
    val humidity: Float? = null,
    val pressure: Int? = null,
    val battery: Int? = null
)

data class MotionEvent(
    val timestamp: String
)