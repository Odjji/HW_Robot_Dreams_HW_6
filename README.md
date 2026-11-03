Назва scenario3.txt з'явилася в нашому обговоренні виключно через те, що вона була жорстко прописана у твоєму початковому коді з ДЗ №3:
cpp
std::ifstream my_file("scenario3.txt");
Будьте обачні, використовуючи код.
Але в ДЗ №5 і тепер, у нашому новому ДЗ №6, ти зробив програму набагато гнучкішою. Завдяки твоєму циклу розбору аргументів у main (--input-file), файл може називатися абсолютно як завгодно — хоч input.txt, хоч mission.txt, хоч scenario3.txt. Програма зчитає той шлях, який ти сам передаси їй у терміналі під час запуску.
Тож для перевірки ти можеш створити файл із будь-яким ім'ям, головне — вказати це ж ім'я в команді запуску.

Фінальний штрих перед здачею (Діаграма класів для README.md)

В умові ДЗ №6 є обов'язкова вимога: оновити README.md та додати діаграму класів (наприклад, mermaid).
Оскільки ти вже біжиш здавати роботу, ось готовий код для цієї діаграми, який ідеально описує всю архітектуру, яку ти щойно побудував. Просто скопіюй цей блок і встав у свій файл README.md:
markdown
### Діаграма класів системи донаведення (ООП рефакторинг)

```mermaid
classDiagram
    class Position_2D {
        +double x
        +double y
        +distanceTo(other) double
        +operator-(other) Position_2D
        +operator+(other) Position_2D
        +operator*(scalar) Position_2D
    }

    class Target {
        -Position_2D pos
        -double hp
        -double value
        -TargetType type
        +getPos() Position_2D
        +getHp() double
        +getValue() double
        +getType() TargetType
    }

    class Ammo {
        -string name_ammo
        -double defet_rad
        -double damage_ammo
        -double price_ammo
        +getNameAmmo() string
        +getDefetRad() double
        +getDamageAmmo() double
        +getPriceAmmo() double
    }

    class MissionScenario {
        -Position_2D wind
        -double altitude_m
        -int count
        -Target targets[64]
        +getWind() Position_2D
        +getAltitudeM() double
        +getCount() int
        +getTarget(index) Target
        +loadFromFile(path) bool
        +saveToFile(path, ...) bool
    }

    class ITrajectorySource {
        <<interface>>
        +G_ACCEL double
        +calculateRelease(impact, altitude, wind)* Position_2D
    }

    class HoverTrajectory {
        +calculateRelease(impact, altitude, wind) Position_2D
    }

    class ForwardTrajectory {
        -Position_2D v_drone
        +calculateRelease(impact, altitude, wind) Position_2D
    }

    ITrajectorySource <|-- HoverTrajectory : реалізує
    ITrajectorySource <|-- ForwardTrajectory : реалізує
    MissionScenario *-- Target : зберігає масив
    Target *-- Position_2D : використовує
    MissionScenario *-- Position_2D : використовує
    ForwardTrajectory *-- Position_2D : використовує
```
