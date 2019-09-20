#include "NitroKey.h"
#include "exceptions.h"
#include <doctest.h>
#include <doctest/trompeloeil.hpp>
#include <libnitrokey/NitrokeyManager.h>

class MockKey {
  public:
    MAKE_MOCK1(set_debug, void(bool));
    MAKE_MOCK0(connect, bool());
    MAKE_MOCK0(get_serial_number, std::string());
    MAKE_MOCK0(get_user_retry_count, uint8_t());
    MAKE_MOCK2(user_authenticate, void(const char*, const char*));
    MAKE_MOCK1(get_password_safe_slot_name, char*(uint8_t));
    MAKE_MOCK1(enable_password_safe, void(const char*));
    MAKE_MOCK0(get_password_safe_slot_status, std::vector<uint8_t>());
    MAKE_MOCK1(get_password_safe_slot_password, char*(uint8_t));
};
MockKey mockKey{};

namespace nitrokey {
NitrokeyManager::NitrokeyManager() = default;
NitrokeyManager::~NitrokeyManager() = default;
void NitrokeyManager::set_debug(bool /* enable */) {}
bool NitrokeyManager::connect()
{
    return mockKey.connect();
}
std::string NitrokeyManager::get_serial_number()
{
    return mockKey.get_serial_number();
}
uint8_t NitrokeyManager::get_user_retry_count()
{
    return mockKey.get_user_retry_count();
}
void NitrokeyManager::user_authenticate(const char* user_password, const char* temporary_password)
{
    mockKey.user_authenticate(user_password, temporary_password);
}
char* NitrokeyManager::get_password_safe_slot_name(uint8_t slot_number)
{
    return mockKey.get_password_safe_slot_name(slot_number);
}
void NitrokeyManager::enable_password_safe(const char* user_pin)
{
    mockKey.enable_password_safe(user_pin);
}
std::vector<uint8_t> NitrokeyManager::get_password_safe_slot_status()
{
    return mockKey.get_password_safe_slot_status();
}
char* NitrokeyManager::get_password_safe_slot_password(uint8_t slot_number)
{
    return mockKey.get_password_safe_slot_password(slot_number);
}
} // namespace nitrokey

std::ostream& operator<<(std::ostream& os, const std::array<char, nitrolukspp::MAX_PIN_INPUT_LENGTH>& /* pin */)
{
    os << "foo";
    return os;
}

