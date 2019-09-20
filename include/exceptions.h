#ifndef NITROLUKSPP_EXCEPTIONS_H
#define NITROLUKSPP_EXCEPTIONS_H

#include <stdexcept>

namespace nitrolukspp {
class WrongPinException : public std::runtime_error {
  public:
    WrongPinException();
};

class NitrokeyApiException : public std::runtime_error {
  public:
    explicit NitrokeyApiException(const std::string& what);
};

class SlotException : public std::runtime_error {
  public:
    explicit SlotException(const std::string& what);
};

class NoEnabledSlotsException : public SlotException {
  public:
    NoEnabledSlotsException();
};

class NoNamedSlotException : public SlotException {
  public:
    explicit NoNamedSlotException(const std::string& name);
};

class EmptyPinException : public std::runtime_error {
  public:
    EmptyPinException();
};
} /* namespace nitrolukspp */

#endif /* NITROLUKSPP_EXCEPTIONS_H */
