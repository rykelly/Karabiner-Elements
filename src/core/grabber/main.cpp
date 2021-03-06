#include "connection_manager.hpp"
#include "constants.hpp"
#include "event_manipulator.hpp"
#include "karabiner_version.h"
#include "notification_center.hpp"
#include "thread_utility.hpp"
#include "version_monitor.hpp"
#include "virtual_hid_device_client.hpp"
#include <iostream>
#include <unistd.h>

int main(int argc, const char* argv[]) {
  if (getuid() != 0) {
    std::cerr << "fatal: karabiner_grabber requires root privilege." << std::endl;
    exit(1);
  }

  // ----------------------------------------
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);
  thread_utility::register_main_thread();

  logger::get_logger().info("version {0}", karabiner_version);

  std::unique_ptr<version_monitor> version_monitor_ptr = std::make_unique<version_monitor>(logger::get_logger());

  // load kexts
  system("/sbin/kextload /Library/Extensions/org.pqrs.driver.Karabiner.VirtualHIDDevice.kext");

  // make socket directory.
  mkdir(constants::get_tmp_directory(), 0755);
  chown(constants::get_tmp_directory(), 0, 0);
  chmod(constants::get_tmp_directory(), 0755);

  unlink(constants::get_grabber_socket_file_path());

  std::unique_ptr<virtual_hid_device_client> virtual_hid_device_client_ptr = std::make_unique<virtual_hid_device_client>(logger::get_logger());
  std::unique_ptr<manipulator::event_manipulator> event_manipulator_ptr = std::make_unique<manipulator::event_manipulator>(*virtual_hid_device_client_ptr);
  std::unique_ptr<device_grabber> device_grabber_ptr = std::make_unique<device_grabber>(*virtual_hid_device_client_ptr, *event_manipulator_ptr);
  connection_manager connection_manager(*version_monitor_ptr, *event_manipulator_ptr, *device_grabber_ptr);

  notification_center::post_distributed_notification_to_all_sessions(constants::get_distributed_notification_grabber_is_launched());

  CFRunLoopRun();

  device_grabber_ptr = nullptr;
  event_manipulator_ptr = nullptr;
  version_monitor_ptr = nullptr;

  return 0;
}
