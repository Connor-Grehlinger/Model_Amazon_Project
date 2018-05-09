/*this template class is a thread-safe queue*/
#ifndef __SAFE_QUEUE__
#define __SAFE_QUEUE__

#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
template<class T>

class SafeQueue {
 private:
  std::queue<T> q;
  mutable std::mutex m;
  std::condition_variable c;
  bool isStopped;
 public:
 SafeQueue(void) : q(), m(), c(), isStopped(false) {}
  void push(T value) {
    std::lock_guard<std::mutex> lock(m);
    q.push(value);
    c.notify_one();
  }
  bool try_pop(T& val) {
    std::unique_lock<std::mutex> lock(m);
    c.wait(lock, [this](){ return !q.empty() || isStopped; });
    if (isStopped) {
      return false;
    }
    /*while(q.empty()) {
      c.wait(lock);     }*/
    val = q.front();
    q.pop();
    std::cout << "popped something" << std::endl;
    return true;
  }
  void deactivate() {
    std::cout << "safequeue being deactivated!" << std::endl;
    this->isStopped = true;
    c.notify_all();
  }
  ~SafeQueue(void) {}  
};
#endif
