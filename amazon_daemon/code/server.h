#ifndef __SERVER__H_
#define __SERVER__H_
#include <string>
#include "webreader.h"
#include "upswriter.h"
#include "upsreader.h"
#include "upsworker.h"
#include "worldreader.h"
#include "worldwriter.h"
#include "worldworker.h"
#include "whouse.h"
#include "safequeue.h"
#include "amazon.pb.h"
#include "ups.pb.h"
#include "uacmt.pb.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <vector>
class Server {
  int ups_socket;
  int world_socket;
  int web_socket;
  int worldid;
  int num_wh;
  std::map<int64_t, int> package_truck_map;
  std::map<int, int64_t> order_package_map;
  std::unique_ptr<Webreader> webreader;
  std::unique_ptr<Upswriter> upswriter;
  std::unique_ptr<Upsreader> upsreader;
  std::unique_ptr<Worldreader> worldreader;
  std::unique_ptr<Worldwriter> worldwriter;
  std::vector<std::unique_ptr<Upsworker> > upsworkers;
  std::vector<std::unique_ptr<Worldworker> > worldworkers;
  std::vector<std::unique_ptr<Whouse> > warehouses;
  SafeQueue<AUCommands> ups_write_queue;
  SafeQueue<UACommands> ups_read_queue;
  SafeQueue<ACommands> world_write_queue;
  SafeQueue<AResponses> world_read_queue;
 public:
  Server();
  ~Server();
  void connect_ups(std::string ups_ip, int ups_port, std::string worldip, int worldport);
  void init_warehouses(void);
  void init_ups(std::string ip, int port, std::string worldip, int worldport);
  void send_wh_to_ups();
  void initial_wh_purchase();
  void init_world(std::string ip, int port);
  void run_server(std::string ups_ip, int ups_port, std::string worldip, int worldport);
  void deploy_threads();

  template<typename T>
    bool sendMesgTo(const T & message,
		    google::protobuf::io::FileOutputStream *out) {
    { //extra scope: make output go away before out->Flush()
      // We create a new coded stream for each message.  Don't worry, this is fast.
      google::protobuf::io::CodedOutputStream output(out);
      // Write the size.
      const int size = message.ByteSize();
      output.WriteVarint32(size);
      uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
      if (buffer != NULL) {
	// Optimization:  The message fits in one buffer, so use the faster
	// direct-to-array serialization path.
	message.SerializeWithCachedSizesToArray(buffer);
      } else {
	// Slightly-slower path when the message is multiple buffers.
	message.SerializeWithCachedSizes(&output);
	if (output.HadError()) {
	  return false;
      }
      }
    }
    out->Flush();
    return true;
  }

  template<typename T>
    bool recvMesgFrom(T & message,
		      google::protobuf::io::FileInputStream * in ){
    google::protobuf::io::CodedInputStream input(in);
    uint32_t size;
    if (!input.ReadVarint32(&size)) {
      return false;
    }
    // Tell the stream not to read beyond that size.
    google::protobuf::io::CodedInputStream::Limit limit = input.PushLimit(size);
    // Parse the message.
    if (!message.MergeFromCodedStream(&input)) {
      return false;
    }
    if (!input.ConsumedEntireMessage()) {
      return false;
    }
    // Release the limit.
    input.PopLimit(limit);
    return true;
  }
};

#endif
