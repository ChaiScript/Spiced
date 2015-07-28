
#ifndef CHAISCRIPT_CREATOR
#define CHAISCRIPT_CREATOR

namespace chaiscript {
  class ChaiScript;
}

namespace spiced {
  std::unique_ptr<chaiscript::ChaiScript> create_chaiscript();
}

#endif
