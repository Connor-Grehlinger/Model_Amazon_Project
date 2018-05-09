#include "upswriter.h"
Upswriter::Upswriter(SafeQueue<AUCommands> &q, int ups_fd):
  ups_write_queue(q),
  mythread(),
  is_active(true),
  ups_fd(ups_fd)
{
  std::cout << "Upswriter constructor finishing" << std::endl;
  this->mythread = std::thread(&Upswriter::wait_write, this);
  std::cout << "Upswriter thread launched!" << std::endl;
}

void Upswriter::wait_write(void) {
  while (this->is_active) {
    AUCommands ups_out_message; 
    std::cout << "Ups writer trying to pop something out of queue!" << std::endl;
    google::protobuf::io::FileOutputStream *out_ups = new google::protobuf::io::FileOutputStream(this->ups_fd);
    if (!ups_write_queue.try_pop(ups_out_message)) {
      this->is_active = false;
      delete out_ups;
      continue;
    }
    std::cout << "Upswriter sending message : " << ups_out_message.DebugString();
    if (this->sendMesgTo(ups_out_message, out_ups) == false) {
      std::cout << "Upswriter couldn't send message to UPS!" << std::endl;
    }
    std::cout << "Upswriter sent message!" << ups_out_message.DebugString();
    delete out_ups;
  }
}

Upswriter::~Upswriter(void) {
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
