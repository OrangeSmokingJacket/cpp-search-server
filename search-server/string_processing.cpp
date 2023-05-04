#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWords(string_view text)
{
    string_view str = text; // so we still will be able to pass const string_view here
    vector<string_view> result;
    if (str.empty())
        return result;

    size_t removal = str.find_first_not_of(" ");
    str.remove_prefix((removal == str.npos) ? str.size() : removal);

    while (!str.empty())
    {
        size_t space = str.find(' ');
        if (space == str.npos)
        {
            result.push_back(str.substr(0, str.size()));
            break;
        }
        else
        {
            result.push_back(str.substr(0, space));
            removal = str.find_first_not_of(" ", space);
            str.remove_prefix((removal == str.npos) ? str.size() : removal);
        }
    }

    return result;
} // parse text into vector of words