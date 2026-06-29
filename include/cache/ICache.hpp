#include <optional>
#include <string>

class ICache{
    public:
        virtual ~ICache() = default;
        virtual std::optional<std::string> get(const std::string& key) = 0;
        virtual void put(const std::string& key, std::string value, int ttl) = 0;
};