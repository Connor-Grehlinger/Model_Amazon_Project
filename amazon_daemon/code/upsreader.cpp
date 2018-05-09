#include "upsreader.h"
Upsreader::Upsreader(SafeQueue<UACommands> &q, int ups_fd) :
  ups_read_queue(q),
  mythread(),
  is_active(true),
  ups_fd(ups_fd)
{
  std::cout << "Upsreader constructor finishing" << std::endl;
  this->mythread = std::thread(&Upsreader::wait_read, this);
  std::cout << "Upsreader thread launched!" << std::endl;
}

void Upsreader::wait_read() {
  while(this->is_active) {
    UACommands ups_in_message;
    google::protobuf::io::FileInputStream *in_ups = new  google::protobuf::io::FileInputStream(this->ups_fd);
    this->recvMesgFrom(ups_in_message, in_ups);
    std::cout << "Upsreader received : " << ups_in_message.DebugString();
    this->ups_read_queue.push(ups_in_message);
    std::cout << "Upsreader pushed the message into the queue!" << std::endl;
    delete in_ups;
  }
}

Upsreader::~Upsreader() {
  this->is_active = false;
  try {
    this->mythread.join();
  } catch(const std::system_error &se) {
    std::cout << se.what();
    //log se.what()
  } catch (const std::exception &e) {
    //log e.what
    std::cout << e.what();
  }
}
