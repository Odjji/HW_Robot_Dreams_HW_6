#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <string>

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

// БЛОК: Клас, що описує поодиноку ціль на полі бою
class Target {
public:
    Target() : pos(0.0, 0.0), hp(0.0), value(0.0), type(UNKNOW) {}
    Target(Position_2D init_pos, double init_hp, TargetType init_type, double init_value)
         : pos(init_pos), hp(init_hp), type(init_type), value(init_value) {}
        
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

// БЛОК: Клас-менеджер місії (відповідає за зчитування, збереження та зберігання цілей)
class MissionScenario {
public:
    static constexpr int MAX_TARGET = 64;
        
    MissionScenario() : wind(0.0, 0.0), altitude_m(0.0), count(0) {}
    MissionScenario(Position_2D init_wind, double init_altitude_m, int init_count)
         : wind(init_wind), altitude_m(init_altitude_m), count(init_count) {}
        
    Position_2D getWind()       const { return wind; }
    double getAltitudeM()       const { return altitude_m; }
    int getCount()              const { return count; }
    Target getTarget(int index) const { return targets[index]; }
        
    // Зчитування вхідного текстового сценарію місії
    bool loadFromFile(const std::string& input_path) {
        std::ifstream my_file(input_path);

        if (!my_file.is_open()) {
            std::cout << "Помилка! Не вдалося відкрити файл! \n";
            return false; // 
        }
        
        my_file >> altitude_m;
        my_file >> wind.x >> wind.y; 
        my_file >> count;
            
        for (int i = 0; i < count; i++) {
            double tx, ty, thp;
            std::string temp_type;
        
            my_file >> tx >> ty >> thp >> temp_type;
            double tvalue = get_target_value(temp_type);
                
            // Створюємо та безпечно записуємо об'єкт у приватний масив класу
            targets[i] = Target(Position_2D(tx, ty), thp, UNKNOW, tvalue);
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
    Target targets[MAX_TARGET];
};

// БЛОК: Абстрактний інтерфейс (контракт) для розрахунку джерел траєкторій
class ITrajectorySource {
public:
    static constexpr double G_ACCEL = 9.81;
    

    virtual ~ITrajectorySource() {}
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


int main(int argc, char* argv[])
{
    std::string input_path = "";
    std::string output_path = "";
    std::string trajectory_type = "hover";
    
        for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--input-file" && i + 1 < argc) {
            input_path = argv[++i];
        } else if (arg == "--output-file" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg == "--trajectory" && i +1 < argc) {
            trajectory_type = argv[++i];
        }

    }
    
        if (input_path.empty() || output_path.empty()) {
        std::cerr << "Помилка: Не вказано обов'язкові аргументи!\n";
        
        return 1;
    } 
    
    MissionScenario mission;
    
    if(!mission.loadFromFile(input_path)) {
        
        return 1;
    }
    
    // Створюємо універсальний роз'єм для траєкторії
    ITrajectorySource* trajectorySim = nullptr;

    // Визначаємо, який саме картридж підключити
    if (trajectory_type == "forward") { 
        trajectorySim = new ForwardTrajectory(); // Створюємо модель ходу
    } else {
        trajectorySim = new HoverTrajectory();   // Інакше створюємо модель зависання
    }

     // Масив доступних боєприпасів із використанням нового класу Ammo
    Ammo all_ammo[3]{
        {"Frag", 60.0, 25.0, 35.0},
        {"HE",   35.0, 55.0, 20.0},
        {"HEAT", 15.0, 130.0, 45.0}
    };
    
    double max_net = -999999.0;    // Початкове дуже мале значення нетто
    int best_i = 0;                // Індекс найкращої цілі (точки влучання)
    int best_a = 0;                // Індекс найкращого боєприпасу
    int best_destroyed = 0;        // Скільки цілей знищено в найкращому варіанті
    double best_value_sum = 0.0;    // Сумарна цінність знищених цілей
    
    // 1. Фіксуємо точку прицілювання (ціль i)
    for (int i = 0; i < mission.getCount(); ++i) { 
        Position_2D impact_pos = mission.getTarget(i).getPos();
        
        // 2. Обираємо тип боєприпасу a
        for (int a = 0; a < 3; ++a) { 
            double curr_value_sum  = 0.0;
            int curr_destr_count   = 0;
            
            // 3. Симулюємо вибух: перевіряємо ВСІ цілі j на карті
            for (int j = 0; j < mission.getCount(); ++j) { 
                // Використовуємо метод distanceTo з нашої структури Position_2D
                double dist_to_targ = mission.getTarget(j).getPos().distanceTo(impact_pos);
                
                // Перевірка радіусу та шкоди через геттери класів Ammo та Target
                if (dist_to_targ <= all_ammo[a].getDefetRad() && all_ammo[a].getDamageAmmo() >= mission.getTarget(j).getHp()) {
                    curr_value_sum += mission.getTarget(j).getValue();
                    curr_destr_count += 1;
                }
            } 
            
            // Підбиття підсумків чисте нетто (цінність - ціна бомби)
            double current_net = curr_value_sum - all_ammo[a].getPriceAmmo();
        
            if (current_net > max_net) {
                max_net = current_net;
                best_i = i;
                best_a = a;
                best_destroyed = curr_destr_count;
                best_value_sum = curr_value_sum;
            } 
            else if (std::abs(current_net - max_net) < 1e-7) {
                if (all_ammo[a].getPriceAmmo() < all_ammo[best_a].getPriceAmmo()) {
                    best_i = i;
                    best_a = a;
                    best_destroyed = curr_destr_count;
                    best_value_sum = curr_value_sum;
                }
            }    
        } 
    } 

    // Отримуємо найкращу точку влучання
    Position_2D best_impact = mission.getTarget(best_i).getPos();

    //Розрахунок точки скиду через універсальний інтерфейс
    Position_2D release_pos = trajectorySim->calculateRelease(best_impact, mission.getAltitudeM(), mission.getWind());

    // Збереження результатів через метод нашого класу-сценарію
    mission.saveToFile(output_path, best_impact, release_pos, all_ammo[best_a].getNameAmmo(), 
                       best_destroyed, best_value_sum, all_ammo[best_a].getPriceAmmo(), max_net);

    
    delete trajectorySim;

    return 0;
}