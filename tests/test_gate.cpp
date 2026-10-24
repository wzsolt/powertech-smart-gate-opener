#include "../esphome/gate_protocol.h"
#include <cassert>
#include <iostream>
#include <string>
using powertech::GateState;
using powertech::Mode;
using powertech::State;

// All source strings here are invented fixtures, unrelated to any installation.
static std::string event(const std::string &value, const std::string &src = "TESTSOURCE") {
  return "$V1PKF0,42," + value + ";src=" + src;
}
static int feed(GateState &gate, const std::string &bytes, uint32_t now) {
  int count = 0;
  for (uint8_t byte : bytes) count += gate.feed(byte, now);
  return count;
}
int main() {
  GateState gate;
  assert(gate.state == State::UNKNOWN && !gate.moving(0));
  assert(gate.presentation_position() == 0.5f);
  assert(gate.request("STOP", "TESTWB3", true, 1).empty());
  assert(gate.request("FULL OPEN", "TESTWB3", false, 1).empty());
  assert(gate.request("FULL OPEN", "REPLACE_WITH_YOUR_WB3_SRC", true, 1).empty());
  assert(gate.request("FULL OPEN", "TESTWB3\r\nSTOP", true, 1).empty());
  assert(gate.request("STATUS", "TESTWB3", true, 1).empty());
  assert(gate.request("RS", "TESTWB3", true, 1).empty());
  assert(gate.request("FULL OPEN", "TESTWB3", true, 10) == "FULL OPEN;src=TESTWB3\r\n");
  assert(gate.mode == Mode::UNKNOWN && gate.state == State::UNKNOWN); // not optimistic
  assert(gate.request("PED OPEN", "TESTWB3", true, 100).empty()); // shared rate limit
  assert(feed(gate, "$V1PKF0,42,PedOpen", 1000) == 0);
  assert(feed(gate, "ing;src=TESTRF\r", 1010) == 0);
  assert(gate.request("PED OPEN", "TESTWB3", true, 1015) == "PED OPEN;src=TESTWB3\r\n");
  assert(gate.state == State::UNKNOWN && gate.mode == Mode::UNKNOWN);
  assert(feed(gate, "\n", 1020) == 1);
  assert(gate.state == State::PED_OPENING && gate.mode == Mode::PEDESTRIAN);
  assert(gate.moving(1021));
  assert(gate.request("STOP", "TESTWB3", true, 1021) == "STOP;src=TESTWB3\r\n"); // bypasses recent command
  assert(gate.request("STOP", "TESTWB3", true, 1022).empty());
  assert(gate.parse_line(event("PedOpening"), 1023));
  assert(gate.request("STOP", "TESTWB3", true, 3000).empty()); // duplicate event does not reset latch
  assert(gate.parse_line(event("Stopped"), 3001));
  assert(!gate.moving(3002) && gate.presentation_position() == 0.5f);
  assert(gate.mode == Mode::PEDESTRIAN);
  assert(gate.request("STOP", "TESTWB3", true, 4000).empty());
  assert(gate.parse_line(event("PedOpened"), 4001));
  assert(gate.presentation_position() != 1.0f && !gate.moving(4002));
  assert(gate.parse_line(event("Closing"), 5000));
  assert(gate.mode == Mode::PEDESTRIAN);
  assert(gate.request("STOP", "TESTWB3", true, 5001).size() > 0);
  assert(feed(gate, event("Closed") + "\r\n" + event("Opening", "OTHERRF") + "\r\n", 6000) == 2);
  assert(gate.mode == Mode::FULL && gate.moving(6001));
  assert(gate.parse_line(event("Opened"), 7000));
  assert(gate.presentation_position() == 1.0f);
  assert(gate.parse_line(event("Closed"), 8000));
  assert(gate.presentation_position() == 0.0f && gate.mode == Mode::UNKNOWN);
  assert(!gate.parse_line("ACK RS:UNKNOWN", 9000));
  assert(!gate.parse_line("ACK STATUS:FULL CLOSED,0", 9000));
  assert(!gate.parse_line(event("UnexpectedFault"), 9000));
  assert(!gate.parse_line("$V1PKF0,bad,Opening;src=TEST", 9000));
  assert(!gate.parse_line("$V1PKF0,42,Opening;src=", 9000));
  assert(gate.last_event_ms == 8000);
  assert(feed(gate, std::string(300, 'X') + event("Opening") + "\n", 9001) == 0);
  assert(feed(gate, std::string("bad\x01") + event("Opening") + "\n", 9002) == 0);
  assert(feed(gate, event("Closing") + "\r\n", 10000) == 1);
  assert(gate.mode == Mode::UNKNOWN); // no full-mode fallback
  assert(!gate.expire(129999));
  assert(!gate.moving(130000));
  assert(gate.request("STOP", "TESTWB3", true, 130000).empty());
  assert(gate.expire(130000));
  assert(gate.state == State::UNKNOWN && gate.mode == Mode::UNKNOWN);
  GateState reboot;
  assert(reboot.parse_line(event("Stopped"), 2));
  assert(reboot.mode == Mode::UNKNOWN && reboot.presentation_position() == 0.5f);
  assert(reboot.parse_line(event("Closing"), UINT32_MAX - 100));
  assert(reboot.moving(10)); // millis wraparound
  assert(reboot.expire(120000));
  assert(!GateState::valid_source(std::string(33, 'X')));
  assert(!GateState::valid_source(""));
  assert(GateState::valid_source(std::string(32, 'X')));
  assert(reboot.request("FULL CLOSE", "TESTWB3", true, 120010) == "FULL CLOSE;src=TESTWB3\r\n");
  assert(reboot.state == State::UNKNOWN && reboot.mode == Mode::UNKNOWN);
  std::cout << "All gate protocol/state regression checks passed\n";
}
