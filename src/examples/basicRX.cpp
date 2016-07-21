/**
    @file   basicRX.cpp
    @author Lime Microsystems (www.limemicro.com)
    @brief  minimal RX example
 */

#include <cstdlib>
#include "lime/LimeSuite.h"
#include <iostream>
#include "math.h"
#include <thread>
#include <chrono>

#include "gnuPlotPipe.h"

using namespace std;

//Device structure, should be initialize to NULL
lms_device_t* device = NULL;

int error()
{
    //print last error message
    cout << "ERROR:" << LMS_GetLastErrorMessage();
    if (device != NULL)
        LMS_Close(device);
    exit(-1);
}

int main(int argc, char** argv)
{
    //Find devices
    int n;
    lms_info_str_t list[8]; //should be large enough to hold all detected devices
    if ((n = LMS_GetDeviceList(list)) < 0) //NULL can be passed to only get number of devices
        error();

    cout << "Devices found: " << n << endl; //print number of devices
    if (n < 1)
        return -1;

    //open the first device
    if (LMS_Open(&device, list[0], NULL))
        error();

    //Initialize device with default configuration
    //Do not use if you want to keep existing configuration
    if (LMS_Init(device) != 0)
        error();

    //Enable RX channel
    //Channels are numbered starting at 0
    if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0)
        error();

    //Set center frequency to 800 MHz
    //Automatically selects antenna port
    if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, 800e6) != 0)
        error();

    //Set sample rate to 8 MHz, ask to use 2x oversampling in RF
    //This set sampling rate for all channels
    if (LMS_SetSampleRate(device, 8e6, 2) != 0)
        error();

    //Enable test signal generation
    //To receive data from RF, remove this line or change signal to LMS_TESTSIG_NONE
    if (LMS_SetTestSignal(device, LMS_CH_RX, 0, LMS_TESTSIG_NCODIV8, 0, 0) != 0)
        error();

    //Streaming Setup

    //Initialize stream
    lms_stream_t streamId; //stream structure
    streamId.channel = 0; //channel number
    streamId.fifoSize = 1024 * 128; //fifo size in samples
    streamId.throughputVsLatency = 1.0; //optimize for max throughput
    streamId.isTx = false; //RX channel
    streamId.dataFmt = lms_stream_t::LMS_FMT_I16; //16-bit integers
    if (LMS_SetupStream(device, &streamId) != 0)
        error();

    //Initialize data buffers
    const int bufersize = 5000; //complex samples per buffer
    int16_t buffer[bufersize * 2]; //buffer to hold complex values (2*samples))

    //Start streaming
    LMS_StartStream(&streamId);

    //Streaming
    GNUPlotPipe gp;
    auto t1 = chrono::high_resolution_clock::now();
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(5)) //run for 5 seconds
    {
        int samplesRead;
        //Receive samples
        samplesRead = LMS_RecvStream(&streamId, buffer, bufersize, NULL, 1000);

        //Plot samples
        //I and Q samples are interleaved in buffer: IQIQIQ...
        gp.write("set title 'Channels Rx AB'\n");
        gp.write("set size square\n set xrange[-2050:2050]\n set yrange[-2050:2050]\n");
        gp.write("plot '-' with points");
        gp.write("\n");
        for (uint32_t j = 0; j < samplesRead; ++j)
            gp.writef("%i %i\n", buffer[2 * j], buffer[2 * j + 1]);
        gp.write("e\n");
        gp.flush();
    }

    //Stop streaming
    LMS_StopStream(&streamId); //stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(device, &streamId); //stream is deallocated and can no longer be used

    //Close device
    LMS_Close(device);

    return 0;
}

