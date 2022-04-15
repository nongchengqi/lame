#include <string>
#include <memory.h>
#include <cstdlib>
#include <jni.h>
#include "lamemp3/lame.h"


std::string jstring2str(JNIEnv *env, jstring jstr);

int WriteWaveHeader(FILE *pFile, int i, int samplerate, int stereo, int i1);

static lame_global_flags *lame = NULL;

extern "C" {

JNIEXPORT jstring JNICALL Java_com_evan_lame_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


JNIEXPORT jboolean JNICALL Java_com_evan_lame_MainActivity_PcmToMp3(JNIEnv *env,jobject,jstring pcmFilePath,jstring mp3FilePath,jint sample_num,jint inSamplerate,jint outSamplerate){
    int read, write;
    const int PCM_SIZE = 8192;
    const int MP3_SIZE = 8192;
    short int pcm_buffer[PCM_SIZE*2];
    unsigned char mp3_buffer[MP3_SIZE];

    FILE *pcm = fopen(jstring2str(env, pcmFilePath).c_str(), "rb");
    FILE *mp3 = fopen(jstring2str(env, mp3FilePath).c_str(), "wb");

    lame = lame_init();
    //输入采样率
    if (inSamplerate > 16000) {
        lame_set_in_samplerate(lame, inSamplerate); //必须和录制的一致
    }
    //声道数
    lame_set_num_channels(lame, sample_num);
    if (outSamplerate > 0) {
        //输出采样率
        lame_set_out_samplerate(lame, outSamplerate);
    }
    //音频质量
    //lame_set_quality(lame, quality);
    //
    lame_set_VBR(lame, vbr_default);
    //初始化参数配置
    lame_init_params(lame);

    do {
        read = fread(pcm_buffer,2* sizeof(short int ),PCM_SIZE ,pcm);
        if (read == 0){
            write = lame_encode_flush(lame,mp3_buffer,MP3_SIZE);
        } else {
            write = lame_encode_buffer_interleaved(lame, pcm_buffer, read, mp3_buffer, MP3_SIZE);
        }
        fwrite(mp3_buffer, write, 1, mp3);
    } while (read != 0);
    lame_mp3_tags_fid(lame, mp3);
    lame_close(lame);
    fclose(mp3);
    fclose(pcm);

    return true;
}


JNIEXPORT jboolean JNICALL Java_com_evan_lame_MainActivity_Mp3ToPcm(
        JNIEnv *env,
        jobject, jstring mp3Path, jstring pcmPath,jboolean writeWavHeader) {
    int read, i, samples;

    long cumulative_read = 0;

    const int PCM_SIZE = 8192;
    const int MP3_SIZE = 8192;

    // 输出左右声道
    short int pcm_l[PCM_SIZE], pcm_r[PCM_SIZE];
    unsigned char mp3_buffer[MP3_SIZE];
    //input输入MP3文件
    FILE *mp3 = fopen(jstring2str(env, mp3Path).c_str(), "rb");
    FILE *pcm = fopen(jstring2str(env, pcmPath).c_str(), "wb");
    fseek(mp3, 0, SEEK_SET);
    lame = lame_init();
    lame_set_decode_only(lame, 1);
    hip_t hip = hip_decode_init();

    mp3data_struct mp3data;
    memset(&mp3data, 0, sizeof(mp3data));

    //忽略前面528个数据，因为都是0
    int ignore = 528;
    int nChannels = -1;
    long wavsize = 0;
    int mp3_len;
    while ((read = fread(mp3_buffer, sizeof(char), MP3_SIZE, mp3)) > 0) {
        mp3_len = read;
        cumulative_read += read * sizeof(char);
        do {
            samples = hip_decode1_headers(hip, mp3_buffer, mp3_len, pcm_l, pcm_r, &mp3data);
            wavsize += samples;
            if (mp3data.header_parsed == 1) {
                if (nChannels < 0)//reading for the first time
                    nChannels = mp3data.stereo;
            }
            if (samples > 0) {
                for (i = 0; i < samples; i++) {
                    if (ignore >= 0) {
                        ignore = ignore - 1;
                        continue;
                    }
                    fwrite((char *) &pcm_l[i], sizeof(char), sizeof(pcm_l[i]), pcm);
                    if (nChannels == 2) {
                        fwrite((char *) &pcm_r[i], sizeof(char), sizeof(pcm_r[i]), pcm);
                    }
                }
            }
            mp3_len = 0;
        } while (samples > 0);
    }

    //是否需要头部
    if (writeWavHeader) {
        if (wavsize < 0) {
            wavsize = 0;
        } else {
            wavsize = (wavsize - 528) * (16 / 8) * nChannels;
        }

        fseek(pcm, 0, SEEK_SET);
        WriteWaveHeader(pcm, wavsize, lame_get_in_samplerate(lame), mp3data.stereo, 16);
    }

    hip_decode_exit(hip);
    lame_close(lame);
    fclose(mp3);
    fclose(pcm);

    return JNI_TRUE;
}
}



std::string jstring2str(JNIEnv *env, jstring jstr) {
    char *rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("GB2312");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray) env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte *ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char *) malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    std::string stemp(rtn);
    free(rtn);
    return stemp;
}



static void
write_16_bits_low_high(FILE * fp, int val)
{
    unsigned char bytes[2];
    bytes[0] = (val & 0xff);
    bytes[1] = ((val >> 8) & 0xff);
    fwrite(bytes, 1, 2, fp);
}

static void
write_32_bits_low_high(FILE * fp, int val)
{
    unsigned char bytes[4];
    bytes[0] = (val & 0xff);
    bytes[1] = ((val >> 8) & 0xff);
    bytes[2] = ((val >> 16) & 0xff);
    bytes[3] = ((val >> 24) & 0xff);
    fwrite(bytes, 1, 4, fp);
}

int WriteWaveHeader(FILE * const fp, int pcmbytes, int freq, int channels, int bits)
{
    int  bytes = (bits + 7) / 8;

    /* quick and dirty, but documented */
    fwrite("RIFF", 1, 4, fp); /* label */
    write_32_bits_low_high(fp, pcmbytes + 44 - 8); /* length in bytes without header */
    fwrite("WAVEfmt ", 2, 4, fp); /* 2 labels */
    write_32_bits_low_high(fp, 2 + 2 + 4 + 4 + 2 + 2); /* length of PCM format declaration area */
    write_16_bits_low_high(fp, 1); /* is PCM? */
    write_16_bits_low_high(fp, channels); /* number of channels */
    write_32_bits_low_high(fp, freq); /* sample frequency in [Hz] */
    write_32_bits_low_high(fp, freq * channels * bytes); /* bytes per second */
    write_16_bits_low_high(fp, channels * bytes); /* bytes per sample time */
    write_16_bits_low_high(fp, bits); /* bits per sample */
    fwrite("data", 1, 4, fp); /* label */
    write_32_bits_low_high(fp, pcmbytes); /* length in bytes of raw PCM data */

    return ferror(fp) ? -1 : 0;
}