#define FMT_STRING_ALIAS 1
#include "NitroKey.h"
#include "exceptions.h"
#include <fmt/format.h>
#include <libnitrokey/NitrokeyManager.h>
#include <string>

nitrolukspp::NitroKey::NitroKey(nitrokey::NitrokeyManager& manager) : key(manager)
{
    key.set_debug(false);
}

bool nitrolukspp::NitroKey::connect()
{
    return key.connect();
}

std::string nitrolukspp::NitroKey::get_serial_number() const
{
    return key.get_serial_number();
}

unsigned int nitrolukspp::NitroKey::get_user_retry_count() const
{
    return key.get_user_retry_count();
}

bool nitrolukspp::NitroKey::is_authenticated() const
{
    return authenticated;
}

void nitrolukspp::NitroKey::user_authenticate(std::array<char, MAX_PIN_INPUT_LENGTH> pin)
{
    try {
        key.user_authenticate(pin.data(), pin.data());
        authenticated = true;
    }
    catch (CommandFailedException& e) {
        authenticated = false;
        if (e.reason_wrong_password()) {
            throw WrongPinException{};
        }
        throw NitrokeyApiException{e.what()};
    }
}

std::string nitrolukspp::NitroKey::get_password_from_slot(const std::string& slot_name,
                                                          std::array<char, MAX_PIN_INPUT_LENGTH> pin)
{
    try {
        key.enable_password_safe(pin.data());
        fmt::print(stderr, "*** Scanning the nitrokey slots...\n");
        auto slots = key.get_password_safe_slot_status();
        constexpr auto SLOT_ENABLED = 1;

        bool slots_enabled = std::any_of(
            slots.begin(), slots.end(), [](const decltype(slots)::value_type& slot) { return slot == SLOT_ENABLED; });
        if (!slots_enabled) {
            throw NoEnabledSlotsException{};
        }

        constexpr auto NO_LUKS_SLOT = -1;
        auto luks_slot_index = NO_LUKS_SLOT;
        for (auto slot = slots.begin(); slot < slots.end(); ++slot) {
            if (*slot == SLOT_ENABLED && strncmp(key.get_password_safe_slot_name(std::distance(slots.begin(), slot)),
                                                 slot_name.c_str(),
                                                 slot_name.length()) == 0) {
                luks_slot_index = std::distance(slots.begin(), slot);
            }
        }

        if (luks_slot_index == NO_LUKS_SLOT) {
            throw NoNamedSlotException(slot_name);
        }

        return key.get_password_safe_slot_password(luks_slot_index);
    }
    catch (CommandFailedException& e) {
        throw NitrokeyApiException{fmt::format(fmt("*** Error while accessing password safe.\n"), e.what())};
    }
    catch (InvalidSlotException& e) {
        throw NitrokeyApiException{fmt::format(fmt("*** Invalid slot used.\n"), e.what())};
    }
}
