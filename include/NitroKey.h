#ifndef NITROLUKSPP_NITROKEY_H
#define NITROLUKSPP_NITROKEY_H

#include "constants.h"
#include <memory>
#include <string>

namespace nitrokey {
class NitrokeyManager;
} /* namespace nitrokey */
namespace nitrolukspp {
class NitroKey {
  public:
    explicit NitroKey(nitrokey::NitrokeyManager& manager);
    bool connect();
    std::string get_serial_number() const;
    unsigned int get_user_retry_count() const;
    void user_authenticate(std::array<char, MAX_PIN_INPUT_LENGTH> pin);
    bool is_authenticated() const;
    std::string get_password_from_slot(const std::string& slot_name, std::array<char, MAX_PIN_INPUT_LENGTH> pin);

  private:
    nitrokey::NitrokeyManager& key;
    bool authenticated{false};
};
} // namespace nitrolukspp

#endif /* NITROLUKSPP_NITROKEY_H */
