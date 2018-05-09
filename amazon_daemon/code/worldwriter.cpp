#include "worldwriter.h"
Worldwriter::Worldwriter(SafeQueue<ACommands> &q, int world_fd) :
  world_write_queue(q),
  mythread(),
  is_active(true),
  world_fd(world_fd)
{
  std::cout << "Worldwriter constructor finishing" << std::endl;
  this->mythread = std::thread(&Worldwriter::wait_write, this);
  std::cout << "Worldwriter thread launched!" << std::endl;
}

void Worldwriter::wait_write(void) {
  while (this->is_active) {
    ACommands world_out_message; 
    google::protobuf::io::FileOutputStream *out_world = new google::protobuf::io::FileOutputStream(this->world_fd);
    std::cout << "World writer trying to pop something out of queue!" << std::endl;
    if (!world_write_queue.try_pop(world_out_message)) {
      this->is_active = false;
      delete out_world;
      continue;
    }
    std::cout << "Worldwriter sending message : " << world_out_message.DebugString();
    if (this->sendMesgTo(world_out_message,out_world) == false) {
      std::cout << "Upswriter couldn't send message to UPS!" << std::endl;
    }
    std::cout << "Worldwriter sent message!" << world_out_message.DebugString();
    delete out_world;
  }
}

Worldwriter::~Worldwriter(void) {
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