TEST_CASE("NitroKey")
{
    nitrokey::NitrokeyManager key;
    nitrolukspp::NitroKey nk(key);
    SUBCASE("connect fails if no key is available")
    {
        REQUIRE_CALL(mockKey, connect()).RETURN(false);
        CHECK(!nk.connect());
    }

    SUBCASE("connect succeeds if a key is available")
    {
        REQUIRE_CALL(mockKey, connect()).RETURN(true);
        CHECK(nk.connect());
    }

    SUBCASE("get_serial_number returns the value received from the key")
    {
        trompeloeil::sequence seq;
        REQUIRE_CALL(mockKey, get_serial_number()).RETURN("123456789").IN_SEQUENCE(seq);
        REQUIRE_CALL(mockKey, get_serial_number()).RETURN("42").IN_SEQUENCE(seq);
        REQUIRE_CALL(mockKey, get_serial_number()).RETURN("271828").IN_SEQUENCE(seq);
        REQUIRE_CALL(mockKey, get_serial_number()).RETURN("6:04:10:23").IN_SEQUENCE(seq);

        CHECK(nk.get_serial_number() == "123456789");
        CHECK(nk.get_serial_number() == "42");
        CHECK(nk.get_serial_number() == "271828");
        CHECK(nk.get_serial_number() == "6:04:10:23");
    }

    SUBCASE("get_user_retry_count returns the value retreived from the key")
    {
        trompeloeil::sequence seq;
        REQUIRE_CALL(mockKey, get_user_retry_count()).RETURN(0).IN_SEQUENCE(seq);
        REQUIRE_CALL(mockKey, get_user_retry_count()).RETURN(1).IN_SEQUENCE(seq);
        REQUIRE_CALL(mockKey, get_user_retry_count()).RETURN(2).IN_SEQUENCE(seq);
        REQUIRE_CALL(mockKey, get_user_retry_count()).RETURN(3).IN_SEQUENCE(seq);
        CHECK(nk.get_user_retry_count() == 0);
        CHECK(nk.get_user_retry_count() == 1);
        CHECK(nk.get_user_retry_count() == 2);
        CHECK(nk.get_user_retry_count() == 3);
    }

    SUBCASE("if the password is wrong, user_authenticate throws a WrongPinException")
    {
        REQUIRE_CALL(mockKey,
                     user_authenticate(trompeloeil::eq(std::string{"0000"}), trompeloeil::eq(std::string{"0000"})))
            .THROW(CommandFailedException{
                0, static_cast<uint8_t>(nitrokey::proto::stick10::command_status::wrong_password)});
        std::array<char, nitrolukspp::MAX_PIN_INPUT_LENGTH> pin{};
        std::fill_n(pin.begin(), 4, '0');
        CHECK_THROWS_AS(nk.user_authenticate(pin), nitrolukspp::WrongPinException);
    }

    std::string pinString = "1984";
    std::array<char, nitrolukspp::MAX_PIN_INPUT_LENGTH> pin{};
    std::copy(pinString.begin(), pinString.end(), std::begin(pin));

    SUBCASE("if an error occurs during PIN verification, user_authenticate throws a NitrokeyApiException")
    {
        REQUIRE_CALL(mockKey, user_authenticate(trompeloeil::eq(pinString), trompeloeil::eq(pinString)))
            .THROW(CommandFailedException{
                0, static_cast<uint8_t>(nitrokey::proto::stick10::command_status::AES_dec_failed)});
        CHECK_THROWS_AS(nk.user_authenticate(pin), nitrolukspp::NitrokeyApiException);
    }

    SUBCASE("if authentication has not happened or was unsuccessfull, is_authenticated returns false")
    {
        CHECK(!nk.is_authenticated());
        REQUIRE_CALL(mockKey, user_authenticate(trompeloeil::eq(pinString), trompeloeil::eq(pinString)))
            .THROW(CommandFailedException{
                0, static_cast<uint8_t>(nitrokey::proto::stick10::command_status::AES_dec_failed)});
        CHECK_THROWS_AS(nk.user_authenticate(pin), nitrolukspp::NitrokeyApiException);
        CHECK(!nk.is_authenticated());
    }

    SUBCASE("if authentication succeeds, is_authenticated returns true")
    {
        CHECK(!nk.is_authenticated());
        REQUIRE_CALL(mockKey, user_authenticate(trompeloeil::eq(pinString), trompeloeil::eq(pinString)));
        nk.user_authenticate(pin);
        CHECK(nk.is_authenticated());
    }

    constexpr auto SLOT_NAME = "LUKS";

    SUBCASE("if enabling password safe fails, a NitrokeyApiException is thrown")
    {
        REQUIRE_CALL(mockKey, enable_password_safe(trompeloeil::eq(pinString)))
            .THROW(CommandFailedException{
                0, static_cast<uint8_t>(nitrokey::proto::stick10::command_status::not_authorized)});
        CHECK_THROWS_AS(nk.get_password_from_slot(SLOT_NAME, pin), nitrolukspp::NitrokeyApiException);
    }

    SUBCASE("if getting password safe slot status fails, a NitrokeyApiException is thrown")
    {
        REQUIRE_CALL(mockKey, enable_password_safe(trompeloeil::eq(pinString)));
        REQUIRE_CALL(mockKey, get_password_safe_slot_status())
            .THROW(CommandFailedException{
                0, static_cast<uint8_t>(nitrokey::proto::stick10::command_status::slot_not_programmed)});
        CHECK_THROWS_AS(nk.get_password_from_slot(SLOT_NAME, pin), nitrolukspp::NitrokeyApiException);
    }

    SUBCASE("if no slots are enabled, a NoEnabledSlotsException is thrown")
    {
        REQUIRE_CALL(mockKey, enable_password_safe(trompeloeil::eq(pinString)));
        REQUIRE_CALL(mockKey, get_password_safe_slot_status()).RETURN(std::vector<uint8_t>{0, 0, 0, 0});
        CHECK_THROWS_AS(nk.get_password_from_slot(SLOT_NAME, pin), nitrolukspp::NoEnabledSlotsException);
    }

    SUBCASE("if getting password safe slot name fails wilt invalid slot, a NitrokeyApiException is thrown")
    {
        REQUIRE_CALL(mockKey, enable_password_safe(trompeloeil::eq(pinString)));
        REQUIRE_CALL(mockKey, get_password_safe_slot_status()).RETURN(std::vector<uint8_t>{0, 0, 1, 0});
        REQUIRE_CALL(mockKey, get_password_safe_slot_name(2)).THROW(InvalidSlotException{2});
        CHECK_THROWS_AS(nk.get_password_from_slot(SLOT_NAME, pin), nitrolukspp::NitrokeyApiException);
    }

    SUBCASE("if getting password safe slot name fails wilt command failure, a NitrokeyApiException is thrown")
    {
        REQUIRE_CALL(mockKey, enable_password_safe(trompeloeil::eq(pinString)));
        REQUIRE_CALL(mockKey, get_password_safe_slot_status()).RETURN(std::vector<uint8_t>{0, 0, 1, 0});
        REQUIRE_CALL(mockKey, get_password_safe_slot_name(2))
            .THROW(CommandFailedException{
                0, static_cast<uint8_t>(nitrokey::proto::stick10::command_status::not_authorized)});
        CHECK_THROWS_AS(nk.get_password_from_slot(SLOT_NAME, pin), nitrolukspp::NitrokeyApiException);
    }

    SUBCASE("if no active LUKS slot is configured, a NoNamedSlotException is thrown")
    {
        REQUIRE_CALL(mockKey, enable_password_safe(trompeloeil::eq(pinString)));
        REQUIRE_CALL(mockKey, get_password_safe_slot_status()).RETURN(std::vector<uint8_t>{0, 0, 1, 0});
        REQUIRE_CALL(mockKey, get_password_safe_slot_name(2)).RETURN(const_cast<char*>("LUX"));
        CHECK_THROWS_AS(nk.get_password_from_slot(SLOT_NAME, pin), nitrolukspp::NoNamedSlotException);
    }

    SUBCASE("if getting password safe slot password fails with command failure, a NitrokeyApiException is thrown")
    {
        REQUIRE_CALL(mockKey, enable_password_safe(trompeloeil::eq(pinString)));
        REQUIRE_CALL(mockKey, get_password_safe_slot_status()).RETURN(std::vector<uint8_t>{0, 0, 1, 0});
        REQUIRE_CALL(mockKey, get_password_safe_slot_name(2)).RETURN(const_cast<char*>(SLOT_NAME));
        REQUIRE_CALL(mockKey, get_password_safe_slot_password(2))
            .THROW(CommandFailedException{
                0, static_cast<uint8_t>(nitrokey::proto::stick10::command_status::no_name_error)});
        CHECK_THROWS_AS(nk.get_password_from_slot(SLOT_NAME, pin), nitrolukspp::NitrokeyApiException);
    }

    SUBCASE("if getting password safe slot password fails with invalid slot, a NitrokeyApiException is thrown")
    {
        REQUIRE_CALL(mockKey, enable_password_safe(trompeloeil::eq(pinString)));
        REQUIRE_CALL(mockKey, get_password_safe_slot_status()).RETURN(std::vector<uint8_t>{0, 0, 1, 0});
        REQUIRE_CALL(mockKey, get_password_safe_slot_name(2)).RETURN(const_cast<char*>(SLOT_NAME));
        REQUIRE_CALL(mockKey, get_password_safe_slot_password(2)).THROW(InvalidSlotException{2});
        CHECK_THROWS_AS(nk.get_password_from_slot(SLOT_NAME, pin), nitrolukspp::NitrokeyApiException);
    }

    SUBCASE("if everything is OK, stored password is returned")
    {
        REQUIRE_CALL(mockKey, enable_password_safe(trompeloeil::eq(pinString)));
        REQUIRE_CALL(mockKey, get_password_safe_slot_status()).RETURN(std::vector<uint8_t>{0, 0, 1, 0});
        REQUIRE_CALL(mockKey, get_password_safe_slot_name(2)).RETURN(const_cast<char*>(SLOT_NAME));
        REQUIRE_CALL(mockKey, get_password_safe_slot_password(2)).RETURN(const_cast<char*>("seekret"));
        CHECK(nk.get_password_from_slot(SLOT_NAME, pin) == "seekret");
    }
}
