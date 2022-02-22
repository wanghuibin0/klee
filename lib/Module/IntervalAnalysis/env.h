// date created:11/27/2015
// author: morgan

#ifndef ENV_H
#define ENV_H

#include "interval.h"
#include "rel.h"
#include "headers.h"

class Env {
private:
  map<Value *, Interval> env;
  bool valid; // when the condition of the branch doesn't hold, this is set to
              // false

public:
  Rels rels;

  Env() : valid(true) {}
  Env(map<Value *, Interval> env) : env(env), valid(true) {}

  bool hasValue(Value *v) { return env.find(v) != env.end(); }
  Interval lookup(Value *v) {
    if (!valid)
      return Interval(); // if the env is not valid and v cannot be found,
                         // return Bot
    // myassert(hasValue(v),"env lookup:");
    if (!hasValue(v)) {
      return Interval::top();
    }
    return env[v];
  }
  void set(Value *v, Interval i) {
    if (!valid)
      return; // if the env is not valid, do nothing
    env[v] = i;
  }

  void remove(Value *v) {
    if (hasValue(v)) {
      auto it = env.find(v);
      env.erase(it);
    }
  }
  void clear() { env.clear(); }

  bool isValid() { return valid; }
  void setValid(bool valid) { this->valid = valid; }

  typedef map<Value *, Interval>::iterator iterator;
  iterator begin() { return env.begin(); }
  iterator end() { return env.end(); }

  int size() { return env.size(); }
  void print() {
    for (iterator it = env.begin(); it != env.end(); it++) {
      outs() << (it->first) << "\t";
      cout << it->second << "\n";
    }
  }

  // join operator
  Env operator|(Env other) {
    if (!other.valid)
      myassert(other.size() == 0, "size is not 0 when env is invalid");
    if (!valid)
      myassert(size() == 0, "size is not 0 when env is invalid");
    bool vld = other.valid ||
               valid; // if valid is false in an env, the env has always size 0
    if (env.size() == 0) {
      other.setValid(vld);
      return other;
    } else if (other.env.size() == 0) {
      this->setValid(vld);
      return *this;
    } else {
      Env newEnv(other.env); // both are valid; nothing needs to be done
      /**/
      Rels newRels = rels | other.rels;
      newEnv.rels = newRels;
      /**/
      for (iterator it = begin(); it != end(); it++) {
        Value *v = it->first;
        Interval i = it->second;
        if (newEnv.hasValue(v)) {
          Interval newI = newEnv.lookup(v) | i;
          newEnv.set(v, newI);
        } else {
          newEnv.set(v, i);
        }
      }
      return newEnv;
    }
  }

  // windening operator
  Env operator||(Env other) { // seems this always on valid envs(since
                              // combine(prevs) on head should always be valid)
    Env newEnv(other.env);
    for (iterator it = begin(); it != end(); it++) {
      Value *v = it->first;
      Interval i = it->second;
      myassert(other.hasValue(v),
               "env.h:wideing between Env with different values");
      Interval newI = i || newEnv.lookup(v);
      newEnv.set(v, newI);
    }
    return newEnv;
  }

  //<= operator
  bool operator<=(Env other) {
    if (env.size() == 0) {
      return true;
    } else if (other.env.size() == 0) {
      return false;
    } else {
      bool ret = true;
      for (iterator it = begin(); it != end(); it++) {
        Value *v = it->first;
        Interval i = it->second;
        ret = ret && other.hasValue(v) && (i <= other.lookup(v));
      }
      return ret;
    }
  }

  //== operator
  bool operator==(Env other) {
    if (env.size() != other.env.size()) {
      return false;
    }
    bool ret = true;
    for (iterator it = begin(); it != end(); it++) {
      Value *v = it->first;
      Interval i = it->second;
      ret = ret && other.hasValue(v) && (i == other.lookup(v));
    }
    return ret;
  }
};
Env globalEnv; // for storing global variables
bool globaIsUsed = false;

#endif
