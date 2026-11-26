#include <windows.h>
#include <cmath>
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>

// Ідентифікатори елементів інтерфейсу
#define ID_EDIT_FORMULA 101
#define ID_BTN_PLOT     102
#define ID_CHK_SIN      103
#define ID_CHK_COS      104
#define ID_CHK_POW      105
#define ID_CHK_SQRT     106

HWND hEditFormula, hChkSin, hChkCos, hChkPow, hChkSqrt;
std::string currentFormula = "sin(x) + 0.5 * cos(2 * x)";
std::string errorMessage = "";

// --- Математичний парсер виразів із залученням <cmath> ---
class MathParser {
    std::string expr;
    size_t pos;
    double xVal;

    void skipSpaces() {
        while (pos < expr.size() && std::isspace(static_cast<unsigned char>(expr[pos]))) {
            pos++;
        }
    }

    double parseExpression() {
        double val = parseTerm();
        while (true) {
            skipSpaces();
            if (pos < expr.size() && expr[pos] == '+') {
                pos++;
                val += parseTerm();
            } else if (pos < expr.size() && expr[pos] == '-') {
                pos++;
                val -= parseTerm();
            } else {
                break;
            }
        }
        return val;
    }

    double parseTerm() {
        double val = parsePower();
        while (true) {
            skipSpaces();
            if (pos < expr.size() && expr[pos] == '*') {
                pos++;
                val *= parsePower();
            } else if (pos < expr.size() && expr[pos] == '/') {
                pos++;
                double divisor = parsePower();
                if (divisor == 0.0) return NAN;
                val /= divisor;
            } else {
                break;
            }
        }
        return val;
    }

    double parsePower() {
        double base = parseFactor();
        skipSpaces();
        if (pos < expr.size() && expr[pos] == '^') {
            pos++;
            double exponent = parsePower();
            return std::pow(base, exponent); // std::pow з <cmath>
        }
        return base;
    }

    double parseFactor() {
        skipSpaces();
        if (pos >= expr.size()) throw std::runtime_error("Незакінчений вираз");

        if (expr[pos] == '+') {
            pos++;
            return parseFactor();
        }
        if (expr[pos] == '-') {
            pos++;
            return -parseFactor();
        }

        // Дужки
        if (expr[pos] == '(') {
            pos++;
            double val = parseExpression();
            skipSpaces();
            if (pos >= expr.size() || expr[pos] != ')') {
                throw std::runtime_error("Очікується закриваюча дужка ')'");
            }
            pos++;
            return val;
        }

        // Числа
        if (std::isdigit(static_cast<unsigned char>(expr[pos])) || expr[pos] == '.') {
            size_t start = pos;
            while (pos < expr.size() && (std::isdigit(static_cast<unsigned char>(expr[pos])) || expr[pos] == '.')) {
                pos++;
            }
            return std::stod(expr.substr(start, pos - start));
        }

        // Змінна x, константи або функції з <cmath>
        if (std::isalpha(static_cast<unsigned char>(expr[pos]))) {
            size_t start = pos;
            while (pos < expr.size() && std::isalpha(static_cast<unsigned char>(expr[pos]))) {
                pos++;
            }
            std::string name = expr.substr(start, pos - start);

            if (name == "x") return xVal;
            if (name == "pi") return 3.14159265358979323846;
            if (name == "e") return 2.71828182845904523536;

            skipSpaces();
            if (pos >= expr.size() || expr[pos] != '(') {
                throw std::runtime_error("Очікується '(' після функції " + name);
            }
            pos++; // пропускаємо '('

            // Обробка pow(a, b) з двома аргументами
            if (name == "pow") {
                double a = parseExpression();
                skipSpaces();
                if (pos >= expr.size() || expr[pos] != ',') {
                    throw std::runtime_error("Очікується ',' у pow(a, b)");
                }
                pos++; // пропускаємо ','
                double b = parseExpression();
                skipSpaces();
                if (pos >= expr.size() || expr[pos] != ')') {
                    throw std::runtime_error("Очікується ')' після pow(a, b)");
                }
                pos++;
                return std::pow(a, b);
            }

            // Функції одного аргументу
            double arg = parseExpression();
            skipSpaces();
            if (pos >= expr.size() || expr[pos] != ')') {
                throw std::runtime_error("Очікується ')' після аргументу " + name);
            }
            pos++;

            if (name == "sin")  return std::sin(arg);
            if (name == "cos")  return std::cos(arg);
            if (name == "sqrt") return (arg >= 0.0) ? std::sqrt(arg) : NAN;
            if (name == "tan")  return std::tan(arg);
            if (name == "abs")  return std::fabs(arg);
            if (name == "log")  return (arg > 0.0) ? std::log(arg) : NAN;
            if (name == "exp")  return std::exp(arg);

            throw std::runtime_error("Невідома функція: " + name);
        }

        throw std::runtime_error("Невідомий символ у формулі");
    }

public:
    double evaluate(const std::string& formula, double x) {
        expr = formula;
        pos = 0;
        xVal = x;
        double result = parseExpression();
        skipSpaces();
        if (pos < expr.size()) {
            throw std::runtime_error("Зайві символи в кінці виразу");
        }
        return result;
    }
};

