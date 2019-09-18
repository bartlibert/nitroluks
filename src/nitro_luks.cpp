#define FMT_STRING_ALIAS 1
#include "NitroKey.h"
#include "exceptions.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <fmt/format.h>
#include <libnitrokey/NitrokeyManager.h>
#include <memory>
#include <termios.h>
#include <unistd.h>

constexpr auto ERROR = 1;

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
    tattr.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}

void reset_input_mode()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}

int main(int /* argc */, char const* /* argv */[])
{
    auto nitrokey_manager = nitrokey::NitrokeyManager::instance();
    auto nk = nitrolukspp::NitroKey(*nitrokey_manager);

    auto connected = nk.connect();

    if (!connected) {
        return error("*** No nitrokey detected.\n");
    }

    fmt::print(stderr, fmt("*** Nitrokey : {:s} found!\n"), nk.get_serial_number());

    std::array<char, nitrolukspp::MAX_PIN_LENGTH + 1> password{};
    do {
        // Stop if user pin is locked
        auto retry_count = nk.get_user_retry_count();
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
            nk.user_authenticate(password);
        }
        catch (nitrolukspp::WrongPinException& e) {
            fmt::print(stderr, e.what());
            continue;
        }
        catch (nitrolukspp::NitrokeyApiException& e) {
            return error(fmt::format(fmt("*** Error in PIN entry: {:s}.\n"), e.what()).c_str());
        }
        fmt::print(stderr, "*** PIN entry successful.\n");
    } while (!nk.is_authenticated());

    try {
        constexpr auto SLOT_NAME = "LUKS";
        fmt::print(stdout, fmt("{:s}"), nk.get_password_from_slot(SLOT_NAME, password));
        return 0;
    }
    catch (nitrolukspp::NitrokeyApiException& e) {
        return error(e.what());
    }
    catch (nitrolukspp::SlotException& e) {
        return error(e.what());
    }
}
