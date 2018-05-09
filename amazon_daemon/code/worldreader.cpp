#include "worldreader.h"
Worldreader::Worldreader(SafeQueue<AResponses> &q, int world_fd) :
  world_read_queue(q),
  mythread(),
  is_active(true),
  world_fd(world_fd)
{
  std::cout << "Worldreader constructor finishing" << std::endl;
  this->mythread = std::thread(&Worldreader::wait_read, this);
  std::cout << "Worldreader thread launched!" << std::endl;
}

void Worldreader::wait_read() {
  while(this->is_active) {
    AResponses world_in_message;
    google::protobuf::io::FileInputStream *in_world = new google::protobuf::io::FileInputStream(this->world_fd);
    this->recvMesgFrom(world_in_message, in_world);
    // std::cout << "Worldreader received : " << world_in_message.DebugString();
    this->world_read_queue.push(world_in_message);
    std::cout << "Worldreader pushed the message into the queue!" << std::endl;
    delete in_world;
  }
}

Worldreader::~Worldreader() {
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
