#ifndef _json_hpp_
#define _json_hpp_

#include <json/json.h>
#include <shared_ptr.hpp>


class Json {
public:
  typedef vz::shared_ptr<Json> Ptr;

  Json(struct json_object *jso) : _jso(jso) {};
  ~Json() { _jso=NULL;};
  
  struct json_object* Object() { return _jso; }
  
private:
  struct json_object* _jso;

};

  
#endif /* _json_hpp_ */
