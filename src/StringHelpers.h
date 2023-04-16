#pragma once

#include <algorithm>
#include <string>

namespace dePhonica {
namespace String {

static std::string toLower(const std::string inputString)
{
    std::string resultString(inputString);

    for (size_t n = 0; n < inputString.size(); n++)
    {
        resultString[n] = std::tolower(resultString[n]);
    }

    return resultString;
}

} // namespace String
} // namespace dePhonica