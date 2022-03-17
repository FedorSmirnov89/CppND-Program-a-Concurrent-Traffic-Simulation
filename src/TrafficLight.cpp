#include "TrafficLight.h"
#include <chrono>
#include <iostream>
#include <random>
#include <stdlib.h>
#include <thread>

/* Implementation of class "MessageQueue" */

template <typename T> T MessageQueue<T>::receive() {
  std::unique_lock<std::mutex> uLock(_mutex);
  _condVar.wait(uLock, [this]() {
    bool nonEmpty = !_queue.empty();
    if (nonEmpty) {
      return _queue.front() == green;
    } else {
      return false;
    }
  });

  T msg = std::move(_queue.back());
  //_queue.pop_back();
  return msg;

  // FP.5a : The method receive should use std::unique_lock<std::mutex> and
  // _condition.wait() to wait for and receive new messages and pull them from
  // the queue using move semantics. The received object should then be returned
  // by the receive function.
}

template <typename T> void MessageQueue<T>::send(T &&msg) {
  // FP.4a : The method send should use the mechanisms
  // std::lock_guard<std::mutex> as well as _condition.notify_one() to add a new
  // message to the queue and afterwards send a notification.

  std::lock_guard<std::mutex> lck(_mutex);

  // we only want to have one message in the queue, since it represents the
  // current state of the traffic light (in my personal opinion, the usage of a
  // message queue is a bit odd here, since the cars do not "consume" the
  // traffic light events -- multiple cars could go through the intersection
  // using the same green phase. Just having the state of the traffic light
  // accessed in a thread-safe way would be sufficient)

  if (!_queue.empty()) {
    _queue.pop_front();
  }

  _queue.push_front(std::move(msg));
  _condVar.notify_all();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight() { _currentPhase = TrafficLightPhase::red; }

void TrafficLight::waitForGreen() {
    _msgQueue.receive();

  // FP.5b : add the implementation of the method waitForGreen, in which an
  // infinite while-loop runs and repeatedly calls the receive function on the
  // message queue. Once it receives TrafficLightPhase::green, the method
  // returns.
}

TrafficLightPhase TrafficLight::getCurrentPhase() {
  std::lock_guard<std::mutex> lck(_mutex);
  return _currentPhase;
}

void TrafficLight::simulate() {
  threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
  // FP.2b : Finally, the private method „cycleThroughPhases“ should be started
  // in a thread when the public method „simulate“ is called. To do this, use
  // the thread queue in the base class.
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases() {
  int cycleDuration = 4000 + rand() % 2000;
  auto lastCycleStart = std::chrono::steady_clock::now();

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto timeNow = std::chrono::steady_clock::now();
    auto curCycle = std::chrono::duration_cast<std::chrono::milliseconds>(
                        timeNow - lastCycleStart)
                        .count();

    if (curCycle > cycleDuration) {
      // cycle time over -> switch the phase

      {
        std::lock_guard<std::mutex> lck(
            _mutex); // protecting the access to the light state

        if (_currentPhase == red) {
          _currentPhase = green;
        } else {
          _currentPhase = red;
        }
      }

      TrafficLightPhase msg = _currentPhase;
      _msgQueue.send(std::move(msg));

      // remember new cycle start
      lastCycleStart = timeNow;
    }
  }

  // FP.2a : Implement the function with an infinite loop that measures the time
  // between two loop cycles and toggles the current phase of the traffic light
  // between red and green and sends an update method to the message queue using
  // move semantics. The cycle duration should be a random value between 4 and 6
  // seconds. Also, the while-loop should use std::this_thread::sleep_for to
  // wait 1ms between two cycles.
}
