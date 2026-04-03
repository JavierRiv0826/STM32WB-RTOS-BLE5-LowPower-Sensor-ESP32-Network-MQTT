package com.example.stm32wb.di

import jakarta.inject.Qualifier

@Qualifier
@Retention(AnnotationRetention.BINARY)
annotation class BleRepo

@Qualifier
@Retention(AnnotationRetention.BINARY)
annotation class MqttRepo