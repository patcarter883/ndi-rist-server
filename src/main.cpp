#include "rpc/server.h"
#include <iostream>
#include <thread>
#include <gst/gst.h>
#include <gst/rtp/rtp.h>
#include <gst/app/gstappsrc.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/video/video.h>
#include "ristnet/RISTNet.h"

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _App App;

struct _App
{
	GstElement *datasrc_pipeline;
	GstElement *videosrc, *videoEncoder;

	gboolean is_eos;
	gboolean isRunning;
	GMainLoop *loop;

	RISTNetReceiver *ristReceiver;

	gboolean isPlaying = false;
};

typedef struct _Config Config;

struct _Config
{
	guint width;
	guint height;
	guint framerate;

	std::string ndi_input_name;
	std::string codec = "h264";
	std::string encoder = "software";
	std::string transport = "m2ts";
	std::string bitrate = "4300";
	std::string rist_output_address = "127.0.0.1:5000?buffer-min=245&buffer-max=1000&rtt-min=40&rtt-max=500&reorder-buffer=60&congestion-control=1";
	std::string rist_output_buffer;
	std::string rist_output_bandwidth = "6000";
};

/* Globals */
Config config;
App app;

std::thread gstreamerThread;
std::thread ristThread;

void runRistThread() {

}

void runGStreamerThread() {

}

void start(string rtmpTarget) {
ristThread = std:thread(runRistThread);
gstreamerThread = std:thread(runGStreamerThread);
}

int main()
{
    // Create a server that listens on port 8080, or whatever the user selected
    rpc::server srv("0.0.0.0", 5999);

    srv.bind("start", &start);

    // Run the server loop.
    srv.run();
    return 0;
}
