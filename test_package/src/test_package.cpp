#include <kl/base64.hpp>
#if defined(HAS_YAML)
#  include <kl/yaml.hpp>
#endif
#if defined(HAS_JSON)
#  include <kl/json.hpp>
#endif

#include <iostream>

int main()
{
    (void)kl::base64_decode("SGVsbG8gV29ybGQh");

#if defined(HAS_YAML)
    std::cout << "HAS_YAML: YES\n";
    (void)kl::yaml::serialize(123);
#else
    std::cout << "HAS_YAML: NO\n";
#endif

#if defined(HAS_JSON)
    std::cout << "HAS_JSON: YES\n";
    (void)kl::json::serialize(123);
#else
    std::cout << "HAS_JSON: NO\n";
#endif
}
