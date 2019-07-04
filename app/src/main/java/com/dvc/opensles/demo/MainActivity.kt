package com.dvc.opensles.demo

import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.view.View
import android.widget.Toast

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
    }

    external fun playpcm(url : String?, samplingRate :Int, tracts : Int)
    external fun setVolume(percent:Int)
    external fun start():Boolean
    external fun pause():Boolean
    external fun destory();

    fun onLoad(view : View) {
        val url:String? = "/sdcard/16k.pcm"
        val samplingRate:Int = 16000
        val tracts:Int = 1
        playpcm(url, samplingRate, tracts)
    }

    fun onPlay(view: View){
        if(!start()) {
            Toast.makeText(applicationContext, "还没有载入资源", Toast.LENGTH_SHORT).show()
        }
    }
    fun onPause(view: View) = pause()

    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}
