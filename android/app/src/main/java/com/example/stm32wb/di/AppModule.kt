package com.example.stm32wb.di

import android.content.Context
import com.example.stm32wb.data.mqtt.MqttRepositoryImpl
import com.example.stm32wb.data.ble.BleRepositoryImpl
import com.example.stm32wb.domain.repository.SensorRepository
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.android.qualifiers.ApplicationContext
import dagger.hilt.components.SingletonComponent
import javax.inject.Singleton

@Module
@InstallIn(SingletonComponent::class)
object AppModule {

    // -------------------------
    // Provide BLE Repository
    // -------------------------
    @Provides
    @Singleton
    @BleRepo
    fun provideBleRepository(
        impl: BleRepositoryImpl
    ): SensorRepository = impl

    // -------------------------
    // Provide MQTT Repository
    // -------------------------
    @Provides
    @Singleton
    @MqttRepo
    fun provideMqttRepository(
        @ApplicationContext context: Context  // <-- inject context
    ): SensorRepository =
        MqttRepositoryImpl(context)
}