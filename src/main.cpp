#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include <thread>

#include <gst/allocators/gstdmabuf.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/rtp/rtp.h>
#include <gst/video/video.h>

#include "RISTNet.h"
#include "rpc/server.h"

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _App App;

struct _App
{
  GstElement* datasrc_pipeline;
  GstElement* videosrc;

  gboolean is_eos;
  gboolean isRunning;
  GMainLoop* loop;

  gboolean isPlaying = false;
};

typedef struct _Config Config;

struct _Config
{
  std::string rist_input_address =
      "rist://@127.0.0.1:5000?buffer-min=245&buffer-max=1000&rtt-min=40&rtt-max=500&"
      "reorder-buffer=60&congestion-control=1";
  std::string rtmp_output_address =
      "rtmp://sydney.restream.io/live/re_6467989_5e25e884e7fc0b843888";
};

/* Globals */
Config config;
App app;

std::thread gstreamerThread;
std::thread ristThread;

//Return a connection object. (Return nullptr if you don't want to connect to that client)
std::shared_ptr<RISTNetReceiver::NetworkConnection> validateConnection(const std::string &ipAddress, uint16_t port) {
    std::cout << "Connecting IP: " << ipAddress << ":" << unsigned(port) << std::endl;

    //Do we want to allow this connection?
    //Do we have enough resources to accept this connection...

    // if not then -> return nullptr;
    // else return a ptr to a NetworkConnection.
    // this NetworkConnection may contain a pointer to any C++ object you provide.
    // That object ptr will be passed to you when the client communicates with you.
    // If the network connection is dropped the destructor in your class is called as long
    // as you do not also hold a reference to that pointer since it's shared.

    auto netConn = std::make_shared<RISTNetReceiver::NetworkConnection>(); // Create the network connection
    return netConn;
}

int
dataFromSender(const uint8_t *buf, size_t len, std::shared_ptr<RISTNetReceiver::NetworkConnection> &connection,
               rist_peer *pPeer, uint16_t connectionID) {
    
    GstBuffer *buffer;
    GstFlowReturn ret;

    buffer = gst_buffer_new_memdup (buf, len);

    g_signal_emit_by_name (app.videosrc, "push-buffer", buffer, &ret);
    gst_buffer_unref (buffer);

  if (ret != GST_FLOW_OK) {
      /* some error, stop sending data */
      GST_DEBUG ("some error");
      g_signal_emit_by_name (app.videosrc, "end-of-stream", &ret);
      return 1;
  }

    return 0; //Keep connection
}

void runRistThread()
{
  RISTNetReceiver ristReceiver;

  ristReceiver.validateConnectionCallback = std::bind(
      &validateConnection, std::placeholders::_1, std::placeholders::_2);
  // receive data from the client
  ristReceiver.networkDataCallback = std::bind(&dataFromSender,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3,
                                                    std::placeholders::_4,
													std::placeholders::_5);
  std::vector<std::string> interfaceListReceiver;
  interfaceListReceiver.push_back(config.rist_input_address);
  RISTNetReceiver::RISTNetReceiverSettings myReceiveConfiguration;

  // Initialize the receiver
  if (!ristReceiver.initReceiver(interfaceListReceiver,
                                      myReceiveConfiguration))
  {
    return;
  }
  while (app.isPlaying)
  {
    std::this_thread::yield();
  }
  return;
}

void stop() {
  std::cout << "Stopping." << std::endl;
	app.isPlaying = false;
  if (g_main_loop_is_running(app.loop))
	{
		g_main_loop_quit(app.loop);
	}
	ristThread.join();
	gstreamerThread.join();
}

static gboolean
datasrc_message(GstBus *bus, GstMessage *message, App *app)
{
	GError *err;
	gchar *debug_info;

	switch (GST_MESSAGE_TYPE(message))
	{
	case GST_MESSAGE_ERROR:
		gst_message_parse_error(message, &err, &debug_info);
		cerr << "Received error from datasrc_pipeline..." << endl;
		cerr << "Error received from element " << GST_OBJECT_NAME(message->src) << ": " << err->message << endl;
		cerr << "Debugging information:  " << (debug_info ? debug_info : "none") << endl;
		g_clear_error(&err);
		g_free(debug_info);
		if (g_main_loop_is_running(app->loop))
			g_main_loop_quit(app->loop);
		break;
	case GST_MESSAGE_EOS:
		cerr << "Received EOS from pipeline..." << endl;
		stop();
		break;
	default:
		// logAppend(fmt::format("\n{} received from element {}\n",
		// GST_MESSAGE_TYPE_NAME(message), GST_OBJECT_NAME(message->src)));
		break;
	}
	// gst_debug_bin_to_dot_file(GST_BIN(app->datasrc_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-debug");
	return TRUE;
}

