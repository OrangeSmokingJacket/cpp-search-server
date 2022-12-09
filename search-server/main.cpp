// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь: 271

// Закомитьте изменения и отправьте их в свой репозиторий.

// Решение, если оно нужно:
// P.S. мне удобнее писать комментарии на английском, могу поменять в будущем
#include <iostream>

using namespace std;
int main()
{
	int counter = 0;
	for (int i = 1; i <= 1000; i++)
	{
		int rest = i;
		int digit;
		do
		{
			digit = rest - (rest / 10 * 10); // division abuse (round to the lowest 10s)
			rest /= 10; // remove last digit
			if (digit == 3)
			{
				counter++;
				break; // break the loop for additional speed
			}
		} while (rest > 0);
	}
	cout << counter << endl;
}