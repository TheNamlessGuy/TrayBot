#include "io.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <io.h>

#include "actions.h"

void IO::read() {
  if (access(DATA_FILE, F_OK) != 0) { // No data file found - this is fine
    Action::list = {};
    Action::amount = 0;
    return;
  }

  FILE* fp = fopen(DATA_FILE, "rb");
  if (fp == nullptr) {
    IO::log("ERROR: Couldn't open data file in IO::read");
    exit(1);
  }

  // Read file into memory
  fseek(fp, 0, SEEK_END);
  long length = ftell(fp) + 1;
  fseek(fp, 0, SEEK_SET);
  char* buffer = (char*) malloc(length);
  if (buffer == nullptr) {
    fclose(fp);
    IO::log("ERROR: Couldn't allocate buffer in IO::read");
    exit(1);
  }
  fread(buffer, 1, length - 1, fp);
  fclose(fp);
  buffer[length - 1] = '\0';

  // Count lines - assumes LF, because CRLF is cringe
  long lines = 0;
  for (long i = 0; i < length; ++i) {
    if (buffer[i] == '\n') { lines += 1; }
  }

  // Parse each line
  Action::Action** actions = (Action::Action**) malloc(sizeof(Action::Action*) * lines);
  long start = 0;
  for (long line = 0; line < lines; ++line) {
    Action::Action* action = (Action::Action*) malloc(sizeof(Action::Action));
    actions[line] = action;

    long idx = start;
    for (long i = idx; /* Noop */; ++i) {
      if (buffer[i] == '|') {
        long nameLen = i - idx;
        action->name = (char*) malloc(nameLen + 1);
        strncpy(action->name, buffer + idx, nameLen);
        action->name[nameLen] = '\0';
        idx = i + 1;
        break;
      }
    }

    for (long i = idx; /* Noop */; ++i) {
      if (buffer[i] == '|') {
        long typeLen = i - idx;
        char* type = (char*) malloc(typeLen + 1);
        strncpy(type, buffer + idx, typeLen);
        type[typeLen] = '\0';
        action->type = atoi(type);
        idx = i + 1;
        break;
      }
    }

    for (long i = idx; /* Noop */; ++i) {
      if (buffer[i] == '\n' || i == length) {
        long valueLen = i - idx;
        action->value = (char*) malloc(valueLen + 1);
        strncpy(action->value, buffer + idx, valueLen);
        action->value[valueLen] = '\0';
        start = i + 1;
        break;
      }
    }
  }

  free(buffer);
  Action::list = actions;
  Action::amount = lines;
}

void IO::write() {
  FILE* fp = fopen(DATA_FILE, "wb");
  if (fp == nullptr) {
    IO::log("ERROR: Couldn't open data file in IO::write");
    exit(1);
  }

  for (long i = 0; i < Action::amount; ++i) {
    //                   <name>                   |   <type> |          <value>                  \n
    long length = strlen(Action::list[i]->name) + 1 + 1    + 1 + strlen(Action::list[i]->value) + 1;

    char line[length + 1]; // +1 for \0
    snprintf(line, length + 1, "%s|%d|%s\n", Action::list[i]->name, Action::list[i]->type, Action::list[i]->value);

    size_t written = fwrite(line, 1, length, fp);
    if (written != length) {
      fclose(fp);
      IO::log("ERROR: Couldn't write to file in IO::write");
      exit(1);
    }
  }

  fclose(fp);
}

void IO::log(const char* msg) {
  FILE* fp = fopen(LOG_FILE, "ab");
  if (fp == nullptr) {
    exit(1);
  }

  fwrite(msg, 1, strlen(msg), fp);
  fclose(fp);
}