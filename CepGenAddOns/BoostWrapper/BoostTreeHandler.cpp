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

#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "CepGen/Cards/Handler.h"
#include "CepGen/Core/Exception.h"
#include "CepGen/Core/ParametersList.h"
#include "CepGen/Core/RunParameters.h"
#include "CepGen/EventFilter/EventExporter.h"
#include "CepGen/EventFilter/EventModifier.h"
#include "CepGen/Generator.h"  // for library loading
#include "CepGen/Modules/CardsHandlerFactory.h"
#include "CepGen/Modules/EventExporterFactory.h"
#include "CepGen/Modules/EventModifierFactory.h"
#include "CepGen/Modules/ProcessFactory.h"
#include "CepGen/Process/Process.h"
#include "CepGen/Utils/TimeKeeper.h"
#include "CepGenAddOns/BoostWrapper/BoostTreeUtils.h"

namespace pt = boost::property_tree;
namespace bc = boost::cepgen;

namespace cepgen {
  namespace card {
    /// Boost tree configuration cards reader/writer
    class BoostTreeHandler : public Handler {
    public:
      /// Boost tree parser from a configuration card
      explicit BoostTreeHandler(const ParametersList&);

      static ParametersDescription description();

      RunParameters* parseFile(const std::string&, RunParameters*) override;
      void pack(const RunParameters* params) override;

    protected:
      /// Read and cast a file into the property tree
      virtual void read(const std::string&) = 0;

      /// The BOOST property tree translated by this configuration
      pt::ptree tree_;

    private:
      static constexpr const char* ADDONS_NAME = "addons";
      static constexpr const char* PROCESS_NAME = "process";
      static constexpr const char* KIN_NAME = "kinematics";
      static constexpr const char* INTEGR_NAME = "integrator";
      static constexpr const char* GENERATOR_NAME = "generator";
      static constexpr const char* EVT_MOD_SEQ_NAME = "eventSequence";
      static constexpr const char* OUTPUT_NAME = "output";
      static constexpr const char* TIMER_NAME = "timer";
      static constexpr const char* LOGGER_NAME = "logger";

      ParametersList proc_, gen_, log_;
      ParametersList evt_mod_, evt_out_;
    };

    BoostTreeHandler::BoostTreeHandler(const ParametersList& params) : Handler(params) {}

    RunParameters* BoostTreeHandler::parseFile(const std::string& filename, RunParameters* params) {
      rt_params_ = params;
      read(filename);

      if (tree_.count(ADDONS_NAME))
        for (const auto& lib : bc::unpack(tree_.get_child(ADDONS_NAME)).keys())
          loadLibrary(lib);

      try {
        proc_ = bc::unpack(tree_.get_child(PROCESS_NAME));
        rt_params_->setProcess(ProcessFactory::get().build(proc_));
      } catch (const boost::exception&) {
        throw CG_FATAL("BoostTreeHandler") << "Failed to retrieve a valid \"" << PROCESS_NAME << "\" block"
                                           << " in the steering card!";
      }
      ParametersList par_kinematics;
      try {
        //----- phase space definition
        if (tree_.count(KIN_NAME))
          par_kinematics += bc::unpack(tree_.get_child(KIN_NAME));
        if (tree_.count(INTEGR_NAME))
          rt_params_->integrator() += bc::unpack(tree_.get_child(INTEGR_NAME));
        if (tree_.count(GENERATOR_NAME))
          rt_params_->generation().setParameters(bc::unpack(tree_.get_child(GENERATOR_NAME)));
        if (tree_.count(EVT_MOD_SEQ_NAME)) {
          evt_mod_ = bc::unpack(tree_.get_child(EVT_MOD_SEQ_NAME));
          for (const auto& name : evt_mod_.keys()) {
            const auto& mod = evt_mod_.get<ParametersList>(name);
            if (!mod.empty())
              rt_params_->addModifier(EventModifierFactory::get().build(name, mod));
          }
        }
        if (tree_.count(OUTPUT_NAME)) {
          evt_out_ = bc::unpack(tree_.get_child(OUTPUT_NAME));
          for (const auto& name : evt_out_.keys()) {
            const auto& mod = evt_out_.get<ParametersList>(name);
            if (!mod.empty())
              rt_params_->addEventExporter(EventExporterFactory::get().build(name, mod));
          }
        }
        if (tree_.count(TIMER_NAME))
          rt_params_->setTimeKeeper(new utils::TimeKeeper);
        if (tree_.count(LOGGER_NAME)) {
          log_ = bc::unpack(tree_.get_child(LOGGER_NAME));
          utils::Logger::get().setLevel(
              log_.getAs<int, utils::Logger::Level>("level", utils::Logger::Level::information));
          utils::Logger::get().setExtended(log_.get<bool>("extended", utils::Logger::get().extended()));
          for (const auto& mod : log_.get<std::vector<std::string> >("enabledModules"))
            utils::Logger::get().addExceptionRule(mod);
        }
        rt_params_->process().kinematics().setParameters(par_kinematics);
      } catch (const boost::exception&) {
      } catch (const Exception&) {
      }

      return rt_params_;
    }

