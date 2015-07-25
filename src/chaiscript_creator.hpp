
#ifndef CHAISCRIPT_CREATOR
#define CHAISCRIPT_CREATOR

namespace chaiscript {
  class ChaiScript;
}

std::unique_ptr<chaiscript::ChaiScript> create_chaiscript();

#endif
