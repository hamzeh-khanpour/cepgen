#include "CepGen/Hadronisers/GenericHadroniser.h"
#include "CepGen/Hadronisers/HadronisersHandler.h"

#include "CepGen/Event/Event.h"
#include "CepGen/Event/Particle.h"

#include "CepGen/Physics/PDG.h"

#include "CepGen/Core/ParametersList.h" //FIXME
#include "CepGen/Core/Exception.h"
#include "CepGen/Core/utils.h"

#include <algorithm>

extern "C"
{
  /// Get the particle's mass in GeV from Pythia
  extern double pymass_( int& );
  /// Launch the Pythia6 fragmentation
  extern void pyexec_();
  /// Set a parameter value to the Pythia module
  extern void pygive_( const char*, int );
  extern void pyckbd_();
  /// List all the particles in the event in a human-readable format
  extern void pylist_( int& );
  /// Join two coloured particles in a colour singlet
  extern void pyjoin_( int&, int& );
  /// Get a particle's human-readable name from Pythia
  extern void pyname_( int&, char*, int );
  /// Get integer-valued event information from Pythia
  extern int pyk_( int&, int& );
  /// Get real-valued event information from Pythia
  extern double pyp_( int&, int& );
  /// Purely virtual method to call at the end of the run
  void pystop_() { CG_INFO( "Pythia6Hadroniser" ) << "End of run"; }

  /// Particles content of the event
  extern struct
  {
    /// Number of particles in the event
    int n;
    int npad;
    /// Particles' general information (status, PDG id, mother, daughter 1, daughter 2)
    int k[5][4000];
    /// Particles' kinematics, in GeV (px, py, pz, E, M)
    double p[5][4000];
    /// Primary vertex for the particles
    double v[5][4000];
  } pyjets_;
}

namespace cepgen
{
  namespace hadr
  {
    /**
     * Full interface to the Pythia 6 algorithm. It can be used in a single particle decay mode as well as a full event hadronisation using the string model, as in Jetset.
     * \brief Pythia 6 hadronisation algorithm
     */
    class Pythia6Hadroniser : public GenericHadroniser
    {
      public:
        Pythia6Hadroniser( const ParametersList& );

        void setParameters( const Parameters& ) override {}
        inline void readString( const char* param ) override { pygive( param ); }
        void init() override {}
        bool run( Event& ev, double& weight, bool full ) override;

        void setCrossSection( double xsec, double xsec_err ) override {}

      private:
        static constexpr unsigned short MAX_PART_STRING = 3;
        static constexpr unsigned short MAX_STRING_EVENT = 2;
        /// Maximal number of characters to fetch for the particle's name
        static constexpr unsigned short NAME_CHR = 16;
        static const std::unordered_map<Particle::Status,int> kStatusMatchMap;

        inline static double pymass(int pdgid_) { return pymass_(pdgid_); }
        inline static void pyckbd() { pyckbd_(); }
        inline static void pygive( const std::string& line ) { pygive_( line.c_str(), line.length() ); }
        inline static void pylist( int mlist ) { pylist_( mlist ); }
        inline static int pyk( int id, int qty ) { return pyk_( id, qty ); }
        inline static double pyp( int id, int qty ) { return pyp_( id, qty ); }
        inline static std::string pyname( int pdgid ) {
          char out[NAME_CHR];
          std::string s;
          pyname_( pdgid, out, NAME_CHR );
          s = std::string( out, NAME_CHR );
          s.erase( remove( s.begin(), s.end(), ' ' ), s.end() );
          return s;
        }
        /// Connect entries with colour flow information
        /// \param[in] njoin Number of particles to join in the colour flow
        /// \param[in] ijoin List of particles unique identifier to join in the colour flow
        inline static void pyjoin( int njoin, int ijoin[2] ) { return pyjoin_( njoin, *ijoin ); }
        bool prepareHadronisation( Event& );
        std::pair<short,short> pickPartonsContent() const;
        unsigned int fillParticles( const Event& ) const;
    };

    Pythia6Hadroniser::Pythia6Hadroniser( const ParametersList& plist ) :
      GenericHadroniser( plist, "pythia6" )
    {}

    const std::unordered_map<Particle::Status,int>
    Pythia6Hadroniser::kStatusMatchMap = {
      { Particle::Status::PrimordialIncoming, 21 },
      { Particle::Status::FinalState, 1 },
      { Particle::Status::Unfragmented, 3 },
      { Particle::Status::Undecayed, 1 },
      { Particle::Status::Fragmented, 11 },
      { Particle::Status::Propagator, 11 },
      { Particle::Status::Incoming, 11 },
    };

