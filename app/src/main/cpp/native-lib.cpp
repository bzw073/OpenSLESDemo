#include <jni.h>
#include <string>
#include <malloc.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>

#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"dvc",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"dvc",FORMAT,##__VA_ARGS__);

FILE* pcmFp = NULL;
void *out_buffer = NULL;
void *buffer = NULL;
int mSamplingRate;
int mTract;

SLObjectItf pEngine = NULL;
SLEngineItf engineItf = NULL;

SLObjectItf pMix = NULL;
SLEnvironmentalReverbItf slEnvironmentalReverbItf;
SLEnvironmentalReverbSettings slEnvironmentalReverbSettings;

SLObjectItf pPlayer = NULL;
SLPlayItf slPlayerItf;

SLAndroidSimpleBufferQueueItf slBufferQueueItf;

SLVolumeItf slVolumeItf;

int volumePercent;

SLuint32 getSLSamplingRate(jint rate);

int getPcmData(void **buf) {
    int size = 0;
    if(!feof(pcmFp)) {
        size = fread(out_buffer, 1, mSamplingRate*mTract*2, pcmFp);
        if(out_buffer == NULL) {
            LOGI("read end");
            return 0;
        }
        LOGI("reading:%d",size);
        *buf = out_buffer;
    }
    return size;
}

//回调方法
void bufferCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    int size = getPcmData(&buffer);
    if(buffer != NULL) {
        (*slBufferQueueItf)->Enqueue(slBufferQueueItf, buffer, size);
    }
}

SLuint32 getSLSamplingRate(jint rate) {
    switch (rate) {
        case 8000:
            return SL_SAMPLINGRATE_8;
        case 11025:
            return SL_SAMPLINGRATE_11_025;
        case 12000:
            return SL_SAMPLINGRATE_12;
        case 16000:
            return SL_SAMPLINGRATE_16;
        case 22050:
            return SL_SAMPLINGRATE_22_05;
        case 24000:
            return SL_SAMPLINGRATE_24;
        case 32000:
            return SL_SAMPLINGRATE_32;
        case 44100:
            return SL_SAMPLINGRATE_44_1;
        case 48000:
            return SL_SAMPLINGRATE_48;
        case 64000:
            return SL_SAMPLINGRATE_64;
        case 88200:
            return SL_SAMPLINGRATE_88_2;
        case 96000:
            return SL_SAMPLINGRATE_96;
        case 19200:
            return SL_SAMPLINGRATE_192;
    }
    return SL_SAMPLINGRATE_44_1;
}

