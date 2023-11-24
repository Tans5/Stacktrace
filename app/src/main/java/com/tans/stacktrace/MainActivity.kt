package com.tans.stacktrace

import android.app.ActivityManager
import android.app.ActivityManager.RunningAppProcessInfo
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Looper
import android.os.Process
import android.util.Log
import androidx.annotation.Keep
import androidx.core.content.getSystemService
import com.tans.stacktrace.databinding.ActivityMainBinding
import java.io.File
import java.io.FileReader
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

        binding.nativeCrashNewThreadBt.setOnClickListener {
            Thread({
                   testCrash()
            }, "TestCrashThread").start()
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
        private const val TAG = "Stacktrace_MainActivity"

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
            val crashThreadName = getThreadNameByTid(tid)
            val crashThread = findThreadByTid(tid)
            val currentProcessInfo = getCurrentProcess()
            val title = "Fatal signal $sig ($sigName), code $sigSub ($sigSubName) in tid $tid ($crashThreadName), pid ${Process.myPid()} (${currentProcessInfo?.processName ?: "Unknown"})"
            Log.d(TAG, title)
            for (s in stacks) {
                Log.d(TAG, s)
            }
            val javaStackTraces = crashThread?.stackTrace
            val javaStacks = javaStackTraces?.map {
                "at $it"
            } ?: emptyList()
            for (s in javaStacks) {
                Log.d(TAG, s)
            }
        }

        private fun getThreadNameByTid(tid: Int): String {
            val file = File("/proc/$tid/comm")
            return try {
                FileReader(file).use {
                    it.readText()
                }.trim()
            } catch (e: Throwable) {
                "Unknown"
            }
        }

        private fun findThreadByTid(tid: Int): Thread? {
            val pid = Process.myPid()
            return if (pid == tid) {
                // MainThread
                Looper.getMainLooper().thread
            } else {
                val name = getThreadNameByTid(tid)
                getAllThread().find { it.name == name }
            }
        }

        private fun getCurrentProcess(): RunningAppProcessInfo? {
            val app = MyApplication.app!!
            val am = app.getSystemService<ActivityManager>()
            val pid = Process.myPid()
            return am?.runningAppProcesses?.find { it.pid == pid }
        }

        private fun getAllThread(): Array<Thread> {
            var group = Looper.getMainLooper().thread.threadGroup
            var topGroup = group
            while (group != null) {
                topGroup = group
                group = topGroup.parent
            }
            return if (topGroup != null) {
                val activeCount = topGroup.activeCount()
                val threads: Array<Thread?> = Array(activeCount * 2) { null }
                val realCount = topGroup.enumerate(threads)
                System.arraycopy(threads, 0, threads, 0, realCount)
                threads.mapNotNull { it }.toTypedArray()
            } else {
                emptyArray()
            }
        }
    }
}