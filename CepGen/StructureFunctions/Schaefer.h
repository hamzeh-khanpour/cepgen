#ifndef CepGen_StructureFunctions_Schaefer_h
#define CepGen_StructureFunctions_Schaefer_h

#include "StructureFunctions.h"

namespace CepGen
{
  namespace SF
  {
    class Schaefer : public StructureFunctions
    {
      public:
        struct Parameterisation
        {
          static Parameterisation standard();
          double amp, alpha_em;
          double q2_cut, w2_lo, w2_hi;
          int res_model, cont_model, higher_twist;
        };
        explicit Schaefer( const Parameterisation& param = Parameterisation::standard() );
        Schaefer& operator()( double q2, double xbj ) override;

      private:
        enum ResonancesModel { ChristyBosted = 1, FioreBrasse = 2 };
        enum ContinuumModel { GD11p = 1, ALLM91 = 2, ALLM97 = 3 };
    };
  }
}

#ifdef SchaeferF2
extern "C"
{
  extern void f2_fit_luxlike_( double& xbj, double& q2, double& F2, double& FL );
  extern CepGen::SF::Schaefer::Parameterisation luxlike_params_;
}
#endif

#endif
