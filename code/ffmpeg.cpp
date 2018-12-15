
extern "C"
{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"
    #include "libavutil/imgutils.h"
    #include "libavutil/avutil.h"
    #include "libavutil/avstring.h"
    #include "libavutil/opt.h"
    #include "libswresample/swresample.h"
}
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")




AVCodecContext *ffmpeg_open_codec(AVFormatContext *fc, int streamIndex)
{
    AVCodecContext *orig = fc->streams[streamIndex]->codec;
    AVCodec *codec = avcodec_find_decoder(orig->codec_id);
    AVCodecContext *result = avcodec_alloc_context3(codec);  // todo: check if this is null?
    if (!codec)
        { OutputDebugString("ffmpeg: Unsupported codec. Yipes."); return false; }
    if (avcodec_copy_context(result, orig) != 0)
        { OutputDebugString("ffmpeg: Codec context copy failed."); return false; }
    if (avcodec_open2(result, codec, 0) < 0)
        { OutputDebugString("ffmpeg: Couldn't open codec."); return false; }
    return result;
}

#define PRINT(msg) { OutputDebugString(msg); }

v2 GetResolution(string path)
{
    // // special case to skip text files for now,
    // // ffmpeg likes to eat these up and then our code doesn't know what to do with them
    // if (StringEndsWith(videopath, ".txt") || StringEndsWith(audiopath, ".txt"))
    // {
    //     LogError("Any txt file not supported yet.");
    //     return {-1,-1};
    // }

    // convert wchar to utf-8 which is what ffmpeg wants
    int numChars = WideCharToMultiByte(
        CP_UTF8,
        0,
        path.chars,
        -1, // input string is null terminated (and output will be as well)
        0, // output buffer
        0, // output size in bytes, if 0 then return num chars and don't use output buffer
        0,
        0
    );
    char *utf8path = (char*)malloc(numChars * sizeof(char));
    WideCharToMultiByte(
        CP_UTF8,
        0,
        path.chars,
        -1, // input string is null terminated (and output will be as well)
        utf8path, // output buffer
        numChars * sizeof(char), // output size in bytes, if 0 then return num chars and don't use output buffer
        0,
        0
    );


    AVFormatContext *vfc = 0;
    AVFormatContext *afc = 0;
    // ffmpeg_stream video;
    // ffmpeg_stream audio;

    int open_result1 = avformat_open_input(&vfc, utf8path, 0, 0);
    int open_result2 = avformat_open_input(&afc, utf8path, 0, 0);
    if (open_result1 != 0 || open_result2 != 0)
    {
        PRINT("Unable to load a format context...\n");
        char averr[1024];
        av_strerror(open_result1, averr, 1024);
        char msg[2048];
        sprintf(msg, "ffmpeg: Can't open file: %s\n", averr);
        PRINT(msg);
        return {-1,-1};
    }

    // todo: need to call avformat_close_input() at some point after avformat_open_input
    // (when no longer using the file maybe?)

    // populate fc->streams
    if (avformat_find_stream_info(vfc, 0) < 0 ||
        avformat_find_stream_info(afc, 0) < 0)
    {
        PRINT("ffmpeg: Can't find stream info in file.");
        return {-1,-1};
    }

    // this must be sending to stdout or something? (not showing up anywhere)
    // av_dump_format(vfc, 0, videopath, 0);

    // find first video and audio stream
    // todo: use av_find_best_stream?
    int video_index = -1;
    for (int i = 0; i < vfc->nb_streams; i++)
    {
        if (vfc->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            if (video_index != -1)
                PRINT("ffmpeg: More than one video stream!");

            if (video_index == -1)
                video_index = i;
        }
    }
    int audio_index = -1;
    for (int i = 0; i < afc->nb_streams; i++)
    {
        if (afc->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
        {
            if (audio_index != -1)
                PRINT("ffmpeg: More than one audio stream!");

            if (audio_index == -1)
                audio_index = i;
        }
    }
    AVCodecContext *vcc;
    AVCodecContext *acc;
    if (video_index != -1)
    {
        vcc = ffmpeg_open_codec(vfc, video_index);
    }
    if (audio_index != -1)
    {
        acc = ffmpeg_open_codec(afc, audio_index);
    }
    //todo: handle less than one or more than one of each properly


    // edit: this doesn't seem to trigger even on a text file?
    // if neither, this probably isn't a video file
    if (video_index == -1 && audio_index == -1)
    {
        PRINT("ffmpeg: No audio or video streams in file.");
        return {-1,-1};
    }

    // try this to catch bad files..
    // edit: also doesn't work
    if (!vcc && !acc)
    {
        PRINT("ffmpeg: No audio or video streams in file.");
        return {-1,-1};
    }

    // if (FFMPEG_LOADING_MSG) PRINT("Populating metadata...\n");
    // PopulateMetadata();

        //fps
        // if (video.codecContext) {
        //     fps = ((double)video.codecContext->time_base.den /
        //            (double)video.codecContext->time_base.num) /
        //            (double)video.codecContext->ticks_per_frame;
        // } else if (audio.codecContext) {
        //     // todo: test this, might get some weird fps from audio
        //     // update: fps is usually too quick on audio steams, just use a default
        //     fps = 30;
        //     // fps = ((double)audio.codecContext->time_base.den /
        //     //        (double)audio.codecContext->time_base.num) /
        //     //        (double)audio.codecContext->ticks_per_frame;
        // } else {
        //     fps = 30; // default if no video or audio.. probably shouldn't get here though
        //     assert(false); // thinking we should have at least one a or v stream
        // }

        // duration
        // if (vfc) {
        //     durationSeconds = vfc->duration / (double)AV_TIME_BASE;
        //     totalFrameCount = durationSeconds * fps;
        // } else if (afc) { //fallback to duration from audio
        //     durationSeconds = afc->duration / (double)AV_TIME_BASE;
        //     totalFrameCount = durationSeconds * fps;
        // } else {
        //     // uuh..
        //     assert(false); // don't call populate metadata until we've loaded the sources
        // }

    v2 result;

        // w / h / aspect_ratio
        if (vcc)
        {
            result.w = vcc->width;
            result.h = vcc->height;
            // aspect_ratio = (double)width / (double)height;
        }
        else
        {
            // todo: what to use here?
            result.w = 400;
            result.h = 400;
            // aspect_ratio = 1;
        }

    return result;

}