// Функція малювання окремого графіка на полотні GDI
template <typename Func>
void DrawMathGraph(HDC hdc, RECT area, int centerX, int centerY, double scale, COLORREF color, int width, Func f) {
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);

    bool hasPrev = false;
    for (int px = area.left; px < area.right; ++px) {
        double x = (px - centerX) / scale;
        double y = f(x);

        if (std::isnan(y) || std::isinf(y) || std::fabs(y) > 100.0) {
            hasPrev = false;
            continue;
        }

        int py = static_cast<int>(centerY - y * scale);
        if (py < area.top || py > area.bottom) {
            hasPrev = false;
            continue;
        }

        if (!hasPrev) {
            MoveToEx(hdc, px, py, nullptr);
            hasPrev = true;
        } else {
            LineTo(hdc, px, py);
        }
    }

    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

// Головна функція обробки повідомлень вікна
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Підпис і текстове поле введення функції
            CreateWindowW(L"STATIC", L"f(x) =", WS_VISIBLE | WS_CHILD,
                          15, 18, 45, 22, hwnd, nullptr, nullptr, nullptr);

            hEditFormula = CreateWindowW(L"EDIT", L"sin(x) + 0.5 * cos(2 * x)",
                                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                         65, 15, 320, 26, hwnd, (HMENU)ID_EDIT_FORMULA, nullptr, nullptr);

            CreateWindowW(L"BUTTON", L"Побудувати графік", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                          400, 14, 150, 28, hwnd, (HMENU)ID_BTN_PLOT, nullptr, nullptr);

            // 4 чекбокси для перевірочних функцій з <cmath>
            hChkSin = CreateWindowW(L"BUTTON", L"std::sin(x) [Синій]",
                                    WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                                    15, 52, 160, 22, hwnd, (HMENU)ID_CHK_SIN, nullptr, nullptr);
            SendMessage(hChkSin, BM_SETCHECK, BST_CHECKED, 0); // Увімкнено за замовчуванням

            hChkCos = CreateWindowW(L"BUTTON", L"std::cos(x) [Зелений]",
                                    WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                                    185, 52, 175, 22, hwnd, (HMENU)ID_CHK_COS, nullptr, nullptr);

            hChkPow = CreateWindowW(L"BUTTON", L"std::pow(x, 2) [Фіолет.]",
                                    WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                                    370, 52, 185, 22, hwnd, (HMENU)ID_CHK_POW, nullptr, nullptr);

            hChkSqrt = CreateWindowW(L"BUTTON", L"std::sqrt(x) [Помаранч.]",
                                     WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                                     565, 52, 195, 22, hwnd, (HMENU)ID_CHK_SQRT, nullptr, nullptr);
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == ID_BTN_PLOT || id == ID_CHK_SIN || id == ID_CHK_COS || id == ID_CHK_POW || id == ID_CHK_SQRT) {
                // Зчитуємо текст із поля введення
                int len = GetWindowTextLengthA(hEditFormula);
                std::string buf(len + 1, '\0');
                GetWindowTextA(hEditFormula, &buf[0], len + 1);
                buf.resize(len);
                currentFormula = buf;

                // Оновлюємо область графіка
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            // Область під графік (нижче панелі елементів керування)
            RECT graphArea = { 15, 85, clientRect.right - 15, clientRect.bottom - 15 };

            // Білий фон полотна та рамка
            FillRect(hdc, &graphArea, (HBRUSH)(COLOR_WINDOW + 1));
            FrameRect(hdc, &graphArea, (HBRUSH)GetStockObject(BLACK_BRUSH));

            int centerX = (graphArea.left + graphArea.right) / 2;
            int centerY = (graphArea.top + graphArea.bottom) / 2;
            double scale = 40.0; // Масштаб: 40 пікселів = 1 одиниця

            // Координатна сітка
            HPEN gridPen = CreatePen(PS_DOT, 1, RGB(220, 220, 220));
            HPEN oldPen = (HPEN)SelectObject(hdc, gridPen);

            for (int x = centerX; x < graphArea.right; x += (int)scale) {
                MoveToEx(hdc, x, graphArea.top, nullptr);
                LineTo(hdc, x, graphArea.bottom);
            }
            for (int x = centerX; x > graphArea.left; x -= (int)scale) {
                MoveToEx(hdc, x, graphArea.top, nullptr);
                LineTo(hdc, x, graphArea.bottom);
            }
            for (int y = centerY; y < graphArea.bottom; y += (int)scale) {
                MoveToEx(hdc, graphArea.left, y, nullptr);
                LineTo(hdc, graphArea.right, y);
            }
            for (int y = centerY; y > graphArea.top; y -= (int)scale) {
                MoveToEx(hdc, graphArea.left, y, nullptr);
                LineTo(hdc, graphArea.right, y);
            }
            SelectObject(hdc, oldPen);
            DeleteObject(gridPen);

            // Осі координат X та Y
            HPEN axisPen = CreatePen(PS_SOLID, 2, RGB(50, 50, 50));
            oldPen = (HPEN)SelectObject(hdc, axisPen);
            MoveToEx(hdc, graphArea.left, centerY, nullptr);
            LineTo(hdc, graphArea.right, centerY);
            MoveToEx(hdc, centerX, graphArea.top, nullptr);
            LineTo(hdc, centerX, graphArea.bottom);
            SelectObject(hdc, oldPen);
            DeleteObject(axisPen);

            // 1. Наперед задані функції з чекбоксів (4 дії з пакета <cmath>)
            if (SendMessage(hChkSin, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                DrawMathGraph(hdc, graphArea, centerX, centerY, scale, RGB(0, 100, 255), 2,
                              [](double x) { return std::sin(x); });
            }
            if (SendMessage(hChkCos, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                DrawMathGraph(hdc, graphArea, centerX, centerY, scale, RGB(0, 170, 60), 2,
                              [](double x) { return std::cos(x); });
            }
            if (SendMessage(hChkPow, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                DrawMathGraph(hdc, graphArea, centerX, centerY, scale, RGB(150, 0, 200), 2,
                              [](double x) { return std::pow(x, 2); });
            }
            if (SendMessage(hChkSqrt, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                DrawMathGraph(hdc, graphArea, centerX, centerY, scale, RGB(240, 130, 0), 2,
                              [](double x) { return (x >= 0.0) ? std::sqrt(x) : NAN; });
            }

            // 2. Функція користувача з текстового поля (червона лінія)
            if (!currentFormula.empty()) {
                MathParser parser;
                try {
                    // Перевіряємо синтаксис формули у точці x = 1.0
                    parser.evaluate(currentFormula, 1.0);
                    DrawMathGraph(hdc, graphArea, centerX, centerY, scale, RGB(220, 20, 20), 3,
                                  [&](double x) { return parser.evaluate(currentFormula, x); });
                } catch (const std::exception& ex) {
                    SetTextColor(hdc, RGB(200, 0, 0));
                    TextOutA(hdc, graphArea.left + 10, graphArea.top + 10, ex.what(), (int)std::string(ex.what()).length());
                }
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_SIZE:
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"FunctionPlotterClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Побудова графіків функцій (C++ / cmath)",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 820, 620,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}