    void BoostTreeHandler::pack(const RunParameters* params) {
      rt_params_ = const_cast<RunParameters*>(params);
      tree_.add_child(PROCESS_NAME, bc::pack(rt_params_->process().parameters()));
      if (!rt_params_->integrator().empty())
        tree_.add_child(INTEGR_NAME, bc::pack(rt_params_->integrator()));

      //----- kinematics block
      tree_.add_child(KIN_NAME, bc::pack(rt_params_->kinematics().parameters(false)));

      //----- generation block
      tree_.add_child(GENERATOR_NAME, bc::pack(rt_params_->generation().parameters()));

      //----- event modification and output
      if (!rt_params_->eventModifiersSequence().empty()) {
        auto evt_mod_tree = bc::pack(evt_mod_);
        for (const auto& mod : rt_params_->eventModifiersSequence())
          evt_mod_tree.put("", mod->name());
        tree_.add_child(EVT_MOD_SEQ_NAME, evt_mod_tree);
      }
      if (!rt_params_->eventExportersSequence().empty()) {
        auto out_mod_tree = bc::pack(evt_out_);
        for (const auto& mod : rt_params_->eventExportersSequence())
          out_mod_tree.add_child(mod->name(), bc::pack(mod->parameters()));
        tree_.add_child(OUTPUT_NAME, out_mod_tree);
      }

      //----- timing and logging
      if (rt_params_->timeKeeper())
        tree_.add_child(TIMER_NAME, bc::pack(ParametersList()));
      log_.set<int>("level", (int)utils::Logger::get().level());
      //TODO: implement the exceptions filtering rules
      //for (const auto& mod : utils::Logger::get().exceptionRules())
      //  log_.operator[]<std::vector<std::string> >("enabledModules").emplace_back(mod);
      tree_.add_child(LOGGER_NAME, bc::pack(log_));
    }

    ParametersDescription BoostTreeHandler::description() {
      auto desc = ParametersDescription();
      desc.setDescription("Boost tree parser/writer");
      return desc;
    }

    //------------------------------------------------------------------
    // class specialisations for each Boost format to be handled
    //------------------------------------------------------------------

    /// A JSON configuration file parser
    class JsonHandler final : public BoostTreeHandler {
      using BoostTreeHandler::BoostTreeHandler;
      void read(const std::string& filename) override { pt::read_json(filename, tree_); }
      void write(const std::string& filename) const override { pt::write_json(filename, tree_); }
    };

    /// An INFO configuration file parser
    class InfoHandler final : public BoostTreeHandler {
      using BoostTreeHandler::BoostTreeHandler;
      void read(const std::string& filename) override { pt::read_info(filename, tree_); }
      void write(const std::string& filename) const override { pt::write_info(filename, tree_); }
    };

    /// An XML configuration file parser
    class XmlHandler final : public BoostTreeHandler {
      using BoostTreeHandler::BoostTreeHandler;
      void read(const std::string& filename) override { pt::read_xml(filename, tree_); }
      void write(const std::string& filename) const override {
        std::ofstream file(filename);
        pt::write_xml(file, tree_);
      }
    };
  }  // namespace card
}  // namespace cepgen

typedef cepgen::card::JsonHandler JsonCardHandler;
typedef cepgen::card::InfoHandler InfoCardHandler;
typedef cepgen::card::XmlHandler XmlCardHandler;
REGISTER_CARD_HANDLER(".json", JsonCardHandler);
REGISTER_CARD_HANDLER(".info", InfoCardHandler);
REGISTER_CARD_HANDLER(".xml", XmlCardHandler);
