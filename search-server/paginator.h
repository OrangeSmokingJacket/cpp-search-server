#pragma once

#include <vector>

#include "document.h"

class Paginator
{
public:
    struct Page
    {
        std::vector<Document> content;
        int pageSize;
    };

    template <typename It>
    Paginator(It begin, It end, int pageSize)
    {
        Page page;
        for (It it = begin; it < end; it++)
        {
            if (page.content.size() < pageSize)
                page.content.push_back(*it);
            else
            {
                page.pageSize = static_cast<int>(page.content.size());
                pages.push_back(page);
                page.content.clear();
                page.pageSize = 0;
                page.content.push_back(*it);
            }
        }
        if (page.content.size() != 0)
        {
            page.pageSize = static_cast<int>(page.content.size());
            pages.push_back(page);
        }
    }
    auto begin() const
    {
        return pages.begin();
    }
    auto end() const
    {
        return pages.end();
    }
    int size()
    {
        int size = 0;
        for (int i = 0; i < pages.size(); i++)
        {
            for (int j = 0; j < pages[i].content.size(); j++)
            {
                size++;
            }
        }
        return size;
    }
private:
    std::vector<Page> pages;
};
template <typename Container>
Paginator Paginate(const Container& c, size_t page_size)
{
    return Paginator(begin(c), end(c), static_cast<int>(page_size));
}