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
        void preparePhaseSpace() override;
        double computeKTFactorisedMatrixElement() override;
        void fillCentralParticlesKinematics() override;
        double WWamplitude( double shat, double that, double uhat, short lam1, short lam2, short lam3, short lam4 ) const;

        /// Rapidity range for the outgoing W bosons
        Kinematics::Limits rap_limits_;
        /// Rapidity of the first outgoing W boson
        double y1_;
        /// Rapidity of the first outgoing W boson
        double y2_;

        Kinematics::Limits ptdiff_limits_;
        /// Transverse momentum difference for the two outgoing W bosons
        double pt_diff_;

        Kinematics::Limits phi_pt_diff_limits_;
        /// Azimuthal angle difference for the two outgoing W bosons
        double phi_pt_diff_;

        /// First outgoing W boson's momentum
        Particle::Momentum p_w1_;
        /// Second outgoing W boson's momentum
        Particle::Momentum p_w2_;
    };
  }
}

#endif