void runGStreamerThread() {
  GError *error = NULL;
  GstBus *datasrc_bus;

	app.loop = g_main_loop_new(NULL, FALSE);
	std::string pipeline_str = "flvmux streamable=true name=mux ! queue ! rtmpsink location='" + config.rtmp_output_address + "'name=rtmpSink multiqueue name=outq appsrc name=videosrc ! queue2 ! tsparse set-timestamps=true alignment=7 ! tsdemux name=demux demux. ! av1parse ! queue ! nvav1dec ! queue ! videoscale ! video/x-raw,width=2560,height=1440 ! queue ! nvautogpuh264enc rate-control=cbr-hq bitrate=16000 gop-size=120 repeat-sequence-header=true preset=hq ! video/x-h264,framerate=60/1,profile=high ! h264parse ! outq.sink_0 outq.src_0 ! mux.  demux. ! aacparse ! queue max-size-time=5000000000 ! outq.sink_1 outq.src_1 ! mux.";
	// std::string pipeline_str = "flvmux streamable=true name=mux ! queue ! rtmpsink name=rtmpSink location='rtmp://sydney.restream.io/live/re_6467989_5e25e884e7fc0b843888 live=true' multiqueue name=outq appsrc name=videosrc ! queue2 ! tsparse set-timestamps=true alignment=7 ! tsdemux name=demux demux. ! av1parse ! queue ! d3d11av1dec ! queue ! videoscale ! video/x-raw,width=2560,height=1440 ! queue ! amfh264enc rate-control=cbr bitrate=16000 gop-size=120 ! video/x-h264,framerate=60/1,profile=high ! h264parse ! outq.sink_0 outq.src_0 ! mux.  demux. ! aacparse ! queue max-size-time=5000000000 ! outq.sink_1 outq.src_1 ! mux.";
  cout << "Running Pipeline: " << pipeline_str << endl;
	app.datasrc_pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
  if (error) {
    cerr << "Error: " << error->message << endl;
    g_clear_error (&error);
  }
  if (app.datasrc_pipeline == NULL)
	{
		cerr << "*** Bad pipeline. ***" << endl;
	}
	// app.videosrc = gst_bin_get_by_name(GST_BIN(app.datasrc_pipeline), "videosrc");
	// GstElement* rtmpsink = gst_bin_get_by_name(GST_BIN(app.datasrc_pipeline), "rtmpSink");
	// g_object_set(G_OBJECT(rtmpsink), "location", config.rtmp_output_address.c_str(), NULL);

  datasrc_bus = gst_element_get_bus(app.datasrc_pipeline);

	/* add watch for messages */
	gst_bus_add_watch(datasrc_bus, (GstBusFunc)datasrc_message, &app);
	gst_object_unref(datasrc_bus);

	gst_element_set_state(app.datasrc_pipeline, GST_STATE_PLAYING);

	g_main_loop_run(app.loop);
	gst_element_set_state(app.datasrc_pipeline, GST_STATE_NULL);

	gst_object_unref(GST_OBJECT(app.datasrc_pipeline));
	g_main_loop_unref(app.loop);
	app.isPlaying = false;

  return;
}

void start(std::string rtmpTarget)
{
  std::cout << "Start Requested for destination " <<  rtmpTarget << std::endl;
	app.isPlaying = true;
	config.rtmp_output_address = rtmpTarget;
	ristThread = std::thread(runRistThread);
	gstreamerThread = std::thread(runGStreamerThread);
}

int main()
{
  gst_init(NULL, NULL);
  
  // Create a server that listens on port 8080, or whatever the user selected
  rpc::server srv("0.0.0.0", 5999);

  std::cout << "RIST Restreamer Started - Listening on port " << srv.port() << std::endl;

  srv.bind("start", &start);
  srv.bind("stop", &stop);

  // Run the server loop.
  srv.run();
  return 0;
}
