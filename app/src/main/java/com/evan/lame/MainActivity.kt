package com.evan.lame

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import com.evan.lame.databinding.ActivityMainBinding
import java.io.File


class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Example of a call to a native method
        binding.sampleText.text = stringFromJNI()

        val mtpResult = Mp3ToPcm(File(cacheDir,"Dream.mp3").absolutePath,File(cacheDir,"test.pcm").absolutePath,true)
        val ptmResult = PcmToMp3(File(cacheDir,"test.pcm").absolutePath,File(cacheDir,"test.mp3").absolutePath,2,0,0)

    }

    /**
     * A native method that is implemented by the 'lame' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    external fun Mp3ToPcm(mp3Path: String,pcmPath: String,writeWanHeader:Boolean):Boolean
    external fun PcmToMp3(pcmPath: String,mp3Path: String,channels:Int,inputRate:Int,outputRate:Int):Boolean

    companion object {
        init {
            System.loadLibrary("lame")
        }
    }
}