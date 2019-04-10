
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



#define PRINT(msg) { OutputDebugString(msg); }



// // The flush packet is a non-NULL packet with size 0 and data NULL
// void ffmpeg_decode(AVCodecContext *avctx, AVFrame *frame, bool *got_frame, AVPacket *pkt)
// {
    // *got_frame = false;

    // int ret = avcodec_send_packet(avctx, pkt);
    // if (ret < 0) {
    //     PRINT("ffmpeg: Error sending a packet for decoding\n");
    //     return;
    // }

    // while (ret >= 0) {
    //     ret = avcodec_receive_frame(avctx, frame);
    //     if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    //         return;
    //     else if (ret < 0) {
    //         PRINT("ffmpeg: Error during decoding\n");
    //         return;
    //     }

    // }
    // *got_frame = true;

    // int ret;

    // *got_frame = false;

    // if (pkt) {
    //     ret = avcodec_send_packet(avctx, pkt);
    //     // In particular, we don't expect AVERROR(EAGAIN), because we read all
    //     // decoded frames with avcodec_receive_frame() until done.
    //     if (ret < 0)
    //         return ret == AVERROR_EOF ? 0 : ret;
    // }

    // ret = avcodec_receive_frame(avctx, frame);
    // if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
    //     return ret;
    // if (ret >= 0)
    //     *got_frame = true;

    // return 0;
// }


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


v2 ffmpeg_GetResolution(string path)
{
    // // special case to skip text files for now,
    // // ffmpeg likes to eat these up and then our code doesn't know what to do with them
    // if (StringEndsWith(videopath, ".txt") || StringEndsWith(audiopath, ".txt"))
    // {
    //     LogError("Any txt file not supported yet.");
    //     return {-1,-1};
    // }

    if (path.ends_with(L".txt")) {
        // todo: special case to skip text files for now,
        // ffmpeg likes to eat these up and then our code doesn't know what to do with them
        return {-1,-1};
    }

    char *utf8path = path.to_utf8_new_memory();

    AVFormatContext *vfc = avformat_alloc_context(); // needs avformat_free_context
    int open_result1 = avformat_open_input(&vfc, utf8path, 0, 0); // needs avformat_close_input

    free(utf8path);


    if (open_result1 != 0)
    {
        return {-1,-1}; // probably not a file ffmpeg can handle (txt, lnk, zip, etc)
        // PRINT("Unable to load a format context...\n");
        // char averr[1024];
        // av_strerror(open_result1, averr, 1024);
        // char msg[2048];
        // sprintf(msg, "ffmpeg: Can't open file: %s\n", averr);
        // PRINT(msg);
        // return {-1,-1};
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
        DEBUGPRINT("ffmpeg: No video streams in file. %s\n", path.to_utf8_reusable());
        return {-1,-1};
    }


    // string temp = string::Create((char*)vfc->iformat->name);
    // PRINT((char*)vfc->iformat->name);

        // if (!vfc->iformat || vfc->iformat==nullptr) { return {-1,-1}; } // assume not a video

        // // TODO: add to this list all formats we don't want to send to gpu every frame
        // string definitely_static_image_formats[] = {
        //     string::Create(L"image2"),
        //     string::Create(L"png_pipe"),
        //     string::Create(L"bmp_pipe"),
        //     // todo: add way to detect missing formats (e.g. check if getframe is never changing or something)
        // };
        // int length_of_formats = sizeof(definitely_static_image_formats)/sizeof(definitely_static_image_formats[0]);

        // for (int i = 0; i < length_of_formats; i++) {
        //     if (string::Create((char*)vfc->iformat->name) == definitely_static_image_formats[i]) {
        //         return true;
        //     }
        // }
        // return false;


    v2 result;

    if (vcc) {
        result.w = vcc->width;
        result.h = vcc->height;
    } else {
        // if no vcc, we can't get a resolution
        PRINT("ffmpeg: No video context in file.");
        return {-1,-1};
    }

    avcodec_free_context(&vcc);
    avformat_close_input(&vfc);
    avformat_free_context(vfc);

    return result;

}


void ffmpeg_init() {
    av_register_all();  // all formats and codecs
}


// a file loaded by ffmpeg
struct ffmpeg_media {


    AVFormatContext *vfc;
    AVCodecContext *vcc;
    int video_stream_index = 0;

    struct SwsContext *sws_context; // will be unique to the particular input/output size wanted
    AVFrame *out_frame; // output buffer, basically
    u8 *out_frame_mem; // memory to use with out_frame
    int out_frame_wid;
    int out_frame_hei;

    // metadata
    double fps = 0;
    double durationSeconds = 0;
    double totalFrameCount = 0;

    bool loaded = false;

    bool getting_frame_lock = false; // wait until false before unloading item


    // these set up the output chain, basically
    // the sws_context needed for the sws_scale call
    // that converts the (probablly planar YUV) default format to the RGB we want
    // and the out_frame AVFrame we use (just to write to) with the sws_scale call
    // todo: these two should only ever be called together, combine?
    void SetupSwsContex(int w, int h)
    {
        if (sws_context) sws_freeContext(sws_context);

        sws_context = {0};

        if (vcc)
        {
            // add this workaround to bypass sws_scale's no-resize special case
            // that seems to be causing extra band on right when width isn't multiple of 8
            // can non-multiple of 8 height also cause this?
            // more info here: https://lists.ffmpeg.org/pipermail/libav-user/2012-July/002451.html
            // the other thing that seems to be unique to vids with the extra bar
            // is a linesize[0] > width in the codecContext and the decoded frame linesize similarly larger
            int non_multiple_of_8_workaround = 0;
            if (w % 8 != 0)
                non_multiple_of_8_workaround = SWS_ACCURATE_RND;

            sws_context = sws_getCachedContext(
                sws_context,
                vcc->width,
                vcc->height,
                vcc->pix_fmt,
                w,
                h,
                AV_PIX_FMT_RGB32,
                non_multiple_of_8_workaround |
                SWS_POINT,  // since src & dst should be the same size, just use linear
                0, 0, 0);
        }
    }
    void SetupFrameOutput(int w, int h)
    {
        if (out_frame) av_frame_free(&out_frame);
        out_frame = av_frame_alloc();  // just metadata

        if (!out_frame) {
            PRINT("ffmpeg: Couldn't alloc output frame");
            return;
        }

        // actual mem for frame
        // int numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, w,h); //deprecated
        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32, w,h,1);
        if (out_frame_mem) av_free(out_frame_mem);
        out_frame_mem = (u8*)av_malloc(numBytes);

        out_frame->width = w;
        out_frame->height = h;

        // set up frame to use buffer memory...
        av_image_fill_arrays(
            out_frame->data,
            out_frame->linesize,
            out_frame_mem,
            AV_PIX_FMT_RGB32,
            w, h, 1);

