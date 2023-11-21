package com.tans.stacktrace

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import com.tans.stacktrace.databinding.ActivityMainBinding
import java.lang.StringBuilder


class MainActivity : AppCompatActivity() {

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
    }

    private external fun dumpTestThreadStack(): Array<String>

    companion object {
        init {
            System.loadLibrary("stacktrace")
        }
    }
}