extern "C" JNIEXPORT void JNICALL
Java_com_dvc_opensles_demo_MainActivity_playpcm(
        JNIEnv *env,
        jobject /* this */,
        jstring url_,
        jint samplingRate,
        jint tract
        ) {
    const char *url = env->GetStringUTFChars(url_, 0);
    mSamplingRate = samplingRate;
    mTract = tract;
    if((pcmFp = fopen(url, "r")) == NULL) {
        return;
    }

    out_buffer = malloc(samplingRate*tract*2);

    SLresult result;
    if(pEngine == NULL) {
        // 创建引擎对象
        slCreateEngine(&pEngine, 0, 0, 0, 0, 0);
        result = (*pEngine)->Realize(pEngine, SL_BOOLEAN_FALSE);
        LOGI("pEngineRealizeResult:%d", result);
        result = (*pEngine)->GetInterface(pEngine,SL_IID_ENGINE, &engineItf);
        LOGI("pEngineGetEngineItfResult:%d", result);

        // 创建混音器
        const SLInterfaceID slInterfaceID[1] = {SL_IID_ENVIRONMENTALREVERB};
        const SLboolean sLboolean[1] = {SL_BOOLEAN_TRUE};
        result = (*engineItf)->CreateOutputMix(engineItf, &pMix, 1, slInterfaceID, sLboolean);
        LOGI("engineCreateOutputMixResult:%d", result);
        result = (*pMix)->Realize(pMix, SL_BOOLEAN_FALSE);
        result = (*pMix)->GetInterface(pMix, SL_IID_ENVIRONMENTALREVERB, &slEnvironmentalReverbItf);
        if (SL_RESULT_SUCCESS == result) {
            result = (*slEnvironmentalReverbItf)->SetEnvironmentalReverbProperties(slEnvironmentalReverbItf, &slEnvironmentalReverbSettings);
        }
        SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, pMix};

        // 创建播放器
        SLDataLocator_AndroidSimpleBufferQueue androidSimpleBufferQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
        SLDataFormat_PCM dataFormat_pcm = {
                SL_DATAFORMAT_PCM,
                tract,
                getSLSamplingRate(samplingRate),
                SL_PCMSAMPLEFORMAT_FIXED_16,
                SL_PCMSAMPLEFORMAT_FIXED_16,
                (tract == 2 ? SL_SPEAKER_TOP_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT : SL_SPEAKER_FRONT_CENTER),
                SL_BYTEORDER_LITTLEENDIAN
        };

        SLDataSource slDataSource = {&androidSimpleBufferQueue, &dataFormat_pcm};
        SLDataSink slDataSink = {&outputMix, NULL};
        const SLInterfaceID sId[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
        const SLboolean sBoolean[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
        result = (*engineItf)->CreateAudioPlayer(engineItf, &pPlayer, &slDataSource, &slDataSink, 3, sId, sBoolean);
        result = (*pPlayer)->Realize(pPlayer,SL_BOOLEAN_FALSE);
        result = (*pPlayer)->GetInterface(pPlayer, SL_IID_PLAY, &slPlayerItf);

        // 创建缓冲区和回调函数
        result = (*pPlayer)->GetInterface(pPlayer, SL_IID_BUFFERQUEUE, &slBufferQueueItf);
        //注册缓冲接口回调
        result = (*slBufferQueueItf)->RegisterCallback(slBufferQueueItf, bufferCallback, NULL);
        //获取音量接口
        result = (*pPlayer)->GetInterface(pPlayer, SL_IID_VOLUME, &slVolumeItf);
    }
    bufferCallback(slBufferQueueItf, NULL);

    env->ReleaseStringUTFChars(url_, url);
}

extern "C" JNIEXPORT jboolean JNICALL
        Java_com_dvc_opensles_demo_MainActivity_start(
                JNIEnv *env,
                jobject /* this */) {
    if( slPlayerItf == NULL) return false;
    if(feof(pcmFp)) {
        (*slBufferQueueItf)->Clear(slBufferQueueItf);
        fseek(pcmFp, 0, SEEK_SET);
        bufferCallback(slBufferQueueItf, NULL);
    }
    (*slPlayerItf)->SetPlayState(slPlayerItf, SL_PLAYSTATE_PLAYING);
    return true;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_dvc_opensles_demo_MainActivity_pause(
        JNIEnv *env,
        jobject /* this */) {
    if( slPlayerItf == NULL) return false;
    (*slPlayerItf)->SetPlayState(slPlayerItf, SL_PLAYSTATE_PAUSED);
    return true;
}

extern "C" JNIEXPORT void JNICALL
Java_com_dvc_opensles_demo_MainActivity_destory(
        JNIEnv *env,
        jobject /* this */) {

    if(pPlayer != NULL) {
        (*slPlayerItf)->SetPlayState(slPlayerItf, SL_PLAYSTATE_STOPPED);
        (*pPlayer)->Destroy(pPlayer);
        pPlayer = NULL;
        slPlayerItf = NULL;
        slBufferQueueItf = NULL;
        slVolumeItf = NULL;
    }

    if(pMix != NULL) {
        (*pMix)->Destroy(pMix);
        pMix = NULL;
        slEnvironmentalReverbItf = NULL;
    }

    if(pEngine != NULL) {
        (*pEngine)->Destroy(pEngine);
        pEngine = NULL;
        engineItf = NULL;
    }

    free(out_buffer);
}

//extern "C" JNIEXPORT void JNICALL
//Java_com_dvc_opensles_demo_MainActivity_setVolume(
//        JNIEnv *env,
//        jobject /* this */,
//        jint percent) {
//    volumePercent = percent;
//    if(slVolumeItf != NULL)
//    {
//        if(percent > 30)
//        {
//            (*slVolumeItf)->SetVolumeLevel(slVolumeItf, (100 - percent) * -20);
//        }
//        else if(percent > 25)
//        {
//            (*slVolumeItf)->SetVolumeLevel(slVolumeItf, (100 - percent) * -22);
//        }
//        else if(percent > 20)
//        {
//            (*slVolumeItf)->SetVolumeLevel(slVolumeItf, (100 - percent) * -25);
//        }
//        else if(percent > 15)
//        {
//            (*slVolumeItf)->SetVolumeLevel(slVolumeItf, (100 - percent) * -28);
//        }
//        else if(percent > 10)
//        {
//            (*slVolumeItf)->SetVolumeLevel(slVolumeItf, (100 - percent) * -30);
//        }
//        else if(percent > 5)
//        {
//            (*slVolumeItf)->SetVolumeLevel(slVolumeItf, (100 - percent) * -34);
//        }
//        else if(percent > 3)
//        {
//            (*slVolumeItf)->SetVolumeLevel(slVolumeItf, (100 - percent) * -37);
//        }
//        else if(percent > 0)
//        {
//            (*slVolumeItf)->SetVolumeLevel(slVolumeItf, (100 - percent) * -40);
//        }
//        else{
//            (*slVolumeItf)->SetVolumeLevel(slVolumeItf, (100 - percent) * -100);
//        }
//    }
//}