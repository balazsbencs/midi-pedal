#include "controller.hpp"

namespace midi {

void Controller::initialize() {
  ports_.relay_set(1, false);
  ports_.relay_set(2, false);
  configurationAvailable_ = ports_.read_active_config();
  ports_.show_boot_status();
}

}  // namespace midi
