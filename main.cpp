#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <chrono>

// Перелік типів цілей для класифікації об'єктів на карті
enum TargetType {
    UNKNOW = 0,
    PERSONNEL, 
    LOGISTICS, 
    APC, 
    TANK, 
    RADAR
};

// --- ДОПОМІЖНА ФУНКЦІЯ КОНВЕРТАЦІЇ 
double get_target_value(std::string text) {
    if (text == "PERSONNEL") return 10;
    if (text == "LOGISTICS") return 20;
    if (text == "APC")       return 40;
    if (text == "TANK")      return 60;
    if (text == "RADAR")     return 90;
    return 0.0; 
}

// БЛОК: Базова математична структура для збереження координат та дій над ними
struct Position_2D {
    double x = 0.0;
    double y = 0.0;
    
    Position_2D() : x(0.0), y(0.0) {}
    Position_2D(double init_x, double init_y) : x(init_x), y(init_y) {}
    
    // Обчислення прямої відстані між двома точками на площині
    double distanceTo(const Position_2D& other) const {
        double dx = this->x - other.x; 
        double dy = this->y - other.y; 
        return std::sqrt(dx * dx + dy * dy);
    }
    
    // Оператор віднімання для зсуву координат проти вектора зносу
    Position_2D operator-(const Position_2D& other) const {
        Position_2D result;
        result.x = this->x - other.x;
        result.y = this->y - other.y;
        return result;
    }
    
    // Оператор додавання для об'єднання кількох сил зносу (вітер + хід дрона)
    Position_2D operator+(const Position_2D& other) const {
        Position_2D result;
        result.x = this->x + other.x;
        result.y = this->y + other.y;
        return result;
    }
    
    // Оператор множення на скаляр (масштабування вектора швидкості на час падіння)
    Position_2D operator*(double scalar) const {
        Position_2D result;
        result.x = this->x * scalar;
        result.y = this->y * scalar;
        return result;
    }
};

// БЛОК: Структура для збереження потенційного результату (Кандидата) розрахунку системи донаведення
struct Sol_Candidate {
    std::string ammo_name;   // Назва обраного боєприпасу
    int destroyed;           // Кількість повністю знищених цілей
    double value_sum;        // Сумарна цінність знищених об'єктів
    double price_ammo;       // Ціна використаного боєприпасу
    double max_netto;        // Чисте тактичне нетто (цінність мінус витрати)
    Position_2D impact;      // Оптимальна точка прицілювання (влучання)
    Position_2D release;     // Розрахована точка скиду з урахуванням балістики
};

// БЛОК: Клас, що описує поодиноку ціль на полі бою
class Target {
public:
    Target() : pos(0.0, 0.0), hp(0.0), value(0.0), type(UNKNOW) {}
    
    // Конструктор класу: виправлено порядок ініціалізації полів відповідно до їх оголошення в private
    Target(Position_2D init_pos, double init_hp, TargetType init_type, double init_value)
         : pos(init_pos), hp(init_hp), value(init_value), type(init_type) {}
        
    Position_2D getPos() const { return pos; }
    double getHp()       const { return hp; }
    double getValue()    const { return value; }
    TargetType getType() const { return type; }

private:
    Position_2D pos;
    double hp;
    double value;
    TargetType type;
};

// БЛОК: Клас, що описує характеристики боєприпасу
class Ammo {
public:
    Ammo() : name_ammo(""), defet_rad(0.0), damage_ammo(0.0), price_ammo(0.0) {}
    Ammo(std::string init_name, double init_defet_rad, double init_damage_ammo, double init_price_ammo)
         : name_ammo(init_name), defet_rad(init_defet_rad), damage_ammo(init_damage_ammo), price_ammo(init_price_ammo) {}

    std::string getNameAmmo()    const { return name_ammo; }
    double getDefetRad()         const { return defet_rad; }
    double getDamageAmmo()       const { return damage_ammo; }
    double getPriceAmmo()        const { return price_ammo; }
    
private:
    std::string name_ammo;
    double defet_rad;
    double damage_ammo;
    double price_ammo;
};

