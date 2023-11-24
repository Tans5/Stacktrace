package com.tans.stacktrace

import android.app.Application

class MyApplication : Application() {


    override fun onCreate() {
        super.onCreate()
        app = this
    }

    companion object {
        var app: Application? = null
    }
}