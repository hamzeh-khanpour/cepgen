#ifndef CepGen_Processes_PPtoWW_h
#define CepGen_Processes_PPtoWW_h

#include "GenericKTProcess.h"

namespace CepGen
{
  namespace Process
  {
    /// Compute the matrix element for a CE \f$\gamma\gamma\rightarrow W^+W^-\f$ process using \f$k_T\f$-factorization approach
    class PPtoWW : public GenericKTProcess
    {
      public:
        PPtoWW();
        inline ~PPtoWW() {}

      private:
        void prepareKTKinematics();
        double computeJacobian();
        double computeKTFactorisedMatrixElement();
        void fillCentralParticlesKinematics();

        /// Minimal rapidity of the first outgoing W boson
        double y_min_;
        /// Maximal rapidity of the first outgoing W boson
        double y_max_;
        /// Rapidity of the first outgoing W boson
        double y1_;
        /// Rapidity of the first outgoing W boson
        double y2_;
        /// Transverse momentum difference for the two outgoing W bosons
        double pt_diff_;
        /// Azimuthal angle difference for the two outgoing W bosons
        double phi_pt_diff_;

        // first outgoing W boson
        Particle::Momentum p_w1_;
        // second outgoing W boson
        Particle::Momentum p_w2_;
    };
  }
}

#endif

