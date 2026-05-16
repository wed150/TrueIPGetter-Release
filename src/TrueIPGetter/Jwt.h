#pragma once

#include "jwt-cpp/traits/nlohmann-json/defaults.h"

namespace TrueIPGetter {

/**
 * @brief Utility class for JWT token operations.
 * 
 * Provides methods for generating and verifying JWT tokens used in IP verification.
 */
class JwtUtils {
public:
    /**
     * @brief Generates a JWT token for the given UUID.
     * 
     * @param uuid The UUID to encode in the token.
     * @return std::string The generated JWT token.
     */
    static std::string generateToken(const std::string& uuid);

    /**
     * @brief Verifies the validity of a JWT token.
     * 
     * @param token The token to verify.
     * @return true if the token is valid, false otherwise.
     */
    static bool verifyToken(const std::string& token);

    /**
     * @brief Sets the secret key used for signing and verifying tokens.
     * 
     * @param secret The secret key.
     */
    static void setSecret(const std::string& secret);

    /**
     * @brief Gets the current secret key.
     * 
     * @return const std::string& The current secret key.
     */
    static const std::string& getSecret();

private:
    static std::string m_secret;
};

} // namespace TrueIPGetter
