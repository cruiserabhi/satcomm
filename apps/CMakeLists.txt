cmake_minimum_required(VERSION 3.10.2)

# Aggregate the various telematics application targets. The project name
# is referenced by downstream CMake files (e.g. telematics-apps_SOURCE_DIR).
project(telematics-apps)

# Console app framework used by many samples and tests.
add_subdirectory(common/console_app_framework)

# Reference applications.
add_subdirectory(reference)

# Sample applications (includes satcom samples such as HarmanSatCommService).
add_subdirectory(samples)

# Test applications.
add_subdirectory(tests)