// БЛОК: Клас-менеджер місії (відповідає за зчитування, збереження та динамічне збереження цілей на STL)
class MissionScenario {
public:
    static constexpr int MAX_TARGET = 64;
        
    MissionScenario() : wind(0.0, 0.0), altitude_m(0.0), count(0) {}
    MissionScenario(Position_2D init_wind, double init_altitude_m, int init_count)
         : wind(init_wind), altitude_m(init_altitude_m), count(init_count) {}
        
    Position_2D getWind()       const { return wind; }
    double getAltitudeM()       const { return altitude_m; }
    
    // Отримання кількості цілей через динамічний контейнер стандартної бібліотеки
    int getCount()  const { 
        return static_cast<int>(targets.size()); 
    }
    
    // Безпечне отримання цілі за її індексом із динамічного вектора
    Target getTarget(int index) const { 
        return targets[index]; 
    }
        
    // Зчитування вхідного текстового сценарію місії з підтримкою необмеженої кількості цілей
    bool loadFromFile(const std::string& input_path) {
        std::ifstream my_file(input_path);

        if (!my_file.is_open()) {
            std::cout << "Помилка! Не вдалося відкрити ... файл! \n";
            return false; 
        }
        
        my_file >> altitude_m;
        my_file >> wind.x >> wind.y; 
        my_file >> count;
        
        // Оптимізація: заздалегідь виділяємо пам'ять одним великим суцільним блоком під count елементів
        targets.reserve(count);
            
        for (int i = 0; i < count; i++) {
            double tx, ty, thp;
            std::string temp_type;
        
            my_file >> tx >> ty >> thp >> temp_type;
            double tvalue = get_target_value(temp_type);
                
            // Динамічно додаємо об'єкт у кінець вектора standard library без обмеження пам'яті
            targets.push_back(Target(Position_2D(tx, ty), thp, UNKNOW, tvalue));
        }
        my_file.close();
        return true;
    }
    
    // Запис фінального звіту про роботу системи донаведення
    bool saveToFile(const std::string& output_path, const Position_2D& impact, const Position_2D& release, const std::string& ammo_name, int destroyed, double value_sum, double price_ammo, double max_net) {
        std::ofstream out_file(output_path);

        if (!out_file.is_open()) {
            std::cout << "Помилка! Не вдалося відкрити файл результату! \n";
            return false;
        }
    
        out_file << std::fixed << std::setprecision(2);
        out_file << impact.x << " " << impact.y << "\n";
        out_file << release.x << " " << release.y << "\n";
        out_file << ammo_name << "\n";

        out_file << destroyed << " " 
                 << static_cast<int>(value_sum) << " " 
                 << static_cast<int>(price_ammo) << " " 
                 << static_cast<int>(max_net) << "\n";

        out_file.close();  
        return true; 
    }    

private:
    Position_2D wind;
    double altitude_m;
    int count;
    std::vector<Target> targets; // Масштабований динамічний контейнер замість статичного масиву
};

// БЛОК: Абстрактний інтерфейс (контракт) для розрахунку джерел траєкторій
class ITrajectorySource {
public:
    static constexpr double G_ACCEL = 9.81;
    
    // Віртуальний деструктор за замовчуванням для безпечного автоматичного очищення через розумні вказівники
    virtual ~ITrajectorySource() = default;
    virtual Position_2D calculateRelease(const Position_2D& impact, double altitude, const Position_2D& wind) const = 0;
};

