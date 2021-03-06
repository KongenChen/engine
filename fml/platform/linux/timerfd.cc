// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/fml/platform/linux/timerfd.h"

#include <sys/types.h>
#include <unistd.h>

#include "flutter/fml/eintr_wrapper.h"

#if FML_TIMERFD_AVAILABLE == 0

#include <asm/unistd.h>
#include <sys/syscall.h>

int timerfd_create(int clockid, int flags) {
  return syscall(__NR_timerfd_create, clockid, flags);
}

int timerfd_settime(int ufc,
                    int flags,
                    const struct itimerspec* utmr,
                    struct itimerspec* otmr) {
  return syscall(__NR_timerfd_settime, ufc, flags, utmr, otmr);
}

#endif  // FML_TIMERFD_AVAILABLE == 0

namespace fml {

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000
#endif

bool TimerRearm(int fd, fml::TimePoint time_point) {
  const uint64_t nano_secs = time_point.ToEpochDelta().ToNanoseconds();

  struct itimerspec spec = {};
  spec.it_value.tv_sec = (time_t)(nano_secs / NSEC_PER_SEC);
  spec.it_value.tv_nsec = nano_secs % NSEC_PER_SEC;
  spec.it_interval = spec.it_value;  // single expiry.

  int result = ::timerfd_settime(fd, TFD_TIMER_ABSTIME, &spec, nullptr);
  return result == 0;
}

bool TimerDrain(int fd) {
  // 8 bytes must be read from a signalled timer file descriptor when signalled.
  uint64_t fire_count = 0;
  ssize_t size = FML_HANDLE_EINTR(::read(fd, &fire_count, sizeof(uint64_t)));
  if (size != sizeof(uint64_t)) {
    return false;
  }
  return fire_count > 0;
}

}  // namespace fml
