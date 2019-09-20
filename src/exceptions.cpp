#include "exceptions.h"

nitrolukspp::WrongPinException::WrongPinException() : std::runtime_error("*** Wrong PIN!") {}

nitrolukspp::NitrokeyApiException::NitrokeyApiException(const std::string& what) : std::runtime_error(what) {}

nitrolukspp::SlotException::SlotException(const std::string& what) : std::runtime_error(what) {}

nitrolukspp::NoEnabledSlotsException::NoEnabledSlotsException() : SlotException("*** No slots enabled.") {}

nitrolukspp::NoNamedSlotException::NoNamedSlotException(const std::string& name)
    : SlotException("*** No slot configured by name " + name)
{
}

nitrolukspp::EmptyPinException::EmptyPinException() : std::runtime_error("*** No PIN provided.") {}
