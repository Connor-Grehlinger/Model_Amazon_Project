#ifndef __WORLDWORKER_H__
#define __WORLDWORKER_H__
#include <exception>
#include <thread>
#include <iostream>
#include <pqxx/pqxx>
#include "safequeue.h"
#include "uacmt.pb.h"
#include "amazon.pb.h"
#include "ups.pb.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

class Worldworker {
 private:
  SafeQueue<AUCommands> &ups_write_queue;
  SafeQueue<AResponses> &world_read_queue;
  SafeQueue<ACommands> &world_write_queue;
  std::thread mythread;
  bool is_active;
 public:
  Worldworker(SafeQueue<AUCommands> &q1, SafeQueue<AResponses> &q2, SafeQueue<ACommands> &q3);
  ~Worldworker();
  void parse_message(void);
  void update_ready(long int packageid);
  void update_stock_db(AResponses &world_in_message);
};
#endif

