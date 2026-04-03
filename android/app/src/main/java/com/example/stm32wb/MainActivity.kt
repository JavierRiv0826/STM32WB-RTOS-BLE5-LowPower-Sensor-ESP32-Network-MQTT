package com.example.stm32wb

import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.annotation.RequiresApi
import dagger.hilt.android.AndroidEntryPoint
import com.example.stm32wb.ui.navigation.AppNavGraph
import com.example.stm32wb.ui.theme.STM32WBTheme

@AndroidEntryPoint
class MainActivity : ComponentActivity() {

    @RequiresApi(Build.VERSION_CODES.S)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContent {
            STM32WBTheme {
                AppNavGraph()
            }
        }
    }
}