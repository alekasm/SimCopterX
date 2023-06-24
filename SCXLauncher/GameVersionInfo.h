#pragma once
#include <string>
#include <unordered_map>
#include "GameVersion.h"


struct GameVersionInfo
{
  std::string md5sum;
  std::string friendlyName;
  std::string version;
  std::string date;
};

static const std::unordered_map<GameVersions, GameVersionInfo> GameVersionInfoMap
{
  { GameVersions::VCLASSICS,{ "6bc646d182ab8625a0d2394112334005",
    "Classics Version", "(1.0.1.4)", "1 April 1997" }
  },
  { GameVersions::V11SC, { "90db54003aa9ba881543c9d2cd0dbfbf",
    "Version 1.1SC", "(1.0.1.0)", "8 December 1996" }
  },
  { GameVersions::V102_PATCH, { "d2f5c5eca71075696964d0f91b1163bf",
    "Version 1.02 Patch", "(1.0.1.3)", "26 February 1997" }
  },
  { GameVersions::V11SC_FR, { "b296b26e922bc43705b49f7414d7218f",
    "Version 1.1SC (French)", "(1.0.1.0)", "9 December 1996" }
  },
  { GameVersions::ORIGINAL, { "17d5eba3e604229c4b87a68f20520b56",
    "Version 1.0", "(1.0.0.0)", "14 November 1996" }
  },
  { GameVersions::V10_JP, { "1ea2ece4cf9b4e0ed3da217a31426795",
    "Version 1.0 (Japanese)", "(1.0.0.0)", "7 December 1996" }
  },
  { GameVersions::DEBUG, { "2970cdb003869392d9bff3253b25e720",
    "Debug Version 1.0.0.1", "(1.0.0.0)", "13 May 1996" }
  }
};

static bool GetGameVersion(std::string hash, GameVersions& version)
{
  std::unordered_map<GameVersions, GameVersionInfo>::const_iterator it;
  for (it = GameVersionInfoMap.begin(); it != GameVersionInfoMap.end(); ++it)
  {
    if (it->second.md5sum.compare(hash.c_str()) == 0)
    {
      version = it->first;
      return true;
    }
  }
  return false;
}

static std::string CreateVersionString(GameVersions version)
{
  std::unordered_map<GameVersions, GameVersionInfo>::const_iterator it;
  it = GameVersionInfoMap.find(version);
  if (it == GameVersionInfoMap.end())
    return "";
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%s %s - %s",
    it->second.friendlyName.c_str(),
    it->second.version.c_str(),
    it->second.date.c_str());
  return std::string(buffer);
}