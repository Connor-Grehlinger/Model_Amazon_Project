#ifndef __UPSWORKER_H__
#define __UPSWORKER_H__
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

class Upsworker {
 private:
  SafeQueue<UACommands> &ups_read_queue;
  SafeQueue<AUCommands> &ups_write_queue;
  SafeQueue<ACommands> &world_write_queue;
  std::thread mythread;
  bool is_active;
 public:
  Upsworker(SafeQueue<UACommands> &q1, SafeQueue<AUCommands> &q2, SafeQueue<ACommands> &q3);
  ~Upsworker();
  void parse_message(void);
  bool check_package_ready(std::string package_id, UACommands ups_in_message);
  void update_package_status(int packageid, std::string status);
  int get_warehouse(int orderid);
  void insert_ordershipment(std::string orderid, std::string pkid);
  void insert_packageready(std::string packageid);
  void update_trackingno(std::string orderid, std::string pkid);
  void check_stock_buy(ACommands &pack_message);
  void reduce_from_warehouse(ACommands &pack_message);
};
#endif