// БЛОК: Конкретна реалізація траєкторії для режиму зависання (Hover)
// Використовується, коли дрон нерухомо висить над сектором цілей
class HoverTrajectory : public ITrajectorySource {
public:
    // Перевизначення базового методу розрахунку точки скиду боєприпасу
    Position_2D calculateRelease(const Position_2D& impact, double altitude, const Position_2D& wind) const override {
        // 1. Рахуємо точний час падіння боєприпасу з висоти польоту
        double t = std::sqrt((2.0 * altitude) / ITrajectorySource::G_ACCEL);
        
        // 2. Повертаємо точку скиду, зміщену тільки проти напрямку вітру
        return impact - wind * t;
    }
};
// БЛОК: Конкретна реалізація траєкторії для скиду на ходу (Forward)
// Використовується, коли дрон продовжує рух під час відділення боєприпасу
class ForwardTrajectory : public ITrajectorySource {
public:
    // Конструктор класу: фіксує постійну крейсерську швидкість дрона (12 м/с строго по осі X)
    ForwardTrajectory() : v_drone(12.0, 0.0) {}

    // Перевизначення базового методу розрахунку для динамічного польоту
    Position_2D calculateRelease(const Position_2D& impact, double altitude, const Position_2D& wind) const override {
        // 1. Обчислюємо час падіння боєприпасу (аналогічно до моделі Hover)
        double t = std::sqrt((2.0 * altitude) / ITrajectorySource::G_ACCEL);
        
        // 2. Повертаємо точку скиду, зміщену проти суми швидкостей вітру та самого дрона
        return impact - (wind + v_drone) * t;
    }

private:
    Position_2D v_drone; // Внутрішня константа власної горизонтальної швидкості БПЛА
};

// БЛОК: Шаблонна функція вищого порядку для високоточного хронометражу обчислень
template <typename Fn_cl>
long long measure_us(Fn_cl fn_cl) {
    const auto start = std::chrono::steady_clock::now(); // Фіксуємо мітку часу ДО початку
    fn_cl();                                             // Виконуємо переданий блок розрахунків
    const auto finish = std::chrono::steady_clock::now(); // Фіксуємо мітку часу ПІСЛЯ завершення
    // Повертаємо чисту тривалість, приведену до мікросекунд
    return std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
}

