#include <iostream>
#include <include/types.h>

#include "rgw_json.h"

// for testing DELETE ME
#include <fstream>

using namespace std;
using namespace json_spirit;

static bool contains_mixed_elements(Array a)
{
  int array_size = a.size();
  if (array_size == 0)
    return false;

  int first_type = a[0].type();
  for (int i = 1; i < array_size; i++) {
    int test_type = a[i].type();
    if (test_type != first_type)
      return true;
  }

  return false;
}

static bool has_children(Value v)
{
  int value_type = v.type();

  if (value_type == obj_type ) {
    return true;
  } else if (value_type != array_type) {
    return false;
  }

  Array temp_array = v.get_array();
  int first_type;
  Value first_value;

  do {
    if (temp_array.size() == 0)
      return false;

    /*
     * if the array is heterogeneous then we want
     * to stop processing now
     */
    if (contains_mixed_elements(temp_array))
      return false;

    first_value = temp_array[0];
    first_type = first_value.type();

    if (first_type == obj_type)
      return true;
    else if (first_type == array_type)
      temp_array = first_value.get_array();

  } while (first_type == array_type);

  return false;
}

JSONObjIter::JSONObjIter()
{
}

JSONObjIter::~JSONObjIter()
{
}

void JSONObjIter::set(const JSONObjIter::map_iter_t &_cur, const JSONObjIter::map_iter_t &_last)
{
  cur = _cur;
  last = _last;
}

void JSONObjIter::operator++()
{
  if (cur != last)
    ++cur;
};

JSONObj *JSONObjIter::operator*()
{
  return cur->second;
};

// does not work, FIXME
ostream& operator<<(ostream& out, JSONObj& obj) {
   out << obj.name << ": " << obj.data_string;
   return out;
}

JSONObj::~JSONObj()
{
}


void JSONObj::add_child(string el, JSONObj *obj)
{
  cout << "add_child: " << name << " <- " << el << endl;
  children.insert(pair<string, JSONObj *>(el, obj));
}

bool JSONObj::get_attr(string name, string& attr)
{
  map<string, string>::iterator iter = attr_map.find(name);
  if (iter == attr_map.end())
    return false;
  attr = iter->second;
  return true;
}

JSONObjIter JSONObj::find(string name)
{
  JSONObjIter iter;
  map<string, JSONObj *>::iterator first;
  map<string, JSONObj *>::iterator last;
  first = children.find(name);
  last = children.upper_bound(name);
  iter.set(first, last);
  return iter;
}

JSONObjIter JSONObj::find_first()
{
  JSONObjIter iter;
  iter.set(children.begin(), children.end());
    cout << "count=" << children.size() << endl;
  for (map<string, JSONObj *>:: iterator i = children.begin(); i != children.end(); ++i) {
    cout << "child: " << i->first << endl;
  }
  return iter;
}

JSONObjIter JSONObj::find_first(string name)
{
  JSONObjIter iter;
  map<string, JSONObj *>::iterator first;
  first = children.find(name);
  iter.set(first, children.end());
  return iter;
}

/* accepts a JSON Array or JSON Object contained in
 * a JSON Spirit Value, v,  and creates a JSONObj for each
 * child contained in v
 */
void JSONObj::handle_value(Value v)
{
  if (v.type() == obj_type) {
    Object temp_obj = v.get_obj();
    for (Object::size_type i = 0; i < temp_obj.size(); i++) {
      Pair temp_pair = temp_obj[i];
      string temp_name = temp_pair.name_;
      Value temp_value = temp_pair.value_;
      JSONObj *child = new JSONObj;
      child->init(this, temp_value, temp_name);
      add_child(temp_name, child);
    }
  } else if (v.type() == array_type) {
    Array temp_array = v.get_array();
    Value value;

    for (int j = 0; j < temp_array.size(); j++) {
      Value cur = temp_array[j];
      
      string temp_name;

      if (cur.type() == obj_type) {
	handle_value(cur);
      } else {
        JSONObj *child = new JSONObj;
        child->init(this, cur, temp_name);
        add_child(child->get_name(), child);
      }
    }
  }
}

void JSONObj::init(JSONObj *p, Value v, string n)
{
  name = n;
  parent = p;
  data = v;

  handle_value(v);
  data_string =  write(v, raw_utf8);
  attr_map.insert(pair<string,string>(name, data_string));
}

JSONObj *JSONObj::get_parent()
{
  return parent;
}

bool JSONObj::is_object()
{
  cout << data.type() << std::endl;
  return (data.type() == obj_type);
}

bool JSONObj::is_array()
{
  return (data.type() == array_type);
}

vector<string> JSONObj::get_array_elements()
{
  vector<string> elements;
  Array temp_array;

  if (data.type() == array_type)
    temp_array = data.get_array();

  int array_size = temp_array.size();
  if (array_size > 0)
    for (int i = 0; i < array_size; i++) {
      Value temp_value = temp_array[i];
      string temp_string;
      temp_string = write(temp_value, raw_utf8);
      elements.push_back(temp_string);
    }

  return elements;
}

RGWJSONParser::RGWJSONParser() : buf_len(0), success(true)
{
}

RGWJSONParser::~RGWJSONParser()
{
}



void RGWJSONParser::handle_data(const char *s, int len)
{
  json_buffer.append(s, len); // check for problems with null termination FIXME
  buf_len += len;
}

// parse a supplied JSON fragment
bool RGWJSONParser::parse(const char *buf_, int len)
{
  string json_string = buf_;
  // make a substring to len
  json_string = json_string.substr(0, len);
  success = read(json_string, data);
  if (success)
    handle_value(data);
  else
    set_failure();

  return success;
}

// parse the internal json_buffer up to len
bool RGWJSONParser::parse(int len)
{
  string json_string = json_buffer.substr(0, len);
  success = read(json_string, data);
  if (success)
    handle_value(data);
  else
    set_failure();

  return success;
}

// parse the complete internal json_buffer
bool RGWJSONParser::parse()
{
  success = read(json_buffer, data);
  if (success)
    handle_value(data);
  else
    set_failure();

  return success;
}

// parse a supplied ifstream, for testing. DELETE ME
bool RGWJSONParser::parse(const char *file_name)
{
  ifstream is(file_name);
  success = read(is, data);
  if (success)
    handle_value(data);
  else
    set_failure();

  return success;
}



