#pragma once
#include <array>
#include <vector>
#include <unordered_map>
class VM;
#include "warrior.hpp"
#include "op.hpp"
#include "utility.hpp"
#include "read_helpers.hpp"

typedef void(*vm_op)(VM&, Warrior&);
typedef std::unordered_map<char, vm_op> opcode_map;

class VM {
public:
  VM(ulong num_warriors, opcode_map opcodes)
        /* NOTE: do NOT pass num_warriors to vector constructor,
         * as it'd allocate these elements
         * instead of setting the max size... (duh)
         */
      : _opcodes{std::move(opcodes)},
        _warriors{std::vector<Warrior>()},
        _num_warriors{num_warriors}
  { }

  void addWarrior(char* filename);
  void run();

  template<typename T>
  T
  readMemory(unsigned int offset) {
    return read_helpers::read<T>(_memory.begin() + offset);
  }


  std::vector<Warrior>& getWarriors();
  auto getAliveWarriors();

  long countAlive() const;
  opcode_map const& getOpcodes() const;
  bool isDone() const;
  uint getMaxCycles() const;

private:
  void checkDone();
  void runLifeCycle();

  opcode_map _opcodes;
  std::vector<Warrior> _warriors;
  ulong _num_warriors;
  uint _last_id = 0;

  std::array<char, MEM_SIZE> _memory;

  unsigned int _cycle = 0;
  unsigned int _delta = 0;
};

class VMInterruptException : std::exception {
};
