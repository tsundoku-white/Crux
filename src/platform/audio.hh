#pragma once

#include "src/core/pch.hh"

class Audio
{
private:
public:
  void load(std::string &path);
  void play(bool once);
  void stop();
};