    bool
    Pythia6Hadroniser::run( Event& ev, double& weight, bool full )
    {
      weight = 1.;

      if ( full )
        prepareHadronisation( ev );

      if ( CG_LOG_MATCH( "Pythia6Hadroniser:dump", debug ) ) {
        CG_DEBUG( "Pythia6Hadroniser" )
          << "Dump of the event before the hadronisation:";
        ev.dump();
      }

      //--- fill Pythia 6 common blocks
      const unsigned short str_in_evt = fillParticles( ev );

      CG_DEBUG( "Pythia6Hadroniser" )
        << "Passed the string construction stage.\n\t"
        << str_in_evt << " string object" << utils::s( str_in_evt )
        << " identified and constructed.";

      const int oldnpart = pyjets_.n;

      //--- run the algorithm
      pyexec_();

      if ( full && pyjets_.n == oldnpart )
        return false; // hadronisation failed

      //--- update the event
      for ( int p = oldnpart; p < pyjets_.n; ++p ) {
        // filter the first particles already present in the event
        const pdgid_t pdg_id = abs( pyjets_.k[1][p] );
        const short charge = pyjets_.k[1][p]/(short)pdg_id;
        ParticleProperties prop;
        if ( full )
          try { prop = PDG::get()( pdg_id ); } catch ( const Exception& ) {
            prop = ParticleProperties{ pdg_id,
              pyname( pdg_id ), pyname( pdg_id ),
              (short)pyk( p+1, 12 ), // colour factor
              pymass( pdg_id ), -1., //pmas( pdg_id, 2 ),
              (short)pyk( p+1, 6 ), // charge
              false
            };
            PDG::get().define( prop );
          }

        const unsigned short moth_id = pyjets_.k[2][p]-1;
        const Particle::Role role = pyjets_.k[2][p] != 0
          ? ev[moth_id].role() // child particle inherits its mother's role
          : Particle::Role::UnknownRole;

        auto& pa = ev.addParticle( role );
        pa.setId( p );
        pa.setPdgId( pdg_id, charge );
        int status = pyjets_.k[0][p];
        if ( status == 11 || status == 14 )
          status = -3;
        pa.setStatus( (Particle::Status)status );
        pa.setMomentum( Particle::Momentum( pyjets_.p[0][p], pyjets_.p[1][p], pyjets_.p[2][p], pyjets_.p[3][p] ) );
        pa.setMass( pyjets_.p[4][p] );
        if ( role != Particle::Role::UnknownRole ) {
          auto& moth = ev[moth_id];
          moth.setStatus( role == Particle::Role::CentralSystem
            ? Particle::Status::Resonance
            : Particle::Status::Fragmented );
          pa.addMother( moth );
        }
      }
      return true;
    }

    bool
    Pythia6Hadroniser::prepareHadronisation( Event& ev )
    {
      CG_DEBUG( "Pythia6Hadroniser" ) << "Hadronisation preparation called.";

      for ( const auto& part : ev.particles() ) {
        if ( part.status() != Particle::Status::Unfragmented )
          continue;
        //--- only loop over all protons to be fragmented

        const auto partons = pickPartonsContent();
        const double mx2 = part.mass2();
        const double mq = pymass( partons.first ), mq2 = mq*mq;
        const double mdq = pymass( partons.second ), mdq2 = mdq*mdq;

        //--- choose random direction in MX frame
        const double phi = 2.*M_PI*drand(), theta = acos( 2.*drand()-1. ); // theta angle

        //--- compute momentum of decay particles from MX
        const double px = std::sqrt( 0.25*std::pow( mx2-mdq2+mq2, 2 )/mx2-mq2 );

        //--- build 4-vectors and boost decay particles
        auto pdq = Particle::Momentum::fromPThetaPhi( px, theta, phi, std::hypot( px, mdq ) );
        auto pq = -pdq; pq.setEnergy( std::hypot( px, mq ) );

        //--- singlet
        auto& quark = ev.addParticle( part.role() );
        quark.addMother( ev[part.id()] );
        quark.setPdgId( partons.first, +1 );
        quark.setStatus( Particle::Status::Unfragmented );
        quark.setMomentum( pq.lorentzBoost( part.momentum() ) );

        //--- doublet
        auto& diquark = ev.addParticle( part.role() );
        diquark.addMother( ev[part.id()] );
        diquark.setPdgId( partons.second, +1 );
        diquark.setStatus( Particle::Status::Unfragmented );
        diquark.setMomentum( pdq.lorentzBoost( part.momentum() ) );

        ev[part.id()].setStatus( Particle::Status::Fragmented );
      }
      return true;
    }

