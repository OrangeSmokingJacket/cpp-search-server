# cpp-search-server

--ENG--

Search server.

The main task of search server is to store and later search text documents.
Inside each request there could be "plus" words (that actually will be searched for),
"minus" words (result has to have none of them),
"stop" words (have no effect of search algorithm, but necessary for human language).
Current version returns documents data (such as rating, relevance and other),
which is needed for ranking the results, and not documents themselves.

Language version: C++ 20.

TODO:
1) Currently all documents and requests are passed as one chunk, which is not the case in our reality.



--RU--

Поисковый сервер.

Задача поискового сервера - хранить и, впоследствии, вывести текстовые документы по запросу.
В запросе могут быть плюс-слова (которые нужно искать в документах),
минус-слова (искомый документ должен не иметь их в своем состава),
стоп-слова (не влияющие на содержания запроса, но нужные в человеческом языке).
Текущая версия возвращает данные документа (такие как рейтинг, релевантность и пр.),
нужные для ранжирования поиска, а не сами документы.

Версия языка: C++ 20.

Добавить:
1) Добавить раздельность запросов и добавление документов, текущая версия не совсем совпадает с реальностью.
