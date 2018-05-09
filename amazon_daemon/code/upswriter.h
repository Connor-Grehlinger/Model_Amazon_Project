#ifndef __UPSWRITER_H__
#define __UPSWRITER_H__
#include <exception>
#include <thread>
#include <iostream>
#include <pqxx/pqxx>
#include "safequeue.h"
#include "uacmt.pb.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

class Upswriter {
 private:
  SafeQueue<AUCommands> &ups_write_queue;
  std::thread mythread;
  bool is_active;
  int ups_fd;
 public:
  Upswriter(SafeQueue<AUCommands> &q, int ups_fd);
  ~Upswriter();
  void wait_write(void);

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
};

#endif
