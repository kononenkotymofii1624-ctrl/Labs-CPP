#include <iostream>
#include <vector>
#include <random> // Сучасна стандартна бібліотека для генерації випадкових чисел
#include <limits>
#include <windows.h>

const int MAX_FAILED_ATTEMPTS = 15;

void printWord(const std::vector<char>& word) {
    std::cout << "{ ";
    for (size_t i = 0; i < word.size(); ++i) {
        std::cout << "'" << word[i] << "'";
        if (i + 1 < word.size()) {
            std::cout << ", ";
        }
    }
    std::cout << " }\n";
}

int main() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    const std::vector<std::vector<char>> WORDS = {
        { 'c', 'o', 'd', 'e' },
        { 'g', 'a', 'm', 'e' },
        { 'b', 'y', 't', 'e' },
        { 'l', 'o', 'o', 'p' },
        { 'v', 'o', 'i', 'd' },
        { 'c', 'h', 'a', 'r' },
        { 'a', 'r', 'r', 'a', 'y' },
        { 's', 't', 'a', 'c', 'k' },
        { 'q', 'u', 'e', 'u', 'e' },
        { 'l', 'o', 'g', 'i', 'c' },
        { 'p', 'i', 'x', 'e', 'l' },
        { 's', 'h', 'a', 'd', 'e', 'r' },
        { 'v', 'e', 'c', 't', 'o', 'r' },
        { 'm', 'a', 't', 'r', 'i', 'x' },
        { 's', 't', 'r', 'i', 'n', 'g' },
        { 'b', 'i', 'n', 'a', 'r', 'y' },
        { 'r', 'a', 'n', 'd', 'o', 'm' },
        { 'c', 'o', 'n', 's', 'o', 'l', 'e' },
        { 'p', 'o', 'i', 'n', 't', 'e', 'r' },
        { 'c', 'o', 'm', 'p', 'i', 'l', 'e', 'r' }
    };

    // 1. Отримуємо апаратне випадкове зерно (seed) від ОС
    std::random_device rd;
    // 2. Ініціалізуємо генератор (вихор Мерсенна) цим зерном
    std::mt19937 gen(rd());
    // 3. Задаємо рівномірний розподіл індексів від 0 до (WORDS.size() - 1)
    std::uniform_int_distribution<size_t> dist(0, WORDS.size() - 1);

    // Отримуємо випадковий індекс слова
    size_t randomIndex = dist(gen);
    const std::vector<char>& secretWord = WORDS[randomIndex];

    std::vector<char> guessedWord(secretWord.size(), '*');

    int failedAttempts = 0;
    int guessedCount = 0;
    int wordLength = static_cast<int>(secretWord.size());

    std::cout << "=== Гра «Вгадай Слово» ===\n";
    std::cout << "Позиції літер нумеруються з 0 до " << wordLength - 1 << ".\n";
    std::cout << "Максимум невдалих спроб: " << MAX_FAILED_ATTEMPTS << "\n\n";

    std::cout << "Загадане слово: ";
    printWord(guessedWord);

    while (guessedCount < wordLength && failedAttempts < MAX_FAILED_ATTEMPTS) {
        char letter;
        int position;

        std::cout << "\nВведіть літеру: ";
        std::cin >> letter;

        std::cout << "Введіть позицію (0.." << wordLength - 1 << "): ";
        if (!(std::cin >> position)) {
            std::cout << "[Помилка] Позиція має бути числом!\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (position < 0 || position >= wordLength) {
            std::cout << "[Помилка] Неправильна позиція! Спробуйте від 0 до " << wordLength - 1 << ".\n";
            continue;
        }

        if (secretWord[position] == letter) {
            if (guessedWord[position] == '*') {
                guessedWord[position] = letter;
                guessedCount++;
                std::cout << "-> Вірно! ";
                printWord(guessedWord);
            } else {
                std::cout << "-> Ця літера на позиції " << position << " вже відкрита!\n";
            }
        } else {
            failedAttempts++;
            std::cout << "-> Невірно! Залишилось спроб: " << (MAX_FAILED_ATTEMPTS - failedAttempts) << "\n";
        }
    }

    std::cout << "\n=== ГРУ ЗАВЕРШЕНО ===\n";
    if (guessedCount == wordLength) {
        std::cout << "Вітаємо! Ви вгадали слово: ";
        printWord(secretWord);
    } else {
        std::cout << "Вичерпано кількість невдалих спроб. Загадане слово було: ";
        printWord(secretWord);
    }

    return 0;
}

