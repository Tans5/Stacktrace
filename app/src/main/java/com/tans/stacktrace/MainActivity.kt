package com.tans.stacktrace

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import androidx.annotation.Keep
import com.tans.stacktrace.databinding.ActivityMainBinding
import java.io.File
import java.lang.StringBuilder


class MainActivity : AppCompatActivity() {

    private val binding: ActivityMainBinding by lazy {
        ActivityMainBinding.inflate(layoutInflater)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(binding.root)
        registerCrashMonitor()
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

    private fun registerCrashMonitor() {
        val dir = getExternalFilesDir(null)
        val cacheFile = File(dir, "crash_monitor")
        if (cacheFile.exists()) {
            cacheFile.delete()
        }
        cacheFile.createNewFile()
        registerCrashMonitor(cacheFile.canonicalPath)
    }

    @Keep
    private external fun registerCrashMonitor(cacheFile: String)

    @Keep
    private external fun dumpTestThreadStack(): Array<String>

    @Keep
    private external fun testCrash()

    companion object {
        init {
            System.loadLibrary("stacktrace")
        }
        const val TAG = "Stacktrace_MainActivity"

        @Keep
        @JvmStatic
        fun handleNativeCrash(
            tid: Int,
            sig: Int,
            sigName: String,
            sigSub: Int,
            sigSubName: String,
            time: Long,
            stacks: Array<String>
        ) {
            Log.d(TAG, "Receive Native crash: $sigName")
        }
    }
}