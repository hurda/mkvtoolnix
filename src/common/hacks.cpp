/*
  mkvmerge -- utility for splicing together matroska files
      from component media subtypes

  hacks.cpp

  Written by Moritz Bunkus <moritz@bunkus.org>

  Distributed under the GPL
  see the file COPYING for details
  or visit http://www.gnu.org/copyleft/gpl.html
*/

/*!
    \file
    \version $Id$
    \brief hacks :)
    \author Moritz Bunkus <moritz@bunkus.org>
*/

#include "os.h"

#include <string.h>

#include <string>
#include <vector>

using namespace std;

#include "base64.h"
#include "common.h"
#include "hacks.h"

static const char *mosu_hacks[] = {
  ENGAGE_SPACE_AFTER_CHAPTERS,
  ENGAGE_NO_CHAPTERS_IN_META_SEEK,
  ENGAGE_NO_META_SEEK,
  ENGAGE_LACING_XIPH,
  ENGAGE_LACING_EBML,
  ENGAGE_NATIVE_BFRAMES,
  ENGAGE_NO_VARIABLE_DATA,
  ENGAGE_NO_DEFAULT_HEADER_VALUES,
  ENGAGE_FORCE_PASSTHROUGH_PACKETIZER,
  NULL
};
static vector<const char *> engaged_hacks;

bool hack_engaged(const char *hack) {
  uint32_t i;

  if (hack == NULL)
    return false;
  for (i = 0; i < engaged_hacks.size(); i++)
    if (!strcmp(engaged_hacks[i], hack))
      return true;

  return false;
}

void engage_hacks(const char *hacks) {
  vector<string> engage_args;
  int aidx, hidx;
  bool valid_hack;

  engage_args = split(hacks, ",");
  for (aidx = 0; aidx < engage_args.size(); aidx++)
    if (engage_args[aidx] == "list") {
      mxinfo("Valid hacks are:\n");
      for (hidx = 0; mosu_hacks[hidx] != NULL; hidx++)
        mxinfo("%s\n", mosu_hacks[hidx]);
      mxexit(0);
    } else if (engage_args[aidx] == "cow") {
      const string initial = "ICAgICAgICAgIChfXykKICAgICAgICAgICgqKikg"
        "IE9oIGhvbmV5LCB0aGF0J3Mgc28gc3dlZXQhCiAgIC8tLS0tLS0tXC8gICBPZiB"
        "jb3Vyc2UgSSdsbCBtYXJyeSB5b3UhCiAgLyB8ICAgICB8fAogKiAgfHwtLS0tfH"
        "wKICAgIF5eICAgIF5eCg==";
      char correction[200];
      memset(correction, 0, 200);
      base64_decode(initial, (unsigned char *)correction);
      mxinfo("%s", correction);
      mxexit(0);
    }
  for (aidx = 0; aidx < engage_args.size(); aidx++) {
    valid_hack = false;
    for (hidx = 0; mosu_hacks[hidx] != NULL; hidx++)
      if (engage_args[aidx] == mosu_hacks[hidx]) {
        valid_hack = true;
        engaged_hacks.push_back(mosu_hacks[hidx]);
        break;
      }
    if (!valid_hack)
      mxerror("'%s' is not a valid hack.\n", engage_args[aidx].c_str());
  }
}
