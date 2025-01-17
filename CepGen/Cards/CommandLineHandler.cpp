/*
 *  CepGen: a central exclusive processes event generator
 *  Copyright (C) 2020-2024  Laurent Forthomme
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

#include <fstream>

#include "CepGen/Cards/Handler.h"
#include "CepGen/Core/Exception.h"
#include "CepGen/Core/RunParameters.h"
#include "CepGen/Event/Event.h"
#include "CepGen/EventFilter/EventExporter.h"
#include "CepGen/EventFilter/EventModifier.h"
#include "CepGen/Modules/CardsHandlerFactory.h"
#include "CepGen/Modules/EventExporterFactory.h"
#include "CepGen/Modules/EventModifierFactory.h"
#include "CepGen/Modules/ProcessFactory.h"
#include "CepGen/Physics/PDG.h"
#include "CepGen/Process/Process.h"
#include "CepGen/Utils/TimeKeeper.h"

namespace cepgen {
  namespace card {
    /// Command line parser
    class CommandLineHandler final : public Handler {
    public:
      /// Cast command line arguments into a configuration word
      explicit CommandLineHandler(const ParametersList& params)
          : Handler(params), argv_(steer<std::vector<std::string> >("args")) {
        if (!filename_.empty())
          parseFile(filename_, rt_params_);
        for (const auto& arg : argv_)
          parseString(arg, rt_params_);
      }

      static ParametersDescription description() {
        auto desc = Handler::description();
        desc.setDescription("Command line configuration parser");
        desc.add<std::vector<std::string> >("args", {}).setDescription("Collection of arguments to be parsed");
        return desc;
      }

      RunParameters* parseString(const std::string&, RunParameters*) override;
      RunParameters* parseFile(const std::string&, RunParameters*) override;

    private:
      std::vector<std::string> argv_;
    };

    RunParameters* CommandLineHandler::parseFile(const std::string& filename, RunParameters* params) {
      rt_params_ = params;
      if (filename.empty())
        throw CG_FATAL("CommandLineHandler") << "Empty filename to be parsed! Aborting.";
      std::ifstream file(filename);
      std::string line;
      while (getline(file, line))
        rt_params_ = parseString(line, rt_params_);
      file.close();
      return rt_params_;
    }

    RunParameters* CommandLineHandler::parseString(const std::string& arg, RunParameters* params) {
      ParametersList pars;
      pars.feed(arg);
      CG_INFO("CommandLineHandler") << "Arguments list: " << arg << " unpacked to:\n\t" << pars << ".";

      rt_params_ = params;

      //----- timer definition
      if (pars.get<bool>("timer", false))
        rt_params_->setTimeKeeper(new utils::TimeKeeper);

      //----- logging definition
      if (pars.has<int>("logging"))
        utils::Logger::get().setLevel(pars.getAs<int, cepgen::utils::Logger::Level>("logging"));
      else if (pars.has<ParametersList>("logging")) {
        const auto& log = pars.get<ParametersList>("logging");
        if (log.has<int>("level"))
          utils::Logger::get().setLevel(log.getAs<int, cepgen::utils::Logger::Level>("level"));
        if (log.has<std::string>("modules"))
          utils::Logger::get().addExceptionRule(log.get<std::string>("modules"));
        else if (log.has<std::vector<std::string> >("modules"))
          for (const auto& mod : log.get<std::vector<std::string> >("modules"))
            utils::Logger::get().addExceptionRule(mod);
        utils::Logger::get().setExtended(log.get<bool>("extended", false));
      }

      //----- PDG definition
      auto pars_pdg = pars.get<ParametersList>("pdg");
      for (const auto& id : pars_pdg.keys())
        PDG::get().define(pars_pdg.get<ParticleProperties>(id));

      //----- phase space definition
      auto pars_kin = pars.get<ParametersList>("kinematics");

      //----- process definition
      auto proc = pars.get<ParametersList>("process");
      if (!proc.empty()) {
        if (rt_params_->hasProcess())
          proc = ParametersList(rt_params_->process().parameters()) + proc;
        rt_params_->setProcess(ProcessFactory::get().build(proc));
        if (proc.has<int>("mode"))
          pars_kin.set<int>("mode", proc.get<int>("mode"));
      }

      if (!pars_kin.empty()) {
        //----- set auxiliary information for phase space definition
        if (pars_kin.has<int>("strfun"))
          pars_kin.set<ParametersList>("structureFunctions", ParametersList().setName<int>(pars_kin.get<int>("strfun")))
              .erase("strfun");
        else if (pars_kin.has<ParametersList>("strfun"))
          pars_kin.rename("strfun", "structureFunctions");
        pars_kin.rename("formfac", "formFactors");

        //----- get the kinematics as already defined in the process object and modify it accordingly
        pars_kin = rt_params_->process().kinematics().parameters(true) + pars_kin;
        rt_params_->process().kinematics().setParameters(pars_kin);
      }

      //----- integration
      pars.fill<ParametersList>("integrator", rt_params_->integrator());

      //----- events generation
      const auto& gen = pars.get<ParametersList>("generation");
      rt_params_->generation().setMaxGen(gen.get<int>("ngen", rt_params_->generation().maxGen()));
      if (gen.has<int>("nthreads"))
        rt_params_->generation().setNumThreads(gen.get<int>("nthreads"));
      if (gen.has<int>("nprn"))
        rt_params_->generation().setPrintEvery(gen.get<int>("nprn"));
      if (gen.has<int>("seed"))
        rt_params_->integrator().set<int>("seed", gen.get<int>("seed"));

      //----- event modification modules
      const auto& mod = pars.get<ParametersList>("eventmod");
      if (!mod.keys(true).empty()) {
        rt_params_->addModifier(EventModifierFactory::get().build(mod));
        rt_params_->eventModifiersSequence().rbegin()->get()->initialise(*rt_params_);
      }

      //----- output modules definition
      const auto& out = pars.get<ParametersList>("output");
      if (!out.keys(true).empty())
        rt_params_->addEventExporter(EventExporterFactory::get().build(out));
      return rt_params_;
    }
  }  // namespace card
}  // namespace cepgen
using cepgen::card::CommandLineHandler;
REGISTER_CARD_HANDLER(".cmd", CommandLineHandler);
