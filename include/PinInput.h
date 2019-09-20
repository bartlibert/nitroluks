#ifndef NITROLUKSPP_PININPUT_H
#define NITROLUKSPP_PININPUT_H

#include <array>
#include <constants.h>
#include <termios.h>

namespace nitrolukspp {
class PinInput {
  public:
    PinInput();
    std::array<char, MAX_PIN_INPUT_LENGTH> get();

  private:
    termios saved_attributes{};
    void disable_echo();
    void reset_input_mode();
};
} /* namespace nitrolukspp */

#endif /* NITROLUKSPP_PININPUT_H */
