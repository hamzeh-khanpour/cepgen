#include <iostream>
#include <fstream>

#include "../include/mcgen.h"
/**
 * @author Laurent Forthomme <laurent.forthomme@uclouvain.be>
 */
int main(int argc, char* argv[]) {
  Parameters ip;
  double xsec, err, minpt, sqs;
  double min, max;
  std::ofstream tmp;
  int it;
  MCGen *mg;

  // max = 10.;
  //max = 10.;
  //min = 0.;
  //max = 14000.;
  min = 0.;
  max = 50.;

  if (argc>1) {
    it = atoi(argv[1]);
  }
  else {
    it = 100;
  }

  ip.in1p = 3500.;
  ip.in2p = 3500.;
  ip.process = new GamGamLL;
  ip.hadroniser = new Pythia6Hadroniser;
  ip.process_mode = Process::InelasticElastic;
  ip.pair = Particle::MUON;
  ip.remnant_mode = Process::SuriYennie;
  //ip.p2mod = 11;
  /*ip.maxtheta = 0;
    ip.maxtheta = 180;*/
  //ip.SetThetaRange(5., 175.);
  //ip.SetThetaRange(0., 180.);
  //ip.SetEtaRange(-999., 999.);
  ip.mineta = -5.;
  ip.maxeta = 5.;
  //ip.maxmx = 1000.;
  ip.mcut = 2;
  ip.minenergy = 0.;
  //ip.itmx = 5;
  //ip.ncvg = 10000;
  ip.minpt = 15.;
  ip.generation = false;
  ip.Dump();

  //tmp.open("tmp/xsec_lpairpp_elastic.dat");
  tmp.open("tmp/xsec_lpairpp_singleinelastic.dat");
  //tmp.open("tmp/xsec_lpairpp_doubleinelastic.dat");
  //tmp.open("tmp/xsec_sqs_lpairpp_elastic_noetacut.dat");
  //tmp.open("tmp.dat");
  //tmp.open("tmp/xsec_sqs_lpairpp_singleinelastic_noetacut.dat");
  //tmp.open("tmp/xsec_sqs_lpairpp_doubleinelastic_noetacut.dat");
  mg = new MCGen(&ip);
  for (int i=0; i<it; i++) {
    minpt = min+i/(double)it*(max-min);
    mg->parameters->minpt = minpt;
    //sqs = min+(double)i/(double)it*(max-min);
    //ip.in1p = sqs/2.;
    //ip.in2p = sqs/2.;
    mg->ComputeXsection(&xsec, &err);
    std::cout << minpt << "\t" << xsec << "\t" << err << std::endl;
    tmp << minpt << "\t" << xsec << "\t" << err << std::endl;
  }
  delete mg;
  return 0;
}

