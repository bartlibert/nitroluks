#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#define FMT_STRING_ALIAS 1
#include <fmt/format.h>
#include <libnitrokey/CommandFailedException.h>
#include <libnitrokey/NitrokeyManager.h>
#include <memory>
#include <termios.h>
#include <unistd.h>

constexpr auto ERROR = 1;
constexpr auto MAX_PIN_LENGTH = 20;

struct termios saved_attributes;

int error(char const* msg)
{
    fmt::print(stderr, fmt("{:s} \n*** Falling back to default LUKS password entry.\n"), msg);
    return ERROR;
}

void disable_echo()
{
    struct termios tattr {
    };
    tcgetattr(STDIN_FILENO, &saved_attributes);

    tcgetattr(STDIN_FILENO, &tattr);
    tattr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}

void reset_input_mode()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}

int main(int /* argc */, char const* /* argv */[])
{
    auto nk = nitrokey::NitrokeyManager::instance();

    nk->set_debug(false);
    auto connected = nk->connect();

    if (!connected) {
        return error("*** No nitrokey detected.\n");
    }

    fmt::print(stderr, fmt("*** Nitrokey : {:s} found!\n"), nk->get_serial_number());

    auto authenticated = false;
    std::array<char, MAX_PIN_LENGTH + 1> password{};
    do {
        // Stop if user pin is locked
        auto retry_count = nk->get_user_retry_count();
        if (retry_count == 0) {
            return error("*** User PIN locked.");
        }

        fmt::print(stderr, fmt("*** {:d} PIN retries left. Enter the (user) PIN. Empty to cancel\n"), retry_count);

        // Ask the password and unlock the nitrokey
        disable_echo();
        fgets(password.data(), password.size(), stdin);
        reset_input_mode();
        // remove the trailing newline
        std::replace(password.begin(), password.end(), '\n', '\0');
        if (std::all_of(password.begin(), password.end(), [](const decltype(password)::value_type& character) {
                return character == '\0';
            })) {
            return error("*** No PIN provided.");
        }

        try {
            nk->user_authenticate(password.data(), password.data());
        }
        catch (CommandFailedException& e) {
            if (e.reason_wrong_password()) {
                fmt::print(stderr, "*** Wrong PIN!\n");
                continue;
            }
            return error(fmt::format(fmt("*** Error in PIN entry: {:s}.\n"), e.what()).c_str());
        }
        authenticated = true;
        fmt::print(stderr, "*** PIN entry successful.\n");
    } while (!authenticated);

    //  Find a slot from the nitrokey where we fetch the LUKS key from.
    try {
        nk->enable_password_safe(password.data());
    }
    catch (CommandFailedException& e) {
        return error(fmt::format(fmt("*** Error while accessing password safe.\n"), e.what()).c_str());
    }

    fmt::print(stderr, "*** Scanning the nitrokey slots...\n");
    auto slots = nk->get_password_safe_slot_status();

    constexpr auto SLOT_ENABLED = 1;
    bool slots_enabled = std::any_of(
        slots.begin(), slots.end(), [](const decltype(slots)::value_type& slot) { return slot == SLOT_ENABLED; });
    if (!slots_enabled) {
        return error("*** No slots enabled.\n");
    }
    constexpr auto SLOT_NAME = "LUKS";
    auto luks_slot = std::find_if(slots.begin(), slots.end(), [nk](const decltype(slots)::value_type& slot) {
        return strncmp(nk->get_password_safe_slot_name(slot), SLOT_NAME, strlen(SLOT_NAME)) == 0;
    });
    if (luks_slot == slots.end()) {
        return error("*** No slot configured by name LUKS.\n");
    }

    // At this point we found a valid slot, go ahead and fetch the password.
    auto LUKS_password = nk->get_password_safe_slot_password(*luks_slot);

    // print password to stdout
    fmt::print(stdout, fmt("{:s}"), LUKS_password);

    return 0;
}
