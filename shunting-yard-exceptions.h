
#ifndef _SHUNTING_YARD_EXCEPTIONS_H
#define _SHUNTING_YARD_EXCEPTIONS_H

#include <stdexcept>

class msg_exception : public std::exception {
protected:
  const std::string msg;
public:
  msg_exception(const std::string& msg) : msg(msg) {}
  ~msg_exception() throw() {}
  const char* what() const throw() {
    return msg.c_str();
  }
};

struct bad_cast : public msg_exception {
  bad_cast(const std::string& msg) : msg_exception(msg) {}
};

struct syntax_error : public msg_exception {
  syntax_error(const std::string& msg) : msg_exception(msg) {}
};

#endif
