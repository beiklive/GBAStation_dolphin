// Copyright 2026 Dolphin Emulator Project
// Copyright 2026 Dan | ticoverse.com
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <switch.h>

#include "AudioCommon/SoundStream.h"
#include "Common/CommonTypes.h"
#include "Core/Config/MainSettings.h"

namespace DolphinNX
{
namespace Audio
{

class SwitchStream final : public SoundStream
{
public:
  SwitchStream(unsigned int backendSampleRate = 48000);
  ~SwitchStream() override;

  bool Init() override;
  bool SetRunning(bool running) override;
  static bool IsValid() { return true; }

private:
  static void AudioThread(void* userdata);

  AudioDriver m_audio_driver{};
  Thread m_audio_thread{};
  u8* m_mem_pool = nullptr;
  u32 m_buffer_frames = 1024;
  std::atomic<bool> m_stop{false};
  std::atomic<bool> m_running{false};
};

}  // namespace Audio
}  // namespace DolphinNX
