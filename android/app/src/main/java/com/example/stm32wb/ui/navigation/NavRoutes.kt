package com.example.stm32wb.ui.navigation

sealed class NavRoutes(val route: String) {
    object Permission : NavRoutes("permission")
    object Sensor : NavRoutes("sensor")
}