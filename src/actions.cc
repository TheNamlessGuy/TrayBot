#include "actions.h"

#include <stdlib.h>
#include <string.h>

#include "io.h"
#include "windows.h"

namespace Action {
  long amount;
  Action** list;

  void add(char* name, int type, char* value) {
    Action* action = (Action*) malloc(sizeof(Action));
    action->name = name;
    action->type = type;
    action->value = value;

    Action** newList = (Action**) malloc(sizeof(Action*) * amount + 1);
    memcpy(newList, list, amount * sizeof(Action*));
    newList[amount] = action;

    Action** oldList = list;
    list = newList;
    amount += 1;

    free(oldList);

    IO::write();
    Window::List::populate();
  }
}