        out_frame_wid = w;
        out_frame_hei = h;
    }



    void Unload() {
        if (loaded) {
            if (getting_frame_lock) return; // try again next frame
            avcodec_free_context(&vcc);
            avformat_close_input(&vfc);
            avformat_free_context(vfc);
            if (sws_context) sws_freeContext(sws_context);
            if (out_frame) av_frame_free(&out_frame);
            if (out_frame_mem) av_free(out_frame_mem);
            vcc = 0;
            vfc = 0;
            sws_context = 0;
            out_frame = 0;
            out_frame_mem = 0;
            loaded = false;

            // vars below
            msRuntimeSoFar = 0;
            fetched_at_least_one_frame = false;
            if (cached_frame.data) {
                free(cached_frame.data);
                cached_frame = {0};
            }
            // PRINT("unloaded media successfully\n");
        }
    }

    void LoadFromFile(string path) {
        if (loaded) {
            PRINT("ffmpeg: object already loaded...\n");
            return;
        }


        char *utf8path = path.to_utf8_new_memory();

        vfc = avformat_alloc_context(); // needs avformat_free_context
        int open_result1 = avformat_open_input(&vfc, utf8path, 0, 0); // needs avformat_close_input

        free(utf8path);


        if (open_result1 != 0) {
            PRINT("Unable to load a format context...\n");
            char averr[1024];
            av_strerror(open_result1, averr, 1024);
            char msg[2048];
            sprintf(msg, "ffmpeg: Can't open file: %s\n", averr);
            PRINT(msg);
            return;
        }


        // populate fc->streams
        if (avformat_find_stream_info(vfc, 0) < 0) {
            PRINT("ffmpeg: Can't find stream info in file.");
            return;
        }

        // this must be sending to stdout or something? (not showing up anywhere)
        // av_dump_format(vfc, 0, videopath, 0);

        // todo: use av_find_best_stream?
        video_stream_index = -1;
        for (int i = 0; i < vfc->nb_streams; i++) {
            if (vfc->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
                if (video_stream_index != -1)
                    PRINT("ffmpeg: More than one video stream!");

                if (video_stream_index == -1)
                    video_stream_index = i;
            }
        }

        if (video_stream_index != -1) {
            vcc = ffmpeg_open_codec(vfc, video_stream_index);
        }
        //todo: handle less than one or more than one of each properly


        // edit: this doesn't seem to trigger even on a text file?
        // if neither, this probably isn't a video file
        if (video_stream_index == -1) {
            PRINT("ffmpeg: No video streams in file.");
            return;
        }

        float width = vcc->width;
        float height = vcc->height;

        fps = ((double)vcc->time_base.den /
               (double)vcc->time_base.num) /
               (double)vcc->ticks_per_frame;

        // not even going to set this for now, until we need it for something
        // // looks like vfc->duration it not going to be set on most gifs/static images
        // if (vfc->duration < 0) durationSeconds = 0; else
        // durationSeconds = (double)vfc->duration / (double)AV_TIME_BASE;
        // totalFrameCount = durationSeconds * fps;


        SetupSwsContex(width, height);
        SetupFrameOutput(width, height);

        loaded = true;
    }




    bitmap GetNextFrame(double *outMsToPlayThisFrame) {
        if (!vfc)  {
            assert(false); // we should only be trying to get a frame on a .loaded media file
            bitmap result = {0};
            return result;
        }
        // if (alreadygotone)
        //     return cached_frame;

        // double msOfDesiredFrame = msRuntimeSoFar + dt;

        getting_frame_lock = true;

        // bitmap result = {0};
        AVFrame *frame = av_frame_alloc();  // just metadata
        AVPacket packet;
        av_init_packet(&packet);

        get_another_frame:
        send_another_packet:
        int ret = av_read_frame(vfc, &packet);  // for video, i think always 1packet=1frame

        if (ret == AVERROR_EOF) {
            packet.data = 0; // send null packet
            packet.size = 0;
            // avcodec_send_packet(vcc, 0); // flush packet
            // avcodec_receive_frame(vcc, frame);
            // avcodec_flush_buffers(vcc);
        } else if (ret < 0 && ret != AVERROR(EAGAIN)) {
            char buf[256];
            av_strerror(ret, buf, 256);
            DEBUGPRINT("ffmpeg: error reading packet: %s\n", buf);
        }
        // if (ret < 0 && ret != AVERROR_EOF) {
        //     PRINT("ffmpeg: error getting packet\n");
        // } else {
        //     if (ret == AVERROR_EOF) { packet->size = 0; packet->data = 0; } // flush packet

            ret = avcodec_send_packet(vcc, &packet);
        //     if (ret != AVERROR(EAGAIN)) goto: send_another_packet;
        // }
        if (ret == AVERROR_EOF) {

        } else if (ret < 0 && ret != AVERROR(EAGAIN)) {
            char buf[256];
            av_strerror(ret, buf, 256);
            DEBUGPRINT("ffmpeg: error sending packet: %s\n", buf);
        }


        ret = avcodec_receive_frame(vcc, frame);
        if (ret == AVERROR_EOF) {
            avcodec_flush_buffers(vcc); // i think?
            av_seek_frame(vfc, -1, 0, AVSEEK_FLAG_BACKWARD); // seek to first frame
            msRuntimeSoFar = 0;
        } else if (ret < 0 && ret != AVERROR(EAGAIN)) {
            char buf[256];
            av_strerror(ret, buf, 256);
            DEBUGPRINT("ffmpeg: error receiving frame: %s\n", buf);
        }
        // if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
        //     //error decoding
        if (ret == 0)
        {
            *outMsToPlayThisFrame = 1000.0 *
                frame->pts *  // frame->best_effort_timestamp *
                vfc->streams[video_stream_index]->time_base.num /
                vfc->streams[video_stream_index]->time_base.den;

            sws_scale(
                sws_context,
                (u8**)frame->data,
                frame->linesize,
                0,
                frame->height,
                out_frame->data,
                out_frame->linesize);
        }


        // send_another_packet:
        // int ret = av_read_frame(vfc, packet);  // for video, i think always 1packet=1frame
        // if (ret >= 0)// || ret == AVERROR_EOF)
        // {
        //     // if (ret == AVERROR_EOF) { packet->size = 0; packet->data = 0; } // flush packet

        //     bool got_frame = false;
        //     ffmpeg_decode(vcc, frame, &got_frame, packet);
        //     if (got_frame) {

        //         sws_scale(
        //             sws_context,
        //             (u8**)frame->data,
        //             frame->linesize,
        //             0,
        //             frame->height,
        //             out_frame->data,
        //             out_frame->linesize);

        //     }
        //     else {
        //         OutputDebugString("ffmpeg: couldn't get frame!\n");
        //         // goto send_another_packet;  //need this?
        //     }
        // }

        av_packet_unref(&packet);
        av_frame_free(&frame);

        bitmap bitmap_in_ffmpeg_memory = {0};
        // bitmap_in_ffmpeg_memory.data = (u32*)out_frame->data; // doesn't work when we try to copy
        // bitmap_in_ffmpeg_memory.w = out_frame->width;
        // bitmap_in_ffmpeg_memory.h = out_frame->height;
        bitmap_in_ffmpeg_memory.data = (u32*)out_frame_mem;
        bitmap_in_ffmpeg_memory.w = out_frame_wid;
        bitmap_in_ffmpeg_memory.h = out_frame_hei;


        // i feel like copying this is not ideal
        // esp since we already remixed it to get it into rgb
        // maybe we could somehow pass the remixed directly to gpu
        // before it gets freed or overwritten.. hmm
        // cached_frame = bitmap_in_ffmpeg_memory.NewCopy();
        bitmap result_frame = bitmap_in_ffmpeg_memory.NewCopy();

        // alreadygotone = true;

        getting_frame_lock = false;

        // return cached_frame;
        return result_frame;
    }


    bitmap cached_frame = {0};
    bool alreadygotone = false;

    double msRuntimeSoFar = 0;
    double msPerFrame = 0;
    double msOfLastFrame = 0;

    bool fetched_at_least_one_frame = false;

    bitmap GetFrame(double dt) {

        msPerFrame = 1000.0 / fps;

        // if our msRuntimeSoFar is way ahead of msOfLastFrame
        // we will just speed through the movie as fast as possible, 1 frame at a time
        // instead we should discard frames (maybe inside GetNextFrame, or a new function GetFrameAt(timestamp))
        // until we get one with the timestamp we want
        // likewise, if we're behind somehow we should adapt
        // if just a frame or two behind, we can probably just repeat a frame
        // but if we're far behind, we should seek backwards instead of just freezing on a single frame
        msRuntimeSoFar += dt;
        if (msRuntimeSoFar > msOfLastFrame + msPerFrame || !fetched_at_least_one_frame) {
            // time to get a new frame
            double msToPlayThisFrame;
            if (cached_frame.data) free(cached_frame.data);
            cached_frame = GetNextFrame(&msToPlayThisFrame);
            msOfLastFrame = msToPlayThisFrame;
        }
        //     return cached_frame;
        // } else
        // {
        //     // repeat last frame
        //     return cached_frame;
        // }
        fetched_at_least_one_frame = true;
        return cached_frame;

    }



    bool IsStaticImageBestGuess() {
        if (!vfc)  {
            assert(false); // file not loaded yet, for now just don't do this
        }

        if (!vfc->iformat || vfc->iformat==nullptr) { return true; } // assume not a video

        // TODO: add to this list all formats we don't want to send to gpu every frame
        static string definitely_static_image_formats[] = {
            string::create_using_passed_in_memory(L"image2"),
            string::create_using_passed_in_memory(L"png_pipe"),
            string::create_using_passed_in_memory(L"bmp_pipe"),
            string::create_using_passed_in_memory(L"jpeg_pipe"),
            // todo: add way to detect missing formats (e.g. check if getframe is never changing or something)
        };
        int length_of_formats = sizeof(definitely_static_image_formats)/sizeof(definitely_static_image_formats[0]);

        for (int i = 0; i < length_of_formats; i++) {
            if (string::create_temporary((char*)vfc->iformat->name) == definitely_static_image_formats[i]) {
                return true;
            }
        }
        return false;

    }



};



