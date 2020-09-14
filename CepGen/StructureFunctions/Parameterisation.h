#ifndef CepGen_StructureFunctions_Parameterisation_h
#define CepGen_StructureFunctions_Parameterisation_h

#include "CepGen/Modules/NamedModule.h"

#include <iosfwd>
#include <memory>

namespace cepgen
{
  namespace sigrat { class Parameterisation; }
  /// Structure functions modelling scope
  namespace strfun
  {
    /// Proton structure function to be used in the outgoing state description
    /// \note Values correspond to the LPAIR legacy steering card values
    enum struct Type {
      Invalid             = 0,
      Electron            = 1,
      ElasticProton       = 2,
      SuriYennie          = 11,
      SzczurekUleshchenko = 12,
      BlockDurandHa       = 13,
      FioreBrasse         = 101,
      ChristyBosted       = 102,
      CLAS                = 103,
      ALLM91              = 201,
      ALLM97              = 202,
      GD07p               = 203,
      GD11p               = 204,
      MSTWgrid            = 205,
      Schaefer            = 301,
      Partonic            = 401,
    };
    /// Human-readable description of this SF parameterisation type
    std::ostream& operator<<( std::ostream&, const strfun::Type& );
    /// Generic placeholder for the parameterisation of nucleon structure functions
    class Parameterisation : public NamedModule<int>
    {
      public:
        /// Standard SF parameterisation constructor
        Parameterisation( double f2 = 0., double fl = 0. );
        /// Copy constructor
        Parameterisation( const Parameterisation& );
        /// User-steered parameterisation object constructor
        Parameterisation( const ParametersList& );
        virtual ~Parameterisation() = default;

        static std::string description() { return "Unnamed structure functions"; }

        /// Assign from another SF parameterisation object
        Parameterisation& operator=( const Parameterisation& sf ) {
          F2 = sf.F2, FL = sf.FL, old_vals_ = sf.old_vals_;
          return *this;
        }
        /// Human-readable dump of the SF parameterisation at this (xBj,Q^2) value
        friend std::ostream& operator<<( std::ostream&, const Parameterisation* );
        /// Human-readable dump of the SF parameterisation at this (xBj,Q^2) value
        friend std::ostream& operator<<( std::ostream&, const Parameterisation& );
        /// Human-readable description of this SF parameterisation
        virtual std::string describe() const; ///< Human-readable description of this SF set

        /// Set of parameters used to build this parameterisation
        const ParametersList& parameters() const { return params_; }
        /// Longitudinal/transverse cross section ratio parameterisation used to compute \f$F_{1/L}\f$
        const sigrat::Parameterisation* sigmaRatio() const { return r_ratio_.get(); }

        /// Compute all relevant structure functions for a given \f$(x_{\rm Bj},Q^2)\f$ couple
        virtual Parameterisation& operator()( double /*xbj*/, double /*q2*/ ) { return *this; }
        /// Compute the longitudinal structure function for a given point
        virtual Parameterisation& computeFL( double xbj, double q2 );
        /// Compute the longitudinal structure function for a given point
        virtual Parameterisation& computeFL( double xbj, double q2, double r );
        /// Compute the \f$F_1\f$ structure function for a given point
        double F1( double xbj, double q2 ) const;
        /// Compute the dimensionless variable \f$\tau=\frac{4x_{\rm Bj}^2m_p^2}{Q^2}\f$
        double tau( double xbj, double q2 ) const;

      public:
        double F2; ///< Last computed transverse structure function value
        double FL; ///< Last computed longitudinal structure function value

      protected:
        const double mp_; ///< Proton mass, in GeV/c\f$^2\f$
        const double mp2_; ///< Squared proton mass, in GeV\f$^2\f$/c\f$^4\f$
        ParametersList params_; ///< List of parameters used for this builder definition
        std::pair<double,double> old_vals_; ///< Last \f$(x_{\rm Bj},Q^2)\f$ couple computed

        /// Longitudinal/transverse cross section ratio parameterisation used to compute \f$F_{1/L}\f$
        std::shared_ptr<sigrat::Parameterisation> r_ratio_;
    };
  }
}

#endif
