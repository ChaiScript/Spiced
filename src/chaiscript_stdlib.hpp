
#ifndef CHAISCRIPT_STDLIB
#define CHAISCRIPT_STDLIB

namespace chaiscript {
  class Module;
}

namespace spiced {
  std::shared_ptr<chaiscript::Module> create_chaiscript_stdlib();
}

#endif
