#pragma once
#include <cstdint>
#include <string>

namespace powertech {

enum class Mode { UNKNOWN, FULL, PEDESTRIAN };
enum class State { UNKNOWN, CLOSED, OPENING, OPENED, PED_OPENING, PED_OPENED, CLOSING, STOPPED };

// Host-testable protocol/state core; contains no installation-specific values.
class GateState {
 public:
  static constexpr uint32_t MOTION_TIMEOUT_MS = 120000;
  static constexpr uint32_t COMMAND_GAP_MS = 750;
  State state = State::UNKNOWN;
  Mode mode = Mode::UNKNOWN;
  uint32_t last_event_ms = 0;
  bool observed = false;

  bool moving(uint32_t now) const {
    return observed && uint32_t(now - last_event_ms) < MOTION_TIMEOUT_MS &&
           (state == State::OPENING || state == State::PED_OPENING || state == State::CLOSING);
  }
  bool expire(uint32_t now) {
    if ((state == State::OPENING || state == State::PED_OPENING || state == State::CLOSING) &&
        uint32_t(now - last_event_ms) >= MOTION_TIMEOUT_MS) {
      state = State::UNKNOWN;
      mode = Mode::UNKNOWN;
      stop_pending_ = false;
      return true;
    }
    return false;
  }
  // Binary cover convention: only Closed proves closed. 0.5 is an unmeasured
  // intermediate/unknown value, never a measured percentage or full-open claim.
  float presentation_position() const {
    if (state == State::CLOSED) return 0.0f;
    if (state == State::OPENED) return 1.0f;
    return 0.5f;
  }
  const char *status() const {
    switch (state) {
      case State::CLOSED: return "Closed";
      case State::OPENING: return "Opening";
      case State::OPENED: return "Opened";
      case State::PED_OPENING: return "PedOpening";
      case State::PED_OPENED: return "PedOpened";
      case State::CLOSING: return "Closing";
      case State::STOPPED: return "Stopped";
      default: return "Unknown";
    }
  }
  const char *mode_name() const {
    if (mode == Mode::FULL) return "Full";
    if (mode == Mode::PEDESTRIAN) return "Pedestrian";
    return "Unknown";
  }
  // Reassemble debugger fragments, accept multiple lines/chunk, bound memory,
  // and discard corrupt/oversized lines through the next newline.
  bool feed(uint8_t byte, uint32_t now) {
    if (byte == '\n') {
      bool changed = false;
      if (!discard_) {
        if (!line_.empty() && line_.back() == '\r') line_.pop_back();
        changed = parse_line(line_, now);
      }
      line_.clear();
      discard_ = false;
      return changed;
    }
    if (discard_) return false;
    if (line_.size() >= 256 || (byte != '\r' && (byte < 32 || byte > 126))) {
      line_.clear();
      discard_ = true;
      return false;
    }
    line_.push_back(static_cast<char>(byte));
    return false;
  }
  bool parse_line(const std::string &line, uint32_t now) {
    const std::string prefix = "$V1PKF0,";
    if (line.compare(0, prefix.size(), prefix) != 0) return false;
    const auto comma = line.find(',', prefix.size());
    if (comma == std::string::npos || comma == prefix.size()) return false;
    for (size_t i = prefix.size(); i < comma; ++i)
      if (line[i] < '0' || line[i] > '9') return false;
    const auto suffix = line.find(";src=", comma + 1);
    if (suffix == std::string::npos || !valid_source(line.substr(suffix + 5))) return false;
    const auto value = line.substr(comma + 1, suffix - comma - 1);
    State next;
    if (value == "Closed") next = State::CLOSED;
    else if (value == "Opening") next = State::OPENING;
    else if (value == "Opened") next = State::OPENED;
    else if (value == "PedOpening") next = State::PED_OPENING;
    else if (value == "PedOpened") next = State::PED_OPENED;
    else if (value == "Closing") next = State::CLOSING;
    else if (value == "Stopped") next = State::STOPPED;
    else return false;
    if (next != state) stop_pending_ = false;
    state = next;
    if (next == State::OPENING || next == State::OPENED) mode = Mode::FULL;
    else if (next == State::PED_OPENING || next == State::PED_OPENED) mode = Mode::PEDESTRIAN;
    else if (next == State::CLOSED) mode = Mode::UNKNOWN;
    observed = true;
    last_event_ms = now;
    return true;
  }
  static bool valid_source(const std::string &source) {
    if (source.empty() || source.size() > 32) return false;
    for (char c : source)
      if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')))
        return false;
    return true;
  }
  // Returning a frame means queued for UART, not acknowledged or executed.
  // Sending never changes observed state/mode. No polling or automatic retries.
  std::string request(const std::string &command, const std::string &source,
                      bool enabled, uint32_t now) {
    if (!enabled || !valid_source(source)) return {};
    if (command != "FULL OPEN" && command != "PED OPEN" && command != "FULL CLOSE" && command != "STOP")
      return {};
    if (command == "STOP") {
      if (!moving(now) || stop_pending_) return {};
    } else if (sent_ && uint32_t(now - last_command_ms_) < COMMAND_GAP_MS) return {};
    // STOP bypasses the ordinary rate limit to remain usable immediately after
    // a movement event; its own latch prevents duplicate transmissions.
    if (command == "STOP") stop_pending_ = true;
    last_command_ms_ = now;
    sent_ = true;
    return command + ";src=" + source + "\r\n";
  }

 private:
  std::string line_;
  bool discard_ = false;
  bool stop_pending_ = false;
  bool sent_ = false;
  uint32_t last_command_ms_ = 0;
};
}  // namespace powertech
