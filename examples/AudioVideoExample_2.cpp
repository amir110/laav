/* 
 * Created (25/04/2017) by Paolo-Pr.
 * This file is part of Live Asynchronous Audio Video Library.
 *
 * Live Asynchronous Audio Video Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Live Asynchronous Audio Video Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Live Asynchronous Audio Video Library.  If not, see <http://www.gnu.org/licenses/>.
 * --------------------------------------------
 *
 * This example shows how to grab audio and video from a V4L camera and an ALSA device,
 * encode (MP2 + H264) and stream it through HTTP with a MPEGTS container.
 * The stream's address is printed in the output log.
 *
 * It also shows how to manage errors during the media tasks.
 * Laav deals four error/exception types:
 *
 * 1) severe unrecoverable errors (i.e: FFMPEG is not properly compiled and it doesn't
 * provide the requested audio/video codecs, out of memory errors etc.)
 *   --> the error can't be caught and it causes the program to terminate, by printing the
 *   code line where the it occurred
 *
 * 2) non-severe unrecoverable errors (i.e: a grabber doesn't accept a specific
 * size/samplerate/format, or a Streamer/CommandSReceiver is created on an already open port)
 *   --> the error doesn't cause the program to terminate and it can be polled; the node which
 *   caused the error is put into an idle mode until the end of the process.
 *
 * 3) recoverable errors (i.e: a device is disconnected)
 *   --> the error doesn't cause the program to terminate and it can be polled; the node
 *   which caused the error is put into an idle state until it is ready to perform its task
 *   again (disconnection -> reconnection)
 *
 * 4) exceptions: see VideoExample_2.cpp
 *
 * All the errors of a device grabber and HTTP nodes are pollable (they can be checked with
 * grabber.getErrno() and grabber.status() functions)
 *
 */

#include "LAAV.hpp"

using namespace laav;

typedef UnsignedConstant<16000> SAMPLERATE;
typedef UnsignedConstant<640> WIDTH;
typedef UnsignedConstant<480> HEIGHT;

int main(int argc, char** argv)
{

    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0]
        << " alsa-device-identifier[i.e: plughw:U0x46d0x819] /path/to/v4l/device" << std::endl;
        return 1;
    }

    LAAVLogLevel = 1;
    std::string addr = "127.0.0.1";

    SharedEventsCatcher eventsCatcher = EventsManager::createSharedEventsCatcher();

    AlsaGrabber <S16_LE, SAMPLERATE, MONO>
    aGrab(eventsCatcher, argv[1], DEFAULT_SAMPLERATE);

    FFMPEGMP2Encoder <S16_LE, SAMPLERATE, MONO>
    aEnc;

    AudioFrameHolder <MP2, SAMPLERATE, MONO>
    aFh;

    V4L2Grabber <YUYV422_PACKED, WIDTH, HEIGHT>
    vGrab(eventsCatcher, argv[2], DEFAULT_FRAMERATE);

    FFMPEGVideoConverter <YUYV422_PACKED, WIDTH, HEIGHT, YUV420_PLANAR, WIDTH, HEIGHT>
    vConv;

    FFMPEGH264Encoder <YUV420_PLANAR, WIDTH, HEIGHT>
    vEnc(DEFAULT_BITRATE, 5, H264_ULTRAFAST, H264_DEFAULT_PROFILE, H264_DEFAULT_TUNE);

    VideoFrameHolder <H264, WIDTH, HEIGHT>
    vFh;

    HTTPAudioVideoStreamer <MPEGTS, H264, WIDTH, HEIGHT, MP2, SAMPLERATE, MONO>
    avStream_1(eventsCatcher, addr, 8080);

    // We intentionally use an already open port (error test)
    HTTPAudioVideoStreamer <MPEGTS, H264, WIDTH, HEIGHT, MP2, SAMPLERATE, MONO>
    avStream_2(eventsCatcher, addr, 8080);

    bool showVideoGrabError = true;
    bool showAudioGrabError = true;
    bool showAudioVideoStreamError = true;

    while (!LAAVStop)
    {
        /*
         * AUDIO/VIDEO pipes
         */
        aGrab >> aEnc >> aFh;
                         aFh >> avStream_1;
                         aFh >> avStream_2;

        vGrab >> vConv >> vEnc >> vFh;
                                  vFh >> avStream_1;
                                  vFh >> avStream_2;

        /*
         * ERROR HANDLING
         */
        if (avStream_2.inputStatus() != READY)
        {
            if (showAudioVideoStreamError)
            {
                std::cerr << "STREAM error: "
                << strerror(avStream_2.getErrno()) << std::endl;
                std::cerr << "The error can't be recovered"  << std::endl;
                showAudioVideoStreamError = false;
            }
        }

        if (vGrab.deviceStatus() != DEV_CAN_GRAB)
        {
            if (showVideoGrabError)
            {
                std::cerr << "VIDEO error: " << vGrab.getV4LErrorString()
                << " (" << strerror(vGrab.getErrno()) << ")" << std::endl;

                if (vGrab.isInUnrecoverableState())
                    std::cerr << "The error can't be recovered" << std::endl;

                showVideoGrabError = false;
            }
        }
        else
            showVideoGrabError = true;

        if (aGrab.status() != DEV_CAN_GRAB)
        {
            if (showAudioGrabError)
            {
                std::cerr << "AUDIO error: " << aGrab.getAlsaErrorString()
                << " (" << strerror(aGrab.getErrno()) << ")" << std::endl;

                if(aGrab.isInUnrecoverableState())
                    std::cerr << "The error can't be recovered" << std::endl;

                showAudioGrabError = false;
            }
        }
        else
            showAudioGrabError = true;

        eventsCatcher->catchNextEvent();
    }

    return 0;

}
