package com.tans.stacktrace

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import com.tans.stacktrace.databinding.ActivityMainBinding
import java.lang.StringBuilder


class MainActivity : AppCompatActivity() {

    init {
        registerCrashMonitor()
    }

    private val binding: ActivityMainBinding by lazy {
        ActivityMainBinding.inflate(layoutInflater)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(binding.root)
        binding.nativeStackBt.setOnClickListener {
            val stacks = dumpTestThreadStack()
            val string = StringBuilder()
            for (s in stacks) {
                string.appendLine(s)
            }
            binding.outputTv.text = string.toString()
        }

        binding.nativeCrashBt.setOnClickListener {
            testCrash()
        }
    }

    private external fun registerCrashMonitor()

    private external fun dumpTestThreadStack(): Array<String>

    private external fun testCrash()

    companion object {
        init {
            System.loadLibrary("stacktrace")
        }
    }
}