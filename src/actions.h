#pragma once

namespace Action {
  struct Action {
    char* name;
    int type;
    char* value;
  };

  extern long amount;
  extern Action** list;

  void add(char* name, int type, char* value);
}