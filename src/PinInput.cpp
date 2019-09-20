#include "PinInput.h"
#include "exceptions.h"
#include <algorithm>
#include <unistd.h>

nitrolukspp::PinInput::PinInput() = default;

void nitrolukspp::PinInput::disable_echo()
{
    termios tattr{};
    tcgetattr(STDIN_FILENO, &saved_attributes);

    tcgetattr(STDIN_FILENO, &tattr);
    tattr.c_lflag &= ~static_cast<unsigned int>(ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}

void nitrolukspp::PinInput::reset_input_mode()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}

auto nitrolukspp::PinInput::get() -> std::array<char, MAX_PIN_INPUT_LENGTH>
{
    std::array<char, MAX_PIN_INPUT_LENGTH> buffer{};
    disable_echo();
    fgets(buffer.data(), buffer.size(), stdin);
    reset_input_mode();
    // remove the trailing newline
    std::replace(buffer.begin(), buffer.end(), '\n', '\0');
    if (std::all_of(buffer.begin(), buffer.end(), [](const decltype(buffer)::value_type& character) {
            return character == '\0';
        })) {
        throw EmptyPinException{};
    }
    return buffer;
}
