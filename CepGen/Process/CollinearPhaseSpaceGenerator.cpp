/*
 *  CepGen: a central exclusive processes event generator
 *  Copyright (C) 2023  Laurent Forthomme
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CepGen/CollinearFluxes/CollinearFlux.h"
#include "CepGen/Core/Exception.h"
#include "CepGen/Modules/PartonFluxFactory.h"
#include "CepGen/Physics/Beam.h"
#include "CepGen/Physics/HeavyIon.h"
#include "CepGen/Physics/PDG.h"
#include "CepGen/Process/CollinearPhaseSpaceGenerator.h"
#include "CepGen/Process/Process.h"

namespace cepgen {
  namespace proc {
    CollinearPhaseSpaceGenerator::CollinearPhaseSpaceGenerator(Process* proc) : PhaseSpaceGenerator(proc) {}

    void CollinearPhaseSpaceGenerator::initialise() {
      const auto& kin = process().kinematics();

      // pick a parton flux parameterisation for each beam
      auto set_flux_properties = [&kin](const Beam& beam, std::unique_ptr<PartonFlux>& flux) {
        auto params = beam.partonFluxParameters();
        const auto params_p_el = CollinearFluxFactory::get().describeParameters(
            "EPAFlux", ParametersList().set("formFactors", kin.incomingBeams().formFactors()));
        const auto params_p_inel = CollinearFluxFactory::get().describeParameters(
            "EPAFlux",
            ParametersList().set("formFactors",
                                 ParametersList()
                                     .setName<std::string>("InelasticNucleon")
                                     .set("structureFunctions", kin.incomingBeams().structureFunctions())));
        const auto params_hi_el = CollinearFluxFactory::get().describeParameters(
            "EPAFlux", ParametersList().set("formFactors", ParametersList().setName<std::string>("HeavyIonDipole")));
        if (params.name<std::string>().empty()) {
          if (beam.elastic()) {
            if (HeavyIon::isHI(beam.pdgId()))
              params = params_hi_el.validate(params);
            else
              params = params_p_el.validate(params);
          } else
            params = params_p_inel.validate(params);
          //TODO: fermions/pions
        }
        flux = std::move(CollinearFluxFactory::get().build(params));
        if (!flux)
          throw CG_FATAL("CollinearPhaseSpaceGenerator:init")
              << "Failed to initiate a parton flux object with properties: " << params << ".";
        if (flux->ktFactorised())
          throw CG_FATAL("CollinearPhaseSpaceGenerator:init")
              << "Invalid incoming parton flux: " << flux->name() << ".";
      };
      set_flux_properties(kin.incomingBeams().positive(), pos_flux_);
      set_flux_properties(kin.incomingBeams().negative(), neg_flux_);

      // register the incoming partons' virtuality
      const auto log_lim_q2 = kin.cuts().initial.q2.truncate(Limits{1.e-10, 5.}).compute(std::log);
      process().defineVariable(m_t1_, Process::Mapping::exponential, log_lim_q2, "Positive-z parton virtuality");
      process().defineVariable(m_t2_, Process::Mapping::exponential, log_lim_q2, "Negative-z parton virtuality");
    }

    bool CollinearPhaseSpaceGenerator::generatePartonKinematics() {
      // gaussian smearing of kt can be introduced here
      process().q1() = Momentum::fromPtYPhiM(0., 0., 0., std::sqrt(m_t1_));
      process().q2() = Momentum::fromPtYPhiM(0., 0., 0., std::sqrt(m_t2_));
      return true;
    }

    double CollinearPhaseSpaceGenerator::fluxes() const {
      return positiveFlux<CollinearFlux>().fluxQ2(process().x1(), m_t1_) * process().x1() / m_t1_ *
             negativeFlux<CollinearFlux>().fluxQ2(process().x2(), m_t2_) * process().x2() / m_t2_;
    }
  }  // namespace proc
}  // namespace cepgen
