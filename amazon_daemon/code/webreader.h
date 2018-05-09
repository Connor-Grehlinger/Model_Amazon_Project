#ifndef __WEBREADER_H__
#define __WEBREADER_H__
#include <exception>
#include <thread>
#include <iostream>
#include <pqxx/pqxx>
#include "safequeue.h"
#include "uacmt.pb.h"

class Webreader {
 private:
  SafeQueue<AUCommands> &ups_write_queue;
  int web_socket;
  std::thread mythread;
  bool is_active;
  long int wh_id;
 public:
  Webreader(SafeQueue<AUCommands> &q);
  ~Webreader();
  void listen_connections(void);
  void form_purchase(int client_socket);
};

#endif
