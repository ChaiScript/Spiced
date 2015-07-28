
#ifndef CHAISCRIPT_BINDINGS
#define CHAISCRIPT_BINDINGS

namespace chaiscript {
  class Module;
}

namespace spiced {
  std::shared_ptr<chaiscript::Module> create_chaiscript_bindings();
}

#endif
