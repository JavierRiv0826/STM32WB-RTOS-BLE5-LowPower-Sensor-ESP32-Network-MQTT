package com.example.stm32wb.ui.navigation

import android.os.Build
import androidx.annotation.RequiresApi
import androidx.compose.runtime.Composable
import androidx.navigation.compose.*
import com.example.stm32wb.ui.permission.PermissionScreen
import com.example.stm32wb.ui.sensor.SensorScreen

@RequiresApi(Build.VERSION_CODES.S)
@Composable
fun AppNavGraph() {

    val navController = rememberNavController()

    NavHost(
        navController = navController,
        startDestination = NavRoutes.Permission.route
    ) {

        composable(NavRoutes.Permission.route) {
            PermissionScreen(
                onPermissionGranted = {
                    navController.navigate(NavRoutes.Sensor.route) {
                        popUpTo(NavRoutes.Permission.route) { inclusive = true }
                    }
                }
            )
        }

        composable(NavRoutes.Sensor.route) {
            SensorScreen()
        }
    }
}