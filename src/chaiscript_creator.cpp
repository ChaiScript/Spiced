#include "ChaiScript/include/chaiscript/chaiscript.hpp"
#include "chaiscript_stdlib.hpp"
#include "chaiscript_bindings.hpp"
#include "chaiscript_creator.hpp"

namespace spiced {
  std::unique_ptr<chaiscript::ChaiScript> create_chaiscript()
  {
    auto chai = std::unique_ptr<chaiscript::ChaiScript>(new chaiscript::ChaiScript(create_chaiscript_stdlib()));
    chai->add(create_chaiscript_bindings());
    return chai;
  }

}