    unsigned int
    Pythia6Hadroniser::fillParticles( const Event& ev ) const
    {
      pyjets_.n = 0;

      //--- initialising the string fragmentation variables
      unsigned int str_in_evt = 0;
      unsigned int num_part_in_str[MAX_STRING_EVENT] = { 0 };
      int jlpsf[MAX_STRING_EVENT][MAX_PART_STRING] = { 0 };

      for ( const auto& role : ev.roles() ) { // loop on roles
        unsigned int part_in_str = 0;
        bool role_has_string = false;
        for ( const auto& part : ev[role] ) {
          unsigned int np = part.id();

          pyjets_.p[0][np] = part.momentum().px();
          pyjets_.p[1][np] = part.momentum().py();
          pyjets_.p[2][np] = part.momentum().pz();
          pyjets_.p[3][np] = part.energy();
          pyjets_.p[4][np] = part.mass();
          try {
            pyjets_.k[0][np] = kStatusMatchMap.at( part.status() );
          } catch ( const std::out_of_range& ) {
            ev.dump();
            throw CG_FATAL( "Pythia6Hadroniser" )
              << "Failed to retrieve a Pythia 6 particle status translation for "
              << "CepGen status " << (int)part.status() << "!";
          }
          pyjets_.k[1][np] = part.integerPdgId();
          const auto& moth = part.mothers();
          pyjets_.k[2][np] = moth.empty()
            ? 0 // no mother
            : *moth.begin()+1; // mother
          const auto& daug = part.daughters();
          if ( daug.empty() )
            pyjets_.k[3][np] = pyjets_.k[4][np] = 0; // no daughters
          else {
            pyjets_.k[3][np] = *daug.begin()+1; // daughter 1
            pyjets_.k[4][np] = *daug.rbegin()+1; // daughter 2
          }
          for ( int i = 0; i < 5; ++i )
            pyjets_.v[i][np] = 0.; // vertex position

          if ( part.status() == Particle::Status::Unfragmented ) {
            pyjets_.k[0][np] = 1; // PYTHIA/JETSET workaround
            jlpsf[str_in_evt][part_in_str++] = part.id()+1;
            num_part_in_str[str_in_evt]++;
            role_has_string = true;
          }
          else if ( part.status() == Particle::Status::Undecayed )
            pyjets_.k[0][np] = 2; // intermediate resonance
          pyjets_.n++;
        }
        //--- at most one string per role
        if ( role_has_string )
          str_in_evt++;
      }

      //--- loop over the strings to bind everything together
      for ( unsigned short i = 0; i < str_in_evt; ++i ) {
        if ( num_part_in_str[i] < 2 )
          continue;

        if ( CG_LOG_MATCH( "Pythia6Hadroniser", debug ) ) {
          std::ostringstream dbg;
          for ( unsigned short j = 0; j < num_part_in_str[i]; ++j )
            if ( jlpsf[i][j] != -1 )
              dbg << Form( "\n\t * %2d (pdgId=%4d)", jlpsf[i][j], pyjets_.k[1][jlpsf[i][j]-1] );
          CG_DEBUG( "Pythia6Hadroniser" )
            << "Joining " << num_part_in_str[i] << " particle" << utils::s( num_part_in_str[i] )
            << " with " << ev[jlpsf[i][0]].role() << " role"
            << " in a same string (id=" << i << ")"
            << dbg.str();
        }
        pyjoin( num_part_in_str[i], jlpsf[i] );
      }
      return str_in_evt;
    }

    std::pair<short,short>
    Pythia6Hadroniser::pickPartonsContent() const
    {
      const double ranudq = drand();
      if ( ranudq < 1./9. )
        return { PDG::down, 2203 }; // (d,uu1)
      if ( ranudq < 5./9. )
        return { PDG::up, 2101 };   // (u,ud0)
      return { PDG::up, 2103 };     // (u,ud1)
    }

  }
}

// register hadroniser and define alias
REGISTER_HADRONISER( pythia6, Pythia6Hadroniser )