// ============================================================================
// ОСНОВНА ФУНКЦІЯ ПРОГРАМИ: MAIN
// ============================================================================
int main(int argc, char* argv[])
{
    std::string input_path = "";
    std::string output_path = "";
    std::string trajectory_type = "hover";
    
    // БЛОК: Парсинг аргументів командного рядка
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--input-file" && i + 1 < argc) {
            input_path = argv[++i];
        } else if (arg == "--output-file" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg == "--trajectory" && i + 1 < argc) {
            trajectory_type = argv[++i];
        }
    }
    
    // Перевірка наявності обов'язкових шляхів до файлів сценарію
    if (input_path.empty() || output_path.empty()) {
        std::cerr << "Помилка: Не вказано обов'язкові аргументи!\n";
        return 1;
    } 
    
    // Створюємо об'єкт менеджера місії
    MissionScenario mission;
    
    // Завантажуємо дані сценарію місії з файлу
    if (!mission.loadFromFile(input_path)) {
        return 1;
    }
    
    // Створюємо універсальний роз'єм для моделі траєкторії (розумний вказівник RAII)
    std::unique_ptr<ITrajectorySource> trajectorySim = nullptr;

    // Визначаємо, яку саме модель динаміки польоту підключити
    if (trajectory_type == "forward") { 
        trajectorySim = std::make_unique<ForwardTrajectory>(); // Створюємо модель ходу
    } else {
        trajectorySim = std::make_unique<HoverTrajectory>();   // Інакше створюємо модель зависання
    }

    // Динамічний вектор доступних боєприпасів із використанням класу Ammo (Modern C++)
    std::vector<Ammo> all_ammo {
        {"Frag", 60.0, 25.0, 35.0},
        {"HE",   35.0, 55.0, 20.0},
        {"HEAT", 15.0, 130.0, 45.0}
    };
    
    // Створюємо плоский контейнер для збереження абсолютно всіх комбінацій рішень
    std::vector<Sol_Candidate> candidates;
    
    // Оптимізація пам'яті: резервуємо точний розмір під майбутній масив варіантів (N * M)
    candidates.reserve(mission.getCount() * all_ammo.size());
    
    // --- ПОЧАТОК ЗОНИ ХРОНОМЕТРАЖУ ЧИСТИХ ОБЧИСЛЕНЬ ---
    long long elpased_us = measure_us([&]() {
    
        // 1. Фіксуємо точку прицілювання (ціль i)
        for (int i = 0; i < mission.getCount(); ++i) { 
            Position_2D impact_pos = mission.getTarget(i).getPos();
        
            // 2. Обираємо тип боєприпасу a
            for (size_t a = 0; a < all_ammo.size(); ++a) { 
                double curr_value_sum  = 0.0;
                int curr_destr_count   = 0;
            
                // 3. Симулюємо вибух: перевіряємо ВСІ цілі j на карті
                for (int j = 0; j < mission.getCount(); ++j) { 
                    double dist_to_targ = mission.getTarget(j).getPos().distanceTo(impact_pos);
                
                    // Перевірка радіусу ураження та пробиття броні/міцності цілі
                    if (dist_to_targ <= all_ammo[a].getDefetRad() && all_ammo[a].getDamageAmmo() >= mission.getTarget(j).getHp()) {
                        curr_value_sum += mission.getTarget(j).getValue();
                        curr_destr_count += 1;
                    }
                } 
            
                // Підбиття підсумків: чисте нетто комбінації (цінність мінус вартість бомби)
                double current_net = curr_value_sum - all_ammo[a].getPriceAmmo();
                
                // Розраховуємо точку скиду через поліморфний розумний вказівник
                Position_2D curr_realese = trajectorySim->calculateRelease(impact_pos, mission.getAltitudeM(), mission.getWind());
                
                // Сформуємо та запишемо поточного кандидата у загальну базу даних
                Sol_Candidate my_candidate;
                my_candidate.ammo_name    = all_ammo[a].getNameAmmo();
                my_candidate.price_ammo   = all_ammo[a].getPriceAmmo();
                my_candidate.destroyed    = curr_destr_count;
                my_candidate.value_sum    = curr_value_sum;
                my_candidate.max_netto    = current_net;
                my_candidate.impact       = impact_pos;
                my_candidate.release      = curr_realese;
                
                candidates.push_back(my_candidate);
            } 
        }
    });
    // --- КІНЕЦЬ ЗОНИ ХРОНОМЕТРАЖУ ЧИСТИХ ОБЧИСЛЕНЬ ---
    
    // БЛОК: Вибір найкращого рішення стандартним STL алгоритмом пошуку максимуму
    auto best_it = std::max_element(candidates.begin(), candidates.end(), [](const Sol_Candidate& a, const Sol_Candidate& b) {
        // Критерій 1: Шукаємо варіант із найбільшим чистим нетто
        if (a.max_netto != b.max_netto) {
            return a.max_netto < b.max_netto;
        } 
        // Тайбрейк 1: За рівного нетто обираємо дешевший боєприпас (дорожчий є "гіршим")
        if (a.price_ammo != b.price_ammo) {
            return a.price_ammo > b.price_ammo;
        }
        // Тайбрейк 2: Перемагає варіант, що зустрівся першим (гарантовано логікою std::max_element)
        return false;
    });  
    
    // Друкуємо тривалість розрахунку у мікросекундах суворо в stdout за форматом ТЗ
    std::cout << "elapsed_us " << elpased_us << "\n";

    // БЛОК: Запис фінальних результатів у файл
    if (best_it != candidates.end()) {
        // Викликаємо збереження, дістаючи всі дані напряму з найкращого кандидата через ітератор
        mission.saveToFile(
            output_path,
            best_it->impact,
            best_it->release,
            best_it->ammo_name,
            best_it->destroyed,
            best_it->value_sum,
            best_it->price_ammo,
            best_it->max_netto
        );
    }
    
    // Розумний вказівник усе видалить сам. Ручних delete немає!
    return 0;
}
