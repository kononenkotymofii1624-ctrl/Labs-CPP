#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <limits>
#include <functional> // для std::hash
#include <windows.h>

enum class Role {
    USER,
    ADMIN
};

struct User {
    std::string login;
    std::string passwordHash; // Зберігаємо тільки хеш пароля
    Role role;
    std::string firstName;
    std::string lastName;
    int age;
    bool isStudent;
};

const std::string FILENAME = "users.csv";

// Функція хешування пароля з додаванням "солі"
std::string hashPassword(const std::string& password) {
    std::string salt = "S3cr3t_S@lt_2026";
    std::size_t hashed = std::hash<std::string>{}(password + salt);
    return std::to_string(hashed);
}

// Завантаження бази користувачів або створення дефолтного адміністратора
std::vector<User> loadUsers(const std::string& filename) {
    std::vector<User> users;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cout << "[Інфо] Файл '" << filename << "' не знайдено. Створюємо новий документ...\n";
        
        // Створюємо вбудованого адміністратора за замовчуванням (логін: admin, пароль: admin)
        User defaultAdmin{
            "admin",
            hashPassword("admin"),
            Role::ADMIN,
            "Головний",
            "Адміністратор",
            30,
            false
        };
        users.push_back(defaultAdmin);

        std::ofstream newFile(filename);
        if (newFile.is_open()) {
            newFile << "Login,PasswordHash,Role,FirstName,LastName,Age,IsStudent\n";
            newFile << defaultAdmin.login << ","
                    << defaultAdmin.passwordHash << ","
                    << "admin,"
                    << defaultAdmin.firstName << ","
                    << defaultAdmin.lastName << ","
                    << defaultAdmin.age << ","
                    << (defaultAdmin.isStudent ? "1" : "0") << "\n";
            newFile.close();
            std::cout << "[Інфо] Створено обліковий запис адміністратора за замовчуванням (логін: admin, пароль: admin).\n";
        } else {
            std::cerr << "[Помилка] Не вдалося створити файл!\n";
        }
        return users;
    }

    std::string line;
    std::getline(file, line); // Пропускаємо заголовок

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string item;
        User u;

        std::getline(ss, u.login, ',');
        std::getline(ss, u.passwordHash, ',');

        if (std::getline(ss, item, ',')) {
            u.role = (item == "admin") ? Role::ADMIN : Role::USER;
        }
        std::getline(ss, u.firstName, ',');
        std::getline(ss, u.lastName, ',');

        if (std::getline(ss, item, ',')) {
            u.age = std::stoi(item);
        }
        if (std::getline(ss, item, ',')) {
            u.isStudent = (item == "1");
        }

        users.push_back(u);
    }

    file.close();
    std::cout << "[Інфо] Завантажено облікових записів: " << users.size() << "\n";
    return users;
}

// Дозапис нового користувача у файл
void appendUserToFile(const std::string& filename, const User& u) {
    std::ofstream file(filename, std::ios::app);
    if (file.is_open()) {
        file << u.login << ","
             << u.passwordHash << ","
             << (u.role == Role::ADMIN ? "admin" : "user") << ","
             << u.firstName << ","
             << u.lastName << ","
             << u.age << ","
             << (u.isStudent ? "1" : "0") << "\n";
        file.close();
    } else {
        std::cerr << "[Помилка] Не вдалося відкрити файл для запису!\n";
    }
}

// Виведення анкети одного користувача
void printUser(const User& u) {
    std::cout << "Логін: " << u.login
              << " | Роль: " << (u.role == Role::ADMIN ? "Адміністратор" : "Користувач")
              << " | Ім'я: " << u.firstName
              << " | Прізвище: " << u.lastName
              << " | Вік: " << u.age
              << " | Студент: " << (u.isStudent ? "Так" : "Ні") << "\n";
}

// Перевірка, чи зайнятий логін
bool isLoginTaken(const std::vector<User>& users, const std::string& login) {
    for (const auto& u : users) {
        if (u.login == login) return true;
    }
    return false;
}

