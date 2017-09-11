#ifndef CepGen_Cards_Handler_h
#define CepGen_Cards_Handler_h

#include "CepGen/Parameters.h"

namespace CepGen
{
  class Parameters;
  /// Location for all steering card parsers/writers
  namespace Cards
  {
    /// Generic steering card handler
    class Handler
    {
      public:
        /// Build a configuration from an external steering card
        Handler() {}
        ~Handler() {}

        /// Retrieve a configuration from a parsed steering cart
        Parameters& parameters() { return params_; }

      protected:
        /// List of parameters parsed from a card handler
        Parameters params_;
    };
  }
}

#endif
