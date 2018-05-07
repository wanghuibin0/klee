// date created:12/2/2015
// author: morgan
// encapsulation of cfg

#ifndef MODULE_H
#define MODULE_H

#include "cfg.h"
#include "env.h"
#include "headers.h"

class NewModule {
private:
  vector<NewCfg *> ncfgs;

public:
  vector<NewCfg *> getCfgs() { return ncfgs; }
  int size() { return ncfgs.size(); }

  void addCfg(NewCfg *ncfg) { ncfgs.push_back(ncfg); }

  NewCfg *getCfg(string name) {
    for (auto &ncfgptr : ncfgs) {
      if (ncfgptr->getName() == name) {
        return ncfgptr;
      }
    }
    return NULL;
  }

  // static Env env;//holding global variables
  // static bool isUsed;
};
// Env NewModule::env;
// bool NewModule::isUsed = 0;

#endif