void DownresFileAtPathToPath(string inpath, string outpath) {

    // ffmpeg -i input.jpg -vf "scale='min(320,iw)':'min(240,ih)'" input_not_upscaled.png

    win32_create_all_directories_needed_for_path(outpath);

    // ffmpeg output filenames need all % chars escaped with another % char. eg file%20exam%ple.jpg -> file%%20exam%%ple.jpg
    wchar_t replacethisbuffer[1024]; //todo
    wc *outpathtemp = outpath.to_wc_reusable();
    CopyStringWithCharsEscaped(replacethisbuffer, 1024, outpathtemp, L'%', L'%');
    free(outpathtemp);

    wchar_t buffer[1024*8]; // todo make sure enough for in and out paths
    // swprintf(buffer, L"ffmpeg -i \"%s\" -vf \"scale='min(200,iw)':-2\" \"%s\"", inpath.chars, outpath.chars);
    wc *inpathtemp = inpath.to_wc_reusable();
    swprintf(buffer, L"ffmpeg -i \"%s\" -vf \"scale='min(200,iw)':-2\" \"%s\"", inpathtemp, replacethisbuffer);
    free(inpathtemp);

    OutputDebugStringW(buffer);
    OutputDebugStringW(L"\n");

    win32_run_cmd_command(buffer);
}



// tries to read resolution and if it can't, then we assume we can't read the file
bool ffmpeg_can_open(string path) {
    v2 res = ffmpeg_GetResolution(path);
    return res.x != -1 && res.y != -1;
}

