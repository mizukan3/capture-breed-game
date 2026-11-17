#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <cmath>
#include <numbers>
namespace capture::platform {
// SDL audio stream owns conversion/resampling. Only source UI tone is connected in this milestone.
class Audio {
 SDL_AudioStream* stream_{};
 bool enabled_{};
public:
 Audio() {
  SDL_AudioSpec spec{};spec.format=SDL_AUDIO_F32;spec.channels=1;spec.freq=48000;
  stream_=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
  if(stream_)SDL_ResumeAudioStreamDevice(stream_);
 }
 ~Audio(){if(stream_)SDL_DestroyAudioStream(stream_);}
 void enable(bool enabled){enabled_=enabled;if(!enabled&&stream_)SDL_ClearAudioStream(stream_);}
 bool enabled() const{return enabled_;}
 void pause(bool value){if(stream_){SDL_ClearAudioStream(stream_);if(value)SDL_PauseAudioStreamDevice(stream_);else SDL_ResumeAudioStreamDevice(stream_);}}
 void ui() {
  if(!enabled_||!stream_)return;
  // audio.js: MIDI81, .055s, amplitude .065, sfx bus .65. Original gain envelope.
  constexpr double duration=.055,attack=duration*.18,amplitude=.065,decay_end=.0001,frequency=880;
  std::array<float,3600> pcm{};
  for(std::size_t i=0;i<pcm.size();++i){const double t=i/48000.;double envelope=0;
   if(t<attack)envelope=amplitude*t/attack;
   else if(t<duration)envelope=amplitude*std::pow(decay_end/amplitude,(t-attack)/(duration-attack));
   pcm[i]=static_cast<float>(std::sin(2*std::numbers::pi*frequency*t)*envelope*.65);
  }
  SDL_PutAudioStreamData(stream_,pcm.data(),static_cast<int>(pcm.size()*sizeof(float)));
 }
};
}
