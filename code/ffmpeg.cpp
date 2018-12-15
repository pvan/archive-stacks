
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

v2 ffmpeg_GetResolution(string path)
{
    // // special case to skip text files for now,
    // // ffmpeg likes to eat these up and then our code doesn't know what to do with them
    // if (StringEndsWith(videopath, ".txt") || StringEndsWith(audiopath, ".txt"))
    // {
    //     LogError("Any txt file not supported yet.");
    //     return {-1,-1};
    // }


    // convert wchar to utf-8 which is what ffmpeg wants...

    int numChars = WideCharToMultiByte(CP_UTF8,0,  path.chars,-1,  0,0,  0,0);
    char *utf8path = (char*)malloc(numChars*sizeof(char));
    WideCharToMultiByte(CP_UTF8,0,  path.chars,-1,  utf8path,numChars*sizeof(char),  0,0);

    AVFormatContext *vfc = avformat_alloc_context(); // needs avformat_free_context
    int open_result1 = avformat_open_input(&vfc, utf8path, 0, 0); // needs avformat_close_input

    free(utf8path);


    if (open_result1 != 0)
    {
        PRINT("Unable to load a format context...\n");
        char averr[1024];
        av_strerror(open_result1, averr, 1024);
        char msg[2048];
        sprintf(msg, "ffmpeg: Can't open file: %s\n", averr);
        PRINT(msg);
        return {-1,-1};
    }


    // populate fc->streams
    if (avformat_find_stream_info(vfc, 0) < 0)
    {
        PRINT("ffmpeg: Can't find stream info in file.");
        return {-1,-1};
    }

    // this must be sending to stdout or something? (not showing up anywhere)
    // av_dump_format(vfc, 0, videopath, 0);

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
    AVCodecContext *vcc;
    if (video_index != -1)
    {
        vcc = ffmpeg_open_codec(vfc, video_index);
    }
    //todo: handle less than one or more than one of each properly


    // edit: this doesn't seem to trigger even on a text file?
    // if neither, this probably isn't a video file
    if (video_index == -1)
    {
        PRINT("ffmpeg: No video streams in file.");
        return {-1,-1};
    }


    v2 result;

    if (vcc) {
        result.w = vcc->width;
        result.h = vcc->height;
    } else {
        result.w = 400; // todo: what to use here?
        result.h = 400;
    }

    avcodec_free_context(&vcc);
    avformat_close_input(&vfc);
    avformat_free_context(vfc);

    return result;

}