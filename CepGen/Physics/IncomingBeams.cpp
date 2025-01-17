/*
 *  CepGen: a central exclusive processes event generator
 *  Copyright (C) 2013-2023  Laurent Forthomme
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

#include <cmath>

#include "CepGen/Core/Exception.h"
#include "CepGen/Modules/FormFactorsFactory.h"
#include "CepGen/Modules/PartonFluxFactory.h"
#include "CepGen/Physics/HeavyIon.h"
#include "CepGen/Physics/IncomingBeams.h"
#include "CepGen/Physics/Modes.h"
#include "CepGen/Physics/Momentum.h"
#include "CepGen/Physics/PDG.h"
#include "CepGen/StructureFunctions/Parameterisation.h"
#include "CepGen/Utils/Math.h"

namespace cepgen {
  IncomingBeams::IncomingBeams(const ParametersList& params) : SteeredObject(params) {
    (*this).add("formFactors", formfac_).add("structureFunctions", strfun_);
  }

  void IncomingBeams::setParameters(const ParametersList& params) {
    SteeredObject::setParameters(params);
    ParametersList plist_pos, plist_neg;
    auto& pos_pdg = plist_pos.operator[]<int>("pdgId");
    auto& neg_pdg = plist_neg.operator[]<int>("pdgId");

    // positive-z incoming beam
    if (const auto& hi_Z1 = steerAs<int, Element>("beam1Z"); hi_Z1 != Element::invalid)
      pos_pdg = HeavyIon(steer<int>("beam1A"), hi_Z1);
    else if (const auto& hi_beam1 = steer<std::vector<int> >("heavyIon1"); hi_beam1.size() >= 2)
      pos_pdg = HeavyIon{(unsigned short)hi_beam1.at(0), static_cast<Element>(hi_beam1.at(1))};
    else
      pos_pdg = steer<int>("beam1id");

    // negative-z incoming beam
    if (const auto& hi_Z2 = steerAs<int, Element>("beam2Z"); hi_Z2 != Element::invalid)
      neg_pdg = HeavyIon(steer<int>("beam2A"), hi_Z2);
    else if (const auto& hi_beam2 = steer<std::vector<int> >("heavyIon2"); hi_beam2.size() >= 2)
      neg_pdg = HeavyIon{(unsigned short)hi_beam2.at(0), (Element)hi_beam2.at(1)};
    else
      neg_pdg = steer<int>("beam2id");

    //----- combined two-beam system

    //--- beams PDG ids
    if (const auto& beams_pdg = steer<std::vector<ParametersList> >("pdgIds"); beams_pdg.size() >= 2) {
      pos_pdg = std::abs(beams_pdg.at(0).get<int>("pdgid"));
      neg_pdg = std::abs(beams_pdg.at(1).get<int>("pdgid"));
    } else if (const auto& beams_pdg = steer<std::vector<int> >("pdgIds"); beams_pdg.size() >= 2) {
      pos_pdg = std::abs(beams_pdg.at(0));
      neg_pdg = std::abs(beams_pdg.at(1));
    }

    //--- beams longitudinal momentum
    double p1z = 0., p2z = 0;
    if (const auto& beams_pz = steer<std::vector<double> >("pz"); beams_pz.size() >= 2) {
      // fill from beam momenta
      p1z = beams_pz.at(0);
      p2z = beams_pz.at(1);
    } else if (const auto& beams_ene = steer<std::vector<double> >("energies"); beams_ene.size() >= 2) {
      // fill from beam energies
      p1z = utils::fastSqrtSqDiff(beams_ene.at(0), PDG::get().mass(pos_pdg));
      p2z = utils::fastSqrtSqDiff(beams_ene.at(1), PDG::get().mass(neg_pdg));
    } else {
      // when everything failed, retrieve "beamNpz" attributes
      params_.fill<double>("beam1pz", p1z);
      params_.fill<double>("beam2pz", p2z);
      if (pos_pdg == neg_pdg) {  // special case: symmetric beams -> fill from centre-of-mass energy
        if (const auto sqrts = params_.has<double>("sqrtS") && steer<double>("sqrtS") > 0. ? steer<double>("sqrtS")
                               : params_.has<double>("cmEnergy") && steer<double>("cmEnergy") > 0.
                                   ? steer<double>("cmEnergy")
                                   : 0.;
            sqrts > 0.) {  // compute momenta from energy
          const auto pz_abs = utils::fastSqrtSqDiff(0.5 * sqrts, PDG::get().mass(pos_pdg));
          p1z = +pz_abs;
          p2z = -pz_abs;
        }
      }
    }
    //--- check the sign of both beams' pz
    if (p1z * p2z < 0. && p1z < 0.)
      std::swap(p1z, p2z);
    else if (p1z * p2z > 0. && p2z > 0.)
      p2z *= -1.;
    plist_pos.set<double>("pz", +fabs(p1z));
    plist_neg.set<double>("pz", -fabs(p2z));

    //--- parton fluxes
    auto set_part_fluxes_from_name_vector = [&plist_pos, &plist_neg](const std::vector<std::string>& fluxes) {
      if (fluxes.empty())
        return;
      plist_pos.set<ParametersList>("partonFlux",
                                    PartonFluxFactory::get().describeParameters(fluxes.at(0)).parameters());
      plist_neg.set<ParametersList>("partonFlux",
                                    fluxes.size() > 1
                                        ? PartonFluxFactory::get().describeParameters(fluxes.at(1)).parameters()
                                        : plist_pos.get<ParametersList>("partonFlux"));
    };
    auto set_part_fluxes_from_name = [&plist_pos, &plist_neg](const std::string& fluxes) {
      if (fluxes.empty())
        return;
      const auto params = PartonFluxFactory::get().describeParameters(fluxes).parameters();
      plist_pos.set<ParametersList>("partonFlux", params);
      plist_neg.set<ParametersList>("partonFlux", params);
    };

    if (const auto& fluxes = steer<std::vector<ParametersList> >("partonFluxes"); fluxes.size() >= 2) {
      plist_pos.set("partonFlux", fluxes.at(0));
      plist_neg.set("partonFlux", fluxes.at(1));
    } else if (const auto& fluxes = steer<ParametersList>("partonFluxes"); !fluxes.empty()) {
      plist_pos.set("partonFlux", fluxes);
      plist_neg.set("partonFlux", fluxes);
    } else if (const auto& fluxes = steer<std::vector<std::string> >("partonFluxes"); !fluxes.empty())
      set_part_fluxes_from_name_vector(fluxes);
    else if (const auto& fluxes = steer<std::vector<std::string> >("ktFluxes"); !fluxes.empty())
      set_part_fluxes_from_name_vector(fluxes);
    else if (const auto& flux = steer<std::string>("partonFluxes"); flux.empty())
      set_part_fluxes_from_name(flux);
    else if (const auto& flux = steer<std::string>("ktFluxes"); !flux.empty())
      set_part_fluxes_from_name(flux);

    //--- form factors
    plist_pos.set<ParametersList>("formFactors", steer<ParametersList>("formFactors"));
    plist_neg.set<ParametersList>("formFactors", steer<ParametersList>("formFactors"));

    if (auto mode = steerAs<int, mode::Kinematics>("mode"); mode != mode::Kinematics::invalid) {
      plist_pos.set<bool>("elastic",
                          mode == mode::Kinematics::ElasticElastic || mode == mode::Kinematics::ElasticInelastic);
      plist_neg.set<bool>("elastic",
                          mode == mode::Kinematics::ElasticElastic || mode == mode::Kinematics::InelasticElastic);
    } else {
      const auto set_beam_elasticity = [](ParametersList& plist_beam) {
        plist_beam.set<bool>("elastic", PartonFluxFactory::get().elastic(plist_beam.get<ParametersList>("partonFlux")));
      };
      set_beam_elasticity(plist_pos);
      set_beam_elasticity(plist_neg);
    }

    //--- structure functions
    if (!steer<ParametersList>("structureFunctions").empty()) {
      plist_pos.set<ParametersList>("structureFunctions", steer<ParametersList>("structureFunctions"));
      plist_neg.set<ParametersList>("structureFunctions", steer<ParametersList>("structureFunctions"));
    }
    CG_DEBUG("IncomingBeams") << "Will build the following incoming beams:\n* " << plist_pos << "\n* " << plist_neg
                              << ".";
    pos_beam_ = Beam(plist_pos).setPdgId(pos_pdg);
    neg_beam_ = Beam(plist_neg).setPdgId(neg_pdg);
  }

  void IncomingBeams::setSqrtS(double sqrts) {
    if (pos_beam_.pdgId() != neg_beam_.pdgId())
      throw CG_FATAL("IncomingBeams:setSqrtS") << "Trying to set √s with asymmetric beams"
                                               << " (" << pos_beam_.pdgId() << "/" << neg_beam_.pdgId() << ").\n"
                                               << "Please fill incoming beams objects manually!";
    const auto pz_abs = utils::fastSqrtSqDiff(0.5 * sqrts, PDG::get().mass(pos_beam_.pdgId()));
    pos_beam_.setMomentum(Momentum::fromPxPyPzM(0., 0., +pz_abs, PDG::get().mass(pos_beam_.pdgId())));
    neg_beam_.setMomentum(Momentum::fromPxPyPzM(0., 0., -pz_abs, PDG::get().mass(neg_beam_.pdgId())));
  }

  double IncomingBeams::s() const {
    const auto sval = (pos_beam_.momentum() + neg_beam_.momentum()).mass2();
    CG_DEBUG("IncomingBeams:s") << "Beams momenta:\n"
                                << "\t" << pos_beam_.momentum() << "\n"
                                << "\t" << neg_beam_.momentum() << "\n"
                                << "\ts = (p1 + p2)^2 = " << sval << ", sqrt(s) = " << std::sqrt(sval) << ".";
    return sval;
  }

  double IncomingBeams::sqrtS() const { return std::sqrt(s()); }

  mode::Kinematics IncomingBeams::mode() const {
    if (const auto& mode = steerAs<int, mode::Kinematics>("mode"); mode != mode::Kinematics::invalid)
      return mode;
    return modeFromBeams(pos_beam_, neg_beam_);
  }

  mode::Kinematics IncomingBeams::modeFromBeams(const Beam& pos, const Beam& neg) {
    if (pos.elastic()) {
      if (neg.elastic())
        return mode::Kinematics::ElasticElastic;
      else
        return mode::Kinematics::ElasticInelastic;
    }
    if (neg.elastic())
      return mode::Kinematics::InelasticElastic;
    else
      return mode::Kinematics::InelasticInelastic;
  }

  void IncomingBeams::setStructureFunctions(int sf_model, int sr_model) {
    const unsigned long kLHAPDFCodeDec = 10000000, kLHAPDFPartDec = 1000000;
    sf_model = (sf_model == 0 ? 11 /* SuriYennie */ : sf_model);
    sr_model = (sr_model == 0 ? 4 /* SibirtsevBlunden */ : sr_model);
    auto& sf_params = params_.operator[]<ParametersList>("structureFunctions");
    sf_params.setName<int>(sf_model).set<int>("sigmaRatio", sr_model);
    if (sf_model / kLHAPDFCodeDec == 1) {  // SF from parton
      const unsigned long icode = sf_model % kLHAPDFCodeDec;
      sf_params.setName<int>(401 /* Partonic */)
          .set<int>("pdfId", icode % kLHAPDFPartDec)
          .set<int>("mode", icode / kLHAPDFPartDec);  // 0, 1, 2
    }
    CG_DEBUG("IncomingBeams:setStructureFunctions")
        << "Structure functions modelling to be built: " << sf_params << ".";
  }

  const ParametersList& IncomingBeams::parameters() const {
    params_ = SteeredObject::parameters();
    params_
        //.set<std::vector<int> >("partonFluxes", {(int)pos_beam_.partonFlux(), (int)neg_beam_.partonFlux()})
        .set<int>("beam1id", pos_beam_.pdgId())
        .set<double>("beam1pz", +pos_beam_.momentum().pz())
        .set<int>("beam2id", neg_beam_.pdgId())
        .set<double>("beam2pz", -neg_beam_.momentum().pz())
        .setAs<int, mode::Kinematics>("mode", mode());
    if (HeavyIon::isHI(pos_beam_.pdgId())) {
      const auto hi1 = HeavyIon::fromPdgId(pos_beam_.pdgId());
      params_.set<int>("beam1A", hi1.A).set<int>("beam1Z", (int)hi1.Z);
    }
    if (HeavyIon::isHI(neg_beam_.pdgId())) {
      const auto hi2 = HeavyIon::fromPdgId(neg_beam_.pdgId());
      params_.set<int>("beam2A", hi2.A).set<int>("beam2Z", (int)hi2.Z);
    }
    return params_;
  }

  ParametersDescription IncomingBeams::description() {
    auto desc = ParametersDescription();
    desc.addAs<int, pdgid_t>("beam1id", PDG::invalid).setDescription("PDG id of the positive-z beam particle");
    desc.add<int>("beam1A", 0).setDescription("Atomic weight of the positive-z ion beam");
    desc.addAs<int, Element>("beam1Z", Element::invalid).setDescription("Atomic number of the positive-z ion beam");
    desc.addAs<int, pdgid_t>("beam2id", PDG::invalid).setDescription("PDG id of the negative-z beam particle");
    desc.add<int>("beam2A", 0).setDescription("Atomic weight of the negative-z ion beam");
    desc.addAs<int, Element>("beam2Z", Element::invalid).setDescription("Atomic number of the negative-z ion beam");
    desc.add<std::vector<ParametersList> >("pdgIds", {}).setDescription("PDG description of incoming beam particles");
    desc.add<std::vector<int> >("pdgIds", {}).setDescription("PDG ids of incoming beam particles");
    desc.add<std::vector<double> >("pz", {}).setDescription("Beam momenta (in GeV/c)");
    desc.add<std::vector<double> >("energies", {}).setDescription("Beam energies (in GeV/c)");
    desc.add<double>("sqrtS", 0.).setDescription("Two-beam centre of mass energy (in GeV)");
    desc.addAs<int, mode::Kinematics>("mode", mode::Kinematics::invalid)
        .setDescription("Process kinematics mode (1 = elastic, (2-3) = single-dissociative, 4 = double-dissociative)");
    desc.add<ParametersDescription>("formFactors",
                                    FormFactorsFactory::get().describeParameters(formfac::gFFStandardDipoleHandler))
        .setDescription("Beam form factors modelling");
    desc.add<ParametersDescription>("structureFunctions",
                                    strfun::Parameterisation::description().setName<int>(11 /*default is SY*/))
        .setDescription("Beam inelastic structure functions modelling");
    return desc;
  }
}  // namespace cepgen
