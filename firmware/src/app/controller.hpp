#pragma once

#include "../hal/ports.hpp"

namespace midi {

class Controller {
 public:
  explicit Controller(ControllerPorts& ports) : ports_(ports) {}

  void initialize();
  bool configuration_available() const { return configurationAvailable_; }

 private:
  ControllerPorts& ports_;
  bool configurationAvailable_{};
};

}  // namespace midi
