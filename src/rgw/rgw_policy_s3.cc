
#include <errno.h>

#include "rgw_policy_s3.h"


class RGWPolicyCondition {
protected:
  string v1;
  string v2;

  virtual bool check(const string& first, const string& second) = 0;

public:
  virtual ~RGWPolicyCondition() {}

  void set_vals(const string& _v1, const string& _v2) {
    v1 = _v1;
    v2 = _v2;
  }

  bool check(RGWPolicyEnv *env) {
     string first, second;
     env->get_value(v1, first);
     env->get_value(v2, second);
     return check(first, second);
  }

};


class RGWPolicyCondition_StrEqual : public RGWPolicyCondition {
protected:
  bool check(const string& first, const string& second) {
    return first.compare(second) == 0;
  }
};

class RGWPolicyCondition_StrStartsWith : public RGWPolicyCondition {
protected:
  bool check(const string& first, const string& second) {
    return first.compare(0, first.size(), second) == 0;
  }
};

void RGWPolicyEnv::add_var(const string& name, const string& value)
{
  vars[name] = value;
}

bool RGWPolicyEnv::get_var(const string& name, string& val)
{
  map<string, string>::iterator iter = vars.find(name);
  if (iter == vars.end())
    return false;

  val = iter->second;

  return true;
}

bool RGWPolicyEnv::get_value(const string& s, string& val)
{
  if (s.empty() || s[0] != '$') {
    val = s;
    return true;
  }

  return get_var(s.substr(1), val);
}


RGWPolicy::~RGWPolicy()
{
  list<RGWPolicyCondition *>::iterator citer;
  for (citer = conditions.begin(); citer != conditions.end(); ++citer) {
    RGWPolicyCondition *cond = *citer;
    delete cond;
  }
}

int RGWPolicy::add_condition(const string& op, const string& first, const string& second)
{
  RGWPolicyCondition *cond = NULL;
  if (op.compare("eq") == 0)
    cond = new RGWPolicyCondition_StrEqual;
  else if (op.compare("starts-with") == 0)
    cond = new RGWPolicyCondition_StrStartsWith;

  if (!cond)
    return -EINVAL;

  cond->set_vals(first, second);
  
  conditions.push_back(cond);

  return 0;
}

bool RGWPolicy::check(RGWPolicyEnv *env)
{
  list<pair<string, string> >::iterator viter;
  for (viter = var_checks.begin(); viter == var_checks.end(); ++viter) {
    pair<string, string>& p = *viter;
    const string& name = p.first;
    const string& check_val = p.second;
    string val;
    env->get_value(name, val); // ignore ret value
    if (val.compare(check_val) != 0)
      return false;
  }

  list<RGWPolicyCondition *>::iterator citer;
  for (citer = conditions.begin(); citer != conditions.end(); ++citer) {
    RGWPolicyCondition *cond = *citer;
    if (!cond->check(env))
      return false;
  }
  return true;
}
