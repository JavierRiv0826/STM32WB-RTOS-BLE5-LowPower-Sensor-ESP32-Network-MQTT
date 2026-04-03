package com.example.stm32wb.ui.sensor

import android.widget.Toast
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Battery3Bar
import androidx.compose.material.icons.filled.Battery6Bar
import androidx.compose.material.icons.filled.BatteryAlert
import androidx.compose.material.icons.filled.BatteryFull
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.hilt.lifecycle.viewmodel.compose.hiltViewModel
import com.example.stm32wb.domain.model.ConnectionState
import com.example.stm32wb.domain.model.SensorData


@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SensorScreen(
    viewModel: SensorViewModel = hiltViewModel()
) {
    val context = LocalContext.current

    val mode by viewModel.mode.collectAsState()
    val connectionState by viewModel.connectionState.collectAsState()
    val sensorData by viewModel.sensorData.collectAsState()
    val motionEvents by viewModel.motionEvents.collectAsState()

    LaunchedEffect(Unit) {
        viewModel.toastMessage.collect { message ->
            Toast.makeText(context, message, Toast.LENGTH_SHORT).show()
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("ᛒ BLE-MQTT \uD83C\uDF10 Sensor Network") }
            )
        }
    ) { padding ->

        Column(
            modifier = Modifier
                .padding(padding)
                .fillMaxSize()
                .padding(16.dp)
        ) {

            // -------------------------
            // Mode Switch Section
            // -------------------------
            Row(verticalAlignment = Alignment.CenterVertically) {
                val modetxt = if (mode == ConnectionMode.MQTT) "MQTT" else "BLE"
                Text("Mode: $modetxt")

                Spacer(Modifier.width(8.dp))

                Switch(
                    checked = mode == ConnectionMode.MQTT,
                    onCheckedChange = { checked ->
                        viewModel.setMode(if (checked) ConnectionMode.MQTT else ConnectionMode.BLE)
                    }
                )

                Spacer(Modifier.width(8.dp))

                Text(if (mode == ConnectionMode.MQTT) "tap to switch to BLE" else "tap to switch to MQTT")
            }

            Spacer(Modifier.height(24.dp))

            // -------------------------
            // Connection State Section
            // -------------------------
            ConnectionStatus(connectionState, mode)

            Spacer(Modifier.height(24.dp))

            // -------------------------
            // Environmental Card
            // -------------------------
            EnvironmentalCard(sensorData)

            Spacer(Modifier.height(16.dp))

            // -------------------------
            // Battery Indicator
            // -------------------------
            BatteryIndicator(sensorData.battery)

            Spacer(Modifier.height(24.dp))

            Text(
                text = "Motion Events",
                style = MaterialTheme.typography.titleMedium
            )

            Spacer(Modifier.height(8.dp))

            LazyColumn(
                modifier = Modifier.fillMaxSize()
            ) {
                items(motionEvents) { event ->
                    MotionCard(event.timestamp)
                }
            }
        }
    }
}

@Composable
fun ConnectionStatus(state: ConnectionState, mode: ConnectionMode) {

    val icon = if(mode == ConnectionMode.BLE) "ᛒ" else "\uD83C\uDF10"

    when (state) {

        is ConnectionState.Idle -> {
            Text("Idle")
        }

        is ConnectionState.ConnectingBle,
        is ConnectionState.ConnectingMqtt -> {

            Row(verticalAlignment = Alignment.CenterVertically) {
                CircularProgressIndicator(
                    modifier = Modifier.size(20.dp),
                    strokeWidth = 2.dp
                )
                Spacer(Modifier.width(12.dp))
                Text("$icon Connecting...")
            }
        }

        is ConnectionState.BleConnected,
        is ConnectionState.MqttConnected -> {
            Text(
                text = "$icon Connected OK",
                color = Color(0xFF2E7D32) // Green
            )
        }

        is ConnectionState.BleDisconnected,
        is ConnectionState.MqttDisconnected -> {
            Text(
                text = "$icon Disconnected ",
                color = Color.Red
            )
        }

        is ConnectionState.Error -> {
            Text(
                text = "Error: ${state.message}",
                color = Color.Red
            )
        }
    }
}

@Composable
fun EnvironmentalCard(sensorData: SensorData) {

    Card(
        modifier = Modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(4.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {

            Text(
                text = "Environment",
                style = MaterialTheme.typography.titleMedium
            )

            Spacer(Modifier.height(12.dp))

            Text("Temperature: ${sensorData.temperature ?: "--"} °C")
            Text("Humidity: ${sensorData.humidity ?: "--"} %")
            Text("Pressure: ${sensorData.pressure ?: "--"} Pa")
        }
    }
}

@Composable
fun BatteryIndicator(battery: Int?) {

    val level = battery ?: 0

    val icon = when {
        level > 80 -> Icons.Default.BatteryFull
        level > 40 -> Icons.Default.Battery6Bar
        level > 15 -> Icons.Default.Battery3Bar
        else -> Icons.Default.BatteryAlert
    }

    Card(
        modifier = Modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(4.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {

            Row(verticalAlignment = Alignment.CenterVertically) {

                Icon(
                    imageVector = icon,
                    contentDescription = "Battery"
                )

                Spacer(Modifier.width(8.dp))

                Text("Battery: ${battery ?: "--"} %")
            }

            Spacer(Modifier.height(8.dp))

            LinearProgressIndicator(
                progress = level / 100f,
                modifier = Modifier.fillMaxWidth()
            )
        }
    }
}

@Composable
fun MotionCard(time: String) {

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp),
        elevation = CardDefaults.cardElevation(2.dp)
    ) {
        Text(
            text = "Motion Detected: $time",
            modifier = Modifier.padding(16.dp)
        )
    }
}