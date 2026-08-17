// Copyright 2026 Dolphin Emulator Project
// Copyright 2026 Dan | ticoverse.com
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinNX/Audio.h"

#include <atomic>
#include <cstdlib>
#include <cstring>

#include "Common/Logging/Log.h"
#include "Common/Thread.h"

namespace DolphinNX
{
namespace Audio
{

SwitchStream::SwitchStream(unsigned int backendSampleRate)
    : SoundStream(backendSampleRate)
{
}

SwitchStream::~SwitchStream()
{
  if (m_mem_pool)
  {
    m_stop.store(true, std::memory_order_release);
    m_running.store(false, std::memory_order_release);
    threadWaitForExit(&m_audio_thread);
    threadClose(&m_audio_thread);
    audrvClose(&m_audio_driver);
    audrenExit();
    std::free(m_mem_pool);
    m_mem_pool = nullptr;
  }
}

bool SwitchStream::Init()
{
  static const AudioRendererConfig config = {
      .output_rate = AudioRendererOutputRate_48kHz,
      .num_voices = 4,
      .num_effects = 0,
      .num_sinks = 1,
      .num_mix_objs = 1,
      .num_mix_buffers = 2,
  };

  Result result = audrenInitialize(&config);
  if (R_FAILED(result))
  {
    ERROR_LOG_FMT(AUDIO, "audrenInitialize failed: 0x{:08X}", result);
    return false;
  }

  result = audrvCreate(&m_audio_driver, &config, 2);
  if (R_FAILED(result))
  {
    audrenExit();
    ERROR_LOG_FMT(AUDIO, "audrvCreate failed: 0x{:08X}", result);
    return false;
  }

  constexpr u32 channels = 2;
  constexpr u32 buffer_count = 2;
  const u32 pool_size = (m_buffer_frames * channels * sizeof(s16) * buffer_count +
                         AUDREN_MEMPOOL_ALIGNMENT - 1) &
                        ~(AUDREN_MEMPOOL_ALIGNMENT - 1);
  m_mem_pool = static_cast<u8*>(std::aligned_alloc(AUDREN_MEMPOOL_ALIGNMENT, pool_size));
  if (!m_mem_pool)
  {
    audrvClose(&m_audio_driver);
    audrenExit();
    ERROR_LOG_FMT(AUDIO, "Failed to allocate Switch audio buffers");
    return false;
  }

  const int mem_pool_id = audrvMemPoolAdd(&m_audio_driver, m_mem_pool, pool_size);
  audrvMemPoolAttach(&m_audio_driver, mem_pool_id);
  static const u8 channel_ids[] = {0, 1};
  audrvDeviceSinkAdd(&m_audio_driver, AUDREN_DEFAULT_DEVICE_NAME, channels, channel_ids);
  audrvUpdate(&m_audio_driver);
  audrenStartAudioRenderer();

  audrvVoiceInit(&m_audio_driver, 0, channels, PcmFormat_Int16, 48000);
  audrvVoiceSetDestinationMix(&m_audio_driver, 0, AUDREN_FINAL_MIX_ID);
  audrvVoiceSetMixFactor(&m_audio_driver, 0, 1.0f, 0, 0);
  audrvVoiceSetMixFactor(&m_audio_driver, 0, 1.0f, 1, 1);
  audrvVoiceStart(&m_audio_driver, 0);

  threadCreate(&m_audio_thread, AudioThread, this, nullptr, 1024 * 128, 0x20, 0);
  threadStart(&m_audio_thread);

  INFO_LOG_FMT(AUDIO, "Switch native audio: {}Hz, {} channels, {} samples", 48000, channels,
               m_buffer_frames);

  return true;
}

bool SwitchStream::SetRunning(bool running)
{
  if (running)
  {
    m_running.store(true, std::memory_order_release);
  }
  else
  {
    m_running.store(false, std::memory_order_release);
  }
  return true;
}

void SwitchStream::AudioThread(void* userdata)
{
  auto* self = static_cast<SwitchStream*>(userdata);
  AudioDriverWaveBuf buffers[2]{};
  for (u32 i = 0; i < 2; ++i)
  {
    buffers[i].data_pcm16 = reinterpret_cast<s16*>(self->m_mem_pool);
    buffers[i].size = self->m_buffer_frames * 2 * sizeof(s16);
    buffers[i].start_sample_offset = i * self->m_buffer_frames;
    buffers[i].end_sample_offset = buffers[i].start_sample_offset + self->m_buffer_frames;
  }

  while (!self->m_stop.load(std::memory_order_acquire))
  {
    AudioDriverWaveBuf* refill = nullptr;
    for (auto& buffer : buffers)
    {
      if (buffer.state == AudioDriverWaveBufState_Free || buffer.state == AudioDriverWaveBufState_Done)
      {
        refill = &buffer;
        break;
      }
    }

    if (refill)
    {
      auto* data = reinterpret_cast<s16*>(self->m_mem_pool) + refill->start_sample_offset * 2;
      if (self->m_running.load(std::memory_order_acquire) && self->GetMixer())
        self->GetMixer()->Mix(data, self->m_buffer_frames);
      else
        std::memset(data, 0, refill->size);

      armDCacheFlush(data, refill->size);
      audrvVoiceAddWaveBuf(&self->m_audio_driver, 0, refill);
      audrvVoiceStart(&self->m_audio_driver, 0);
    }

    audrvUpdate(&self->m_audio_driver);
    audrenWaitFrame();
  }
}

}  // namespace Audio
}  // namespace DolphinNX
