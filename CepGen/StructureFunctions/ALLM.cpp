#include "ALLM.h"

namespace CepGen
{
  namespace SF
  {
    ALLMParameterisation
    ALLMParameterisation::allm91()
    {
      ALLMParameterisation p;
      p.pomeron = Parameters(
        {  0.26550,  0.04856,  1.04682 },
        { -0.04503, -0.36407,  8.17091 },
        {  0.49222,  0.52116,  3.5515  } );
      p.reggeon = Parameters(
        {  0.67639,  0.49027,  2.66275 },
        {  0.60408,  0.17353,  1.61812 },
        {  1.26066,  1.83624,  0.81141 } );
      p.m02 = 0.30508;
      p.mp2 = 10.676;
      p.mr2 = 0.20623;
      p.q02 = 0.27799;
      p.lambda2 = 0.06527;
      return p;
    }

    ALLMParameterisation
    ALLMParameterisation::allm97()
    {
      ALLMParameterisation p;
      p.pomeron = Parameters(
        {  0.28067,  0.22291,  2.1979 },
        { -0.0808,  -0.44812,  1.1709 },
        {  0.36292,  1.8917,   1.8439 } );
      p.reggeon = Parameters(
        {  0.80107,  0.97307,  3.4924  },
        {  0.58400,  0.37888,  2.6063  },
        {  0.01147,  3.7582,   0.49338 } );
      p.m02 = 0.31985;
      p.mp2 = 49.457;
      p.mr2 = 0.15052;
      p.q02 = 0.52544;
      p.lambda2 = 0.06526;
      return p;
    }

    ALLMParameterisation
    ALLMParameterisation::hht_allm()
    {
      ALLMParameterisation p;
      p.pomeron = Parameters(
        {  0.412,  0.164,  17.7  },
        { -0.835, -0.446,  10.6  },
        { -45.8,    55.7, -0.031 } );
      p.reggeon = Parameters(
        { -1.04,   2.97,   0.163 },
        {  0.706,  0.185, -16.4  },
        { -1.29,   4.51,   1.16  } );
      p.m02 = 0.446;
      p.mp2 = 74.2;
      p.mr2 = 29.3;
      p.q02 = 4.74e-5;
      p.lambda2 = 2.2e-8;
      return p;
    }

    ALLMParameterisation
    ALLMParameterisation::hht_allm_ft()
    {
      ALLMParameterisation p;
      p.pomeron = Parameters(
        {  0.356,  0.171, 18.6  },
        { -0.075, -0.470, 9.2   },
        { -0.477,  54.0,  0.073 } );
      p.reggeon = Parameters(
        { -0.636, 3.37,  -0.660 },
        {  0.882, 0.082, -8.5   },
        {  0.339, 3.38,   1.07  } );
      p.m02 = 0.388;
      p.mp2 = 50.8;
      p.mr2 = 0.838;
      p.q02 = 1.87e-5;
      p.lambda2 = 4.4e-9;
      return p;
    }

    StructureFunctions
    ALLM( double q2, double xbj, const ALLMParameterisation& param )
    {
      const double factor = q2/( q2+param.m02 );
      const double W2_eff = q2*( 1.-xbj )/xbj;
      const double xp = ( q2+param.mp2 )/( q2+W2_eff+param.mp2 ),
                   xr = ( q2+param.mr2 )/( q2+W2_eff+param.mr2 );

      const double xlog1 = log( ( q2+param.q02 )/ param.lambda2 ), xlog2 = log( param.q02/param.lambda2 );
      const double t = log( xlog1/xlog2 );

      const double apom = param.pomeron.a[0] + ( param.pomeron.a[0]-param.pomeron.a[1] )*( 1./( 1.+pow( t, param.pomeron.a[2] ) ) - 1. );
      const double bpom = param.pomeron.b[0] + param.pomeron.b[1]*pow( t, param.pomeron.b[2] );
      const double cpom = param.pomeron.c[0] + ( param.pomeron.c[0]-param.pomeron.c[1] )*( 1./( 1.+pow( t, param.pomeron.c[2] ) ) - 1. );

      const double areg = param.reggeon.a[0] + param.reggeon.a[1]*pow( t, param.reggeon.a[2] );
      const double breg = param.reggeon.b[0] + param.reggeon.b[1]*pow( t, param.reggeon.b[2] );
      const double creg = param.reggeon.c[0] + param.reggeon.c[1]*pow( t, param.reggeon.c[2] );

      const double F2_Pom = cpom*pow( xp, apom )*pow( 1.-xbj, bpom ),
                   F2_Reg = creg*pow( xr, areg )*pow( 1.-xbj, breg );

      StructureFunctions allm;
      allm.F2 = factor * ( F2_Pom + F2_Reg );
      return allm;
    }
  }
}