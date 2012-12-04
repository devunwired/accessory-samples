/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define ADK_INTERNAL
#include "fwk.h"
#include "simpleOgg.h"
#include "Audio.h"
#include "coop.h"
#include "f_ff.h"
#include "v_ivorbiscodec.h"
#include "v_ivorbisfile.h"

static size_t oggRead2(void *ptr, size_t size, size_t nmemb, void *data){

    FIL *f = data;
    FRESULT r;
    UINT read = 0;

    read = size * nmemb;
    if(read + f->fptr > f->fsize) read = f->fsize - f->fptr;
    if(!read) return 0;

    r = f_read(f, ptr, size * nmemb, &read);

    if(r) return 0;
    return read / size;
}

static int oggSeek2(void *data, ogg_int64_t offset, int whence){

    return -1;
}

static int oggClose2(void *data){

    FIL *f = data;

    f_close(f);
    return 0;
}

static long oggTell2(void *data){

    FIL *f = data;

    return f->fptr;
}

static char _playOgg(const char *name, char *abort){

    const static ov_callbacks cbks ={ oggRead2, oggSeek2, oggClose2, oggTell2 };
    const uint32_t bufSz = 2048;
    const uint32_t numBufs = 3;
    uint16_t** bufs;
    uint32_t bufIdx = 0;
    int vorbis_state = 0;
    OggVorbis_File F;
    long i, t;
    char ret = SIMPLE_OGG_MEM_ERR;

    FRESULT r;
    FIL f;

    ret = SIMPLE_OGG_OK;

    r = f_open(&f, name, FA_READ);
    if(r) return SIMPLE_OGG_OPEN_ERR;

    i = ov_open_callbacks(&f, &F, NULL, 0, cbks);
    if(i != 0) return SIMPLE_OFF_OGG_OPEN_ERR;

    bufs = malloc(sizeof(uint16_t*[numBufs]));
    if(!bufs) goto errout;

    for(i = 0; i < numBufs; i++) bufs[i] = NULL;
    for(i = 0; i < numBufs; i++) if(!(bufs[i] = malloc(sizeof(uint16_t[bufSz])))) break;
    if(i != numBufs) goto errout;

    vorbis_info *info = ov_info(&F, 0);
    //dbgPrintf("vorbis info: version %d channels %d rate %d\n", info->version, info->channels, info->rate);

    audioOn(AUDIO_ALARM, ov_info(&F, 0)->rate);
    while(!(*abort)){
        uint8_t volume = getVolume();

        i = ov_read(&F, bufs[bufIdx], bufSz * 2, &vorbis_state);
        if(i == 0){
            break;
        } else if(i == OV_HOLE || i == OV_EBADLINK){
            ret = SIMPLE_OGG_FILE_ERR;
            break;
        } else {
            i >>= 1;
            if (info->channels == 1) {
                for (t = 0; t < i; t++){
                    bufs[bufIdx][t] =
                        ((signed long)(((((int16_t) bufs[bufIdx][t]) >> 4) * (signed long)((unsigned long)volume) + 1)) >> 8) + 2048;
                }
            } else {
                // 2 channels, pick the left channel
                i >>= 1;
                for (t = 0; t < i; t++){
                    bufs[bufIdx][t] =
                        ((signed long)(((((int16_t) bufs[bufIdx][t*2]) >> 4) * (signed long)((unsigned long)volume) + 1)) >> 8) + 2048;
                }
            }
        }
        audioAddBuffer(AUDIO_ALARM, bufs[bufIdx], i);
        if(++bufIdx == numBufs) bufIdx = 0;
    }
    bufs[bufIdx][0] = 2048;
    audioAddBuffer(AUDIO_ALARM, bufs[bufIdx], 1);
    ov_clear(&F);
    f_close(&f);
    audioOff(AUDIO_ALARM);

errout:
    for(i = 0; i < numBufs; i++) if(bufs[i]) free(bufs[i]);
    free(bufs);
    return ret;
}

char playOgg(const char *name){
    char abort = 0;
    return _playOgg(name, &abort);
}

struct oggWorkerArgs {
    const char *name;
    char *complete;
    char *abort;
};

static void oggBackgroundWorker(void *_arg)
{
    struct oggWorkerArgs *args = _arg;

    char ret = _playOgg(args->name, args->abort);

    *args->complete = 1;

    free(args);
}

char playOggBackground(const char *name, char *complete, char *abort){

    struct oggWorkerArgs *args = malloc(sizeof(struct oggWorkerArgs));
    if (!args)
        return -1;

    args->name = name;
    args->complete = complete;
    args->abort = abort;
    coopSpawn(&oggBackgroundWorker, args, 12*1024);

    return 0;

}

