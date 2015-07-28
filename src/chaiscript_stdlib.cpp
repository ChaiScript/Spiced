#include "ChaiScript/include/chaiscript/chaiscript_stdlib.hpp"
#include "chaiscript_stdlib.hpp"

namespace spiced {
  std::shared_ptr<chaiscript::Module> create_chaiscript_stdlib()
  {
    return chaiscript::Std_Lib::library();
  }
}

