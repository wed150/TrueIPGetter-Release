#include "Jwt.h"


namespace TrueIPGetter {

// Static member initialization
std::string JwtUtils::m_secret = "wed15";

std::string JwtUtils::generateToken(const std::string& uuid) {
    return jwt::create<jwt::traits::nlohmann_json>()
        .set_issuer("wed15")
        .set_subject(uuid)
        .set_audience("game_client")
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes{5})
        .sign(jwt::algorithm::hs256(m_secret));
}

bool JwtUtils::verifyToken(const std::string& token) {
    try {
        const auto decoded = jwt::decode(token);
        const auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256(m_secret)) // Only allow HS256 with our secret
            .with_issuer("wed15")
            .with_audience("game_client");

        verifier.verify(decoded);
    } catch (...) {
        return false;
    }
    return true;
}

void JwtUtils::setSecret(const std::string& secret) {
    m_secret = secret;
}

const std::string& JwtUtils::getSecret() {
    return m_secret;
}

} // namespace TrueIPGetter
