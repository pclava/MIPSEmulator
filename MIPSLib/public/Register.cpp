#include "utils.h"
#include "Register.h"

using namespace MIPS;

Register::Register(const unsigned int number, const s32 reset_value) :
    value(reset_value),
    reset_value(reset_value),
    number(number) {}

void Register::reset() {
    this->value = this->reset_value;
}

void Register::set(const s32 v) {
    this->value = v;
}

[[nodiscard]] s32 Register::read() const {
    return this->value;
}

void RegisterFile::reset() {
    for (Register &i : this->registers) {
        i.reset();
    }
}
void RegisterFile::write(const unsigned int index, const s32 v) {
    registers[index].set(v);
}
[[nodiscard]] s32 RegisterFile::read(const unsigned int index) const {
    return registers[index].value;
}
s32 RegisterFile::operator[](const unsigned int index) const {
    return registers[index].read();
}
s32& RegisterFile::operator[](const unsigned int index) {
    return registers[index].value;
}