// Створення користувача (використовується і для самореєстрації, і адміністратором)
void createAccount(std::vector<User>& users, bool isAdminCreating) {
    User u;
    std::cout << "\n--- Створення облікового запису ---\n";
    
    while (true) {
        std::cout << "Введіть логін: ";
        std::cin >> u.login;
        if (isLoginTaken(users, u.login)) {
            std::cout << "[Помилка] Цей логін вже зайнятий. Спробуйте інший.\n";
        } else {
            break;
        }
    }

    std::string rawPassword;
    std::cout << "Введіть пароль: ";
    std::cin >> rawPassword;
    u.passwordHash = hashPassword(rawPassword);

    // Якщо створює адмін — він може обрати роль; при звичайній реєстрації роль завжди USER
    if (isAdminCreating) {
        int roleChoice;
        std::cout << "Оберіть роль (1 - Користувач, 2 - Адміністратор): ";
        while (!(std::cin >> roleChoice) || (roleChoice != 1 && roleChoice != 2)) {
            std::cout << "Введіть 1 або 2: ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        u.role = (roleChoice == 2) ? Role::ADMIN : Role::USER;
    } else {
        u.role = Role::USER;
    }

    std::cout << "Введіть ім'я: ";
    std::cin >> u.firstName;

    std::cout << "Введіть прізвище: ";
    std::cin >> u.lastName;

    std::cout << "Введіть вік: ";
    while (!(std::cin >> u.age) || u.age < 0) {
        std::cout << "Некоректний вік. Введіть додатне число: ";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    char statusInput;
    std::cout << "Ви студент? (1/y - так, 0/n - ні): ";
    std::cin >> statusInput;
    u.isStudent = (statusInput == '1' || statusInput == 'y' || statusInput == 'Y');

    users.push_back(u);
    appendUserToFile(FILENAME, u);

    std::cout << "\n[Успіх] Обліковий запис створено та збережено у файл!\n";
    printUser(u);
}

// Авторизація користувача
int loginUser(const std::vector<User>& users) {
    std::string login, rawPassword;
    std::cout << "\n--- Вхід у систему ---\n";
    std::cout << "Логін: ";
    std::cin >> login;
    std::cout << "Пароль: ";
    std::cin >> rawPassword;

    std::string enteredHash = hashPassword(rawPassword);

    for (size_t i = 0; i < users.size(); ++i) {
        if (users[i].login == login && users[i].passwordHash == enteredHash) {
            std::cout << "\n[Інфо] Вхід виконано успішно! Вітаємо, " << users[i].firstName << ".\n";
            return static_cast<int>(i);
        }
    }

    std::cout << "\n[Помилка] Невірний логін або пароль.\n";
    return -1;
}

// Панель звичайного користувача
void userMenu(const User& currentUser) {
    int choice = 0;
    do {
        std::cout << "\n=== КАБІНЕТ КОРИСТУВАЧА (" << currentUser.login << ") ===\n"
                  << "1. Переглянути мої дані\n"
                  << "0. Вийти з акаунту\n"
                  << "Ваш вибір: ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1;
        }

        switch (choice) {
            case 1:
                std::cout << "\n--- Ваша анкета ---\n";
                printUser(currentUser);
                break;
            case 0:
                std::cout << "Вихід з облікового запису...\n";
                break;
            default:
                std::cout << "Невідома команда.\n";
        }
    } while (choice != 0);
}

// Панель адміністратора
void adminMenu(std::vector<User>& users, const User& currentAdmin) {
    int choice = 0;
    do {
        std::cout << "\n=== ПАНЕЛЬ АДМІНІСТРАТОРА (" << currentAdmin.login << ") ===\n"
                  << "1. Переглянути всі анкети\n"
                  << "2. Фільтрувати анкети за ім'ям\n"
                  << "3. Переглянути тільки студентів\n"
                  << "4. Створити нового користувача\n"
                  << "0. Вийти з акаунту\n"
                  << "Ваш вибір: ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1;
        }

        switch (choice) {
            case 1:
                std::cout << "\n--- Усі зареєстровані користувачі ---\n";
                for (const auto& u : users) printUser(u);
                break;
            case 2: {
                std::string targetName;
                std::cout << "Введіть ім'я для пошуку: ";
                std::cin >> targetName;
                std::cout << "\n--- Результати пошуку ---\n";
                bool found = false;
                for (const auto& u : users) {
                    if (u.firstName == targetName) {
                        printUser(u);
                        found = true;
                    }
                }
                if (!found) std::cout << "Анкет з таким ім'ям не знайдено.\n";
                break;
            }
            case 3: {
                std::cout << "\n--- Анкети студентів ---\n";
                bool found = false;
                for (const auto& u : users) {
                    if (u.isStudent) {
                        printUser(u);
                        found = true;
                    }
                }
                if (!found) std::cout << "Студентів у базі немає.\n";
                break;
            }
            case 4:
                createAccount(users, true);
                break;
            case 0:
                std::cout << "Вихід з панелі адміністратора...\n";
                break;
            default:
                std::cout << "Невідома команда.\n";
        }
    } while (choice != 0);
}

int main() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    std::vector<User> users = loadUsers(FILENAME);

    int choice = 0;
    do {
        std::cout << "\n=== ГОЛОВНЕ МЕНЮ ===\n"
                  << "1. Увійти (Авторизація)\n"
                  << "2. Зареєструватися\n"
                  << "0. Вихід з програми\n"
                  << "Ваш вибір: ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1;
        }

        switch (choice) {
            case 1: {
                int userIndex = loginUser(users);
                if (userIndex != -1) {
                    if (users[userIndex].role == Role::ADMIN) {
                        adminMenu(users, users[userIndex]);
                    } else {
                        userMenu(users[userIndex]);
                    }
                }
                break;
            }
            case 2:
                createAccount(users, false);
                break;
            case 0:
                std::cout << "Завершення роботи.\n";
                break;
            default:
                std::cout << "Невідома команда.\n";
        }
    } while (choice != 0);

    return 0;
}