#ifndef CepGen_StructureFunctions_ALLM_h
#define CepGen_StructureFunctions_ALLM_h

#include "StructureFunctions.h"
#include <vector>

namespace CepGen
{
  namespace SF
  {
    class ALLM
    {
      public:
        class Parameterisation
        {
          private:
            struct Parameters {
              Parameters() :
                a( { 0., 0., 0. } ), b( { 0., 0., 0. } ), c( { 0., 0., 0. } ) {}
              Parameters( std::vector<double> c, std::vector<double> a, std::vector<double> b ) :
                a( a ), b( b ), c( c ) {}
              std::vector<double> a, b, c;
            };

          public:
            /// Pre-HERA data fit (694 data points)
            static Parameterisation allm91();
            /// Fixed target and HERA photoproduction total cross sections (1356 points)
            static Parameterisation allm97();
            static Parameterisation hht_allm();
            static Parameterisation hht_allm_ft();
            static Parameterisation gd07p();
            static Parameterisation gd11p();

            Parameters pomeron, reggeon;
            /// Effective photon squared mass
            double m02;
            /// Effective pomeron squared mass
            double mp2;
            /// Effective reggeon squared mass
            double mr2;
            double q02;
            /// Squared QCD scale
            double lambda2;
        };

        ALLM( const ALLM::Parameterisation& param = ALLM::Parameterisation::allm97() ) : params_( param ) {}
        StructureFunctions operator()( double q2, double xbj ) const;

      private:
        Parameterisation params_;
    };
  }
}

#endif
