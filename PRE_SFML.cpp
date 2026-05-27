// ===== Includes / namespace =====

#include <SFML/Graphics.hpp>
#include <optional>
#include <random>
#include <vector>
#include <string>
#include <utility>
#include <iostream>
#include <sstream> // "stream text into a string"
#include <iomanip> // input/output manipulators.
#include <cmath>

using namespace sf;
using namespace std;

// ===== Core game data types =====
// TerrainType, ResourceType, MapView, MenuView, GameAction
// Tile, Grid

enum class TerrainType {
    Plain, 
    Water, 
};

enum class ResourceType {
    None,
    Oil
};

enum class MapView {
    Surface,
    Underground
};

enum class MenuView {
    Main,
    Production,
    Economy,
    Diplomacy,
    War,
    Bank,
    CivilianAffairs,
    Status
};

enum class GameAction {
    EndYearTurn,
    Quit
};

struct GameState {
    int year = 1;
    int day = 1;

    float dayTimer = 0.f;
    float normalSecondsPerDay = 5.f;
    float attackSecondsPerDay = 0.2f;

    bool duringAttack = false;
    int attackDaysLeft = 0;

    bool gameOver;
    bool victory;

    double targetPopulationPerTile = 5000.0;
};



void toggle_map_view(MapView& currentView) {
    if (currentView == MapView::Surface) {
        currentView = MapView::Underground;
    }
    else {
        currentView = MapView::Surface;
    }
}

struct Tile {                           // in a struct, members are public by default.
    TerrainType terrain = TerrainType::Plain;
    ResourceType resource = ResourceType::None;
    int ownerId = -1;
};

class Grid {                            //in a class, members are private by default. Classes are blueprints/types.

private:
    int width;                          //Member variable.
    int height;
    vector<Tile> tiles;                 //This is encapsulation == hide internal data, expose controlled functions.

public:
    Grid(int w, int h) : width(w), height(h), tiles(w* h) { // : != ::   Here, ':' does not mean "belong
        //to a class; instead ": starts the member initializer list". 
    }

    int getWidth() const {                      //All examples of OOP, because it's all centered around the Grid object (only the instanciation of Grid
        //will be an object. OOP; data and operations are grouped.
        return width;
    }

    int getHeight() const {                     //Member function.
        return height;
    }

    int index(int x, int y) const {
        return (y * width) + x;
    }

    bool isInside(int x, int y) const {
        return (x >= 0 && y >= 0) && (x < width && y < height); //no need for if or else!
    }

    Tile& get(int x, int y) {                   //overload
        return tiles[index(x, y)];
    }

    const Tile& get(int x, int y) const {       //overload
        return tiles[index(x, y)];
    }

    void resetTile(int x, int y) {
        get(x, y).terrain = TerrainType::Plain; //compiler knows to use (overloaded) Tile&, because context says "modify"?
        get(x, y).ownerId = -1;
    }
};

// ===== Economy model =====
// EconomyStart, Economy, EconomyLine, EconomyRow, Nation

struct EconomyStart {
    double population;
    double birthRate;
    double money;
    double oil;
    double industry;
    double food;
    double interestRate;
    double taxRate;
    double inflationRate;
    double unemploymentRate;
    double controlledArea;
    double economySize;
};

class Economy {  //if it's all public, it should be a struct...  "Whatcha gonna do about it?" -Mike Tyson
public:
    double population;
    double birthRate;
    double money;
    double oil;
    double industry;
    double food;
    double interestRate;
    double taxRate;
    double inflationRate;
    double unemploymentRate;
    double controlledArea;
    double economySize;

    Economy(const EconomyStart& start)
        : population(start.population),
        birthRate(start.birthRate),
        money(start.money),
        oil(start.oil),
        industry(start.industry),
        food(start.food),
        interestRate(start.interestRate),
        taxRate(start.taxRate),
        inflationRate(start.inflationRate),
        unemploymentRate(start.unemploymentRate),
        controlledArea(start.controlledArea),
        economySize(start.economySize)
    {
    }

    int getMoney() const {
        return money;
    }

    void spendMoney(int amount) {
        money -= amount;
    }    

    void growPopulation() {
        population += static_cast<int>(population * birthRate);
    }

    //void growControlledArea(){
    // controlledArea += static_Cast<int>(controlledArea * birthRate);
    // } 
};

void advance_days(GameState& gameState, Economy& economy, int days) {
    gameState.day += days;

    while (gameState.day > 365) {
        gameState.day -= 365;
        gameState.year++;
        economy.growPopulation();
    }
};

struct EconomyLine {
    string name;
    double* value;
    bool isPercent;
};

struct Nation {
    string name;
    Economy economy;
    pair<int, int> capital;
    int id;
};


// ===== UI types =====
// Button, Panel, MenuChoice, AnnualPlanRow

class Button {
private:
    RectangleShape shape;    //visible drawable rectangle.
    Text label;

public:
    Button(Vector2f position, Vector2f size, Color color,
        const Font& font, string text) : label(font)
    {
        shape.setPosition(position);
        shape.setSize(size);
        shape.setFillColor(color);

        label.setString(text);
        label.setCharacterSize(18);
        label.setFillColor(Color::White);

        FloatRect labelBounds = label.getLocalBounds(); // measurement rectangle/bounds data.
        label.setOrigin(Vector2f(
            labelBounds.position.x + labelBounds.size.x /2.f,
            labelBounds.position.y + labelBounds.size.y /2.f
        ));

        label.setPosition(Vector2f(
            position.x + size.x / 2.f,
            position.y + size.y / 2.f ));
    }

    bool isClicked(Vector2f mousePos) const {
        return shape.getGlobalBounds().contains(mousePos);
    }

    void draw(RenderWindow& window) const {
        window.draw(shape);
        window.draw(label);
    }
};

struct EconomyRow {
    int sectorIndex;
    Button minusButton;
    Button plusButton;
};

class Panel {
private:
    RectangleShape shape;
    Vector2f position;
    Vector2f size = Vector2f(320.f, 260.f);
    float padding = 20.f;
    float spacing = 40.f;
    int rowCount = 0;

public:
    Panel(Vector2f startPosition) :position(startPosition) {
        shape.setPosition(startPosition);
        shape.setSize(size);
        shape.setFillColor(Color(45, 45, 60));
    }

    Vector2f nextRowPosition(){
               Vector2f result(position.x + padding,
                               position.y + padding + rowCount * spacing);

               rowCount++;
               resizeToContent();

               return result;
    }

    void resizeToContent() {
        size.y = padding * 2.f + rowCount * spacing;
        shape.setSize(size);
    }

    void resizeForRows(int rows) {
        rowCount = rows;
        resizeToContent();
    }

    void draw(RenderWindow& window) const {
        window.draw(shape);
    }
};

struct MenuChoice {
    Button actualButton;
    MenuView opens;
};

Vector2f stackedPosition(Vector2f start,
                        int index,
                        float spacing) {
    return Vector2f(start.x,
                    start.y + index * spacing);
}

struct ProductionSector {
    string name;
    int percentage = 0;
};

struct ControlRow {
    int sectorIndex;
    Button minusButton;
    Button plusButton;
};

int totalProductionPercent(const vector<ProductionSector>& items) {
    int total = 0;

    for (const ProductionSector& item: items) {
        total += item.percentage;
    }
    return total;
}

struct GameEvent {
    string message;
};

struct EventBuffer {
    static constexpr int MAXEVENTS = 16;

    GameEvent events[MAXEVENTS];
    int eventhead = 0;
    int eventtail = 0;
};

void D_PostEvent(EventBuffer& buffer, GameEvent event) {
    buffer.events[buffer.eventhead] = event;
    buffer.eventhead = (buffer.eventhead + 1) % EventBuffer::MAXEVENTS;
}

// ===== Generation helpers =====
// random_coord, generate_lake, create_lakes, paint_water, paint_oil

vector<string> govt_names = { "Republic", "Dictatorship", "Negan", "Blackadder" };

int rand_int(int min, int max) {
    static mt19937 gen(random_device{}());
    uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

pair<int, int> random_coord(const Grid& world) {
    return {
        rand_int(0, world.getWidth() - 1),
        rand_int(0, world.getHeight() - 1) };
}

struct LakeData {
    pair<int, int> seed;
    int size;
};

LakeData generate_lake(const Grid& world) {
     LakeData lake;

     lake.size = rand_int(10, 30);

     int max_x = world.getWidth() - lake.size;
     int max_y = world.getHeight() - lake.size;

     lake.seed = {
         rand_int(0, max_x),
         rand_int(0, max_y)
     };

     return lake;
}

void paint_water(Tile& tile) {
    tile.terrain = TerrainType::Water;
}

void paint_oil(Tile& tile) {
    tile.resource = ResourceType::Oil;
}

void paint_lake(Grid& world, LakeData lake, void (*paint)(Tile&)) {
    int lx = lake.seed.first;
    int ly = lake.seed.second;

    for (int y = ly; y < ly + lake.size; y++) {
        for (int x = lx; x < lx + lake.size; x++) {
            paint(world.get(x, y));
        }
    }
}

void create_lake(Grid& world, LakeData lake) { //simplify parameters by putting lake_seed and size in a class
    int lx = lake.seed.first;
    int ly = lake.seed.second;

    for (int y = ly; y < ly + lake.size; y++) {
            for (int x = lx; x < lx + lake.size; x++) {
                world.get(x, y).terrain = TerrainType::Water;
            }
        }
}

void create_lakes(Grid& world, EventBuffer& eventBuffer, int lake_count, void(*paint)(Tile&), string message) {
    for (int i = 0; i < lake_count; i++) {
        LakeData lake = generate_lake(world);
   
        paint_lake(world, lake, paint);

        D_PostEvent(eventBuffer, { message });
    }
}

Color get_tile_color(const Tile& tile, MapView currentView) {
    if (currentView == MapView::Surface && tile.terrain == TerrainType::Water) {
        return Color::Blue;
    }
    else if (currentView == MapView::Surface) {
        return Color(15,90,30);
    }
    else if (currentView == MapView::Underground && tile.resource == ResourceType::Oil) {
        return Color::Black;
    }
    else {
        return Color(90, 70, 45);
    }
}

string formatDouble(double value) {
    ostringstream stream;
    stream << fixed << setprecision(2) << value;
    return stream.str();
}

int main()
{
    // ===== Main setup and game loop =====
    //SETUP

    int x = 0;
    int y = 0;
    const float speed = 700.f;

    EventBuffer eventBuffer;
    GameState gameState;

    //Setup: world
    Grid world(160, 90);

    int grid_width = world.getWidth();
    int grid_height = world.getHeight();

    pair<int, int> playerCapital = random_coord(world);
    world.get(playerCapital.first, playerCapital.second).ownerId = 0;
    D_PostEvent(eventBuffer, { "Capital founded" });

    Economy nationalEconomy({
        100000,
        0.02,
        100000,
        100000,
        1000,
        300000,
        0.03,
        0.10,
        0.02,
        0.04,
        1,
        100000
    });

    //vector<EconomySector> 

    LakeData lake = generate_lake(world);
    int lake_count = rand_int(3, 7);
    create_lakes(world, eventBuffer, lake_count, paint_water, "Lake generated");

    LakeData oilLake = generate_lake(world);
    int oilLake_count = rand_int(2, 5);
    create_lakes(world, eventBuffer, oilLake_count, paint_oil, "Oil lake generated");

    
    //Setup: camera
    VideoMode desktop = VideoMode::getDesktopMode();
    RenderWindow window(desktop, "SFML works!", State::Fullscreen);
    window.setFramerateLimit(60);
    View camera = window.getDefaultView();

    MapView currentView = MapView::Surface;
    MenuView currentMenu = MenuView::Main;

    Clock clock;

    float sq_tile_size = 32.f;
    float mapPixelWidth = world.getWidth() * sq_tile_size;

    float mapPixelHeight = world.getHeight() * sq_tile_size;

    camera.setSize(Vector2f(mapPixelWidth, mapPixelHeight));

    camera.setCenter(Vector2f(mapPixelWidth / 2.f, mapPixelHeight / 2.f
    ));

    RectangleShape sq_tile(Vector2f(sq_tile_size, sq_tile_size));
    sq_tile.setOutlineThickness(1.f);
    sq_tile.setOutlineColor(Color(30, 80, 30));   //RGB: red, green, blue values

    float capitalMarker_size = 8.f;
    CircleShape capitalMarker(capitalMarker_size);
    capitalMarker.setFillColor(Color::Red);
    capitalMarker.setPosition(Vector2f(playerCapital.first * sq_tile_size,
        playerCapital.second * sq_tile_size));

    //Setup:UI buttons
    Vector2f menuButtonStart(1620.f, 200.f);
    Vector2f buttonSize(140.f, 40.f);
    float buttonSpacing = 50.f;
    float textSpacing = 40.f;

    Vector2f panelListStart(1340.f, 340.f);

    Font uiFont;
    if (!uiFont.openFromFile("arial.ttf")) {
        return 1;
    }

    /*vector <AnnualPlanRow> productionControls = {
    {0,
    Button(
            Vector2f(1300.f, 340.f),
            Vector2f(28.f, 28.f),
            Color(90, 90, 90),
            uiFont,
            "-"),
    Button(
            Vector2f(1520.f, 340.f),
            Vector2f(28.f, 28.f),
            Color(90, 90, 90),
            uiFont,
            "+"
           )
    }
    };*/

    auto makeText = [&uiFont](string value, Vector2f position) {        //makeText is a variable holding a lambda. It is a callable object. not a normal function. 
        Text text(uiFont);
        text.setString(value);
        text.setCharacterSize(18);
        text.setFillColor(Color::White);
        text.setPosition(position);
        return text;
        };

    Vector2f mapButtonStart(1620.f, 80.f);

    Button surfaceButton(
        Vector2f(1620.f, 80.f),
        Vector2f(140.f, 40.f),
        Color(60, 90, 60),
        uiFont, "Surface"
    );

    Button undergroundButton(
        Vector2f(1620.f, 130.f),
        Vector2f(140.f, 40.f),
        Color(80, 70, 50),
        uiFont, "Oil"
    );

    Button mainMenuButton(
        Vector2f(1720.f, 760.f),
        Vector2f(140.f, 40.f),
        Color(90, 90, 90),
        uiFont, "Menu"
    );

    Button quitButton(Vector2f(1720.f, 810.f),
        Vector2f(140.f, 40.f),
        Color(90, 90, 90),
        uiFont, "Quit"
    );

    Button attackButton(Vector2f(1320.f, 250.f),
        Vector2f(140.f, 40.f),
        Color(90, 90, 90),
        uiFont, "Attack"
        );

    int attackLengthDays = 10;

    //Setup: production panel
    Panel prod_panel(Vector2f(1280.f, 200.f));
    Panel econ_panel(Vector2f(1280.f, 200.f));
    Panel war_panel(Vector2f(1280.f, 200.f));

    Text productionTitle = makeText("Production", prod_panel.nextRowPosition());
    Text economyTitle = makeText("Economy", econ_panel.nextRowPosition());
    Text warTitle = makeText("War", war_panel.nextRowPosition());
    Text howMany = makeText("How many days in a row?", war_panel.nextRowPosition());

    vector<string> warMenu{
        "Attack",
        "Move Tanks",
        "Buy Turrets",
        "Show current map",
        "Pause/Resume: press P to stop during attacks."
    };

    Vector2f attackInputBoxPosition = stackedPosition(panelListStart, 1, 28.f);

    RectangleShape attackInputBox(Vector2f(90.f, 32.f));
    attackInputBox.setPosition(attackInputBoxPosition);
    attackInputBox.setFillColor(Color(25,25,35));
    attackInputBox.setOutlineThickness(1.f);
    attackInputBox.setOutlineColor(Color::White);

    vector<EconomyLine> economyPanel = {
        {"Population", &nationalEconomy.population, false},
        {"Birth rate", &nationalEconomy.birthRate, true},
        {"Money", &nationalEconomy.money, false},
        {"Oil", &nationalEconomy.oil, false},
        {"Industry", &nationalEconomy.industry, false},
        {"Food", &nationalEconomy.food, false},
        {"Interest rate", &nationalEconomy.interestRate, true},
        {"Tax rate", &nationalEconomy.taxRate, true},
        {"Inflation", &nationalEconomy.inflationRate, true},
        {"Unemployment", &nationalEconomy.unemploymentRate, true},
        {"Controlled Area", &nationalEconomy.controlledArea, false},
        {"Economy Size", &nationalEconomy.economySize, false}
    };

    Vector2f setProdButtonStart(1320.f, 250.f);

    Button setProdButton(
        prod_panel.nextRowPosition(),

        Vector2f(200.f, 40.f),
        Color(90, 90, 90),
        uiFont, "Set Annual Production"
    );

    const int JETS_INDEX = 0;

    vector<ProductionSector> annualPlan = {
           {"Jets" , 0},
           {"Tanks" , 0},
           {"Troops" , 0},
           {"Trucks" , 0},
           {"Industrial Robots" , 0},
           {"Cars" , 0},
    };

    Button jetsPlusButton(
        Vector2f(1520.f, 340.f),
        Vector2f(28.f, 28.f),
        Color(90, 90, 90),
        uiFont,
        "+"
    );

    Button jetsMinusButton(
        Vector2f(1300.f, 340.f),
        Vector2f(28.f, 28.f),
        Color(90, 90, 90),
        uiFont,
        "-"
    );

    vector<ControlRow> productionControls;
    vector<ControlRow> economyControls;

    for (int i = 0; i < static_cast<int>(annualPlan.size()); i++) {
        Vector2f rowPos = stackedPosition(panelListStart, i, 28.f);

        productionControls.push_back({
            i,
            Button(Vector2f(1300.f, rowPos.y), Vector2f(28.f, 28.f), Color(90,90,90), uiFont, "-"),
            Button(Vector2f(1520.f, rowPos.y), Vector2f(28.f, 28.f), Color(90,90,90), uiFont, "+")
            });
    }

    for (int i = 0; i < static_cast<int>(economyPanel.size()); i++) {
        Vector2f rowPos = stackedPosition(panelListStart, i, 28.f);

        economyControls.push_back({
            i,
            Button(Vector2f(1300.f, rowPos.y), Vector2f(28.f, 28.f), Color(90,90,90), uiFont, "-"),
            Button(Vector2f(1520.f, rowPos.y), Vector2f(28.f, 28.f), Color(90,90,90), uiFont, "+")
            });
    }

    Text setProd = makeText("*Production has not\n"
        "been allocated yet.*",
        stackedPosition(
                    panelListStart,
                    static_cast<int>(annualPlan.size()),
                    28.f )
            );

    Text hundredProText = makeText("All production has been \n"
                                    "allocated for this year.\n"
                                    "Lower another sector to \n"
                                    "free ressources." ,
        stackedPosition(
            panelListStart,
            static_cast<int>(annualPlan.size()),
            28.f)
    );

    prod_panel.resizeForRows(3 + static_cast<int>(annualPlan.size()));
    econ_panel.resizeForRows(3 + static_cast<int>(economyPanel.size()));
    war_panel.resizeForRows(3 + static_cast<int>(warMenu.size()));

    bool settingAttackLength = false;
    string attackLengthText = "10";
    bool attackLengthInputActive = false;


    bool showSetProdText = true;

    int mainMenuRow = 0;

    auto nextMainMenuPosition = [&]() {         //callable object. NOT a function.
        return stackedPosition(menuButtonStart, mainMenuRow++, buttonSpacing);
        };

    vector<MenuChoice> mainMenu = {
        {Button(nextMainMenuPosition(),
                buttonSize,
                Color(90, 90, 90),
                uiFont,
                "Production"
                ),
                MenuView::Production
        },
        {Button(nextMainMenuPosition(),
                buttonSize,
                Color(90, 90, 90),
                uiFont,
                "Economy"
                ),
                MenuView::Economy
        },

        {Button(nextMainMenuPosition(),
                buttonSize,
                Color(90,90,90),
                uiFont,
                "War"
                ),
                MenuView::War
        },

        {
        Button(nextMainMenuPosition(),
                buttonSize,
                Color(90,90,90),
                uiFont,
                "Diplomacy"
                ),
                 MenuView::Diplomacy
        }
    };

    //Gameloop

    while (window.isOpen())
    {
        float deltaTime = clock.restart().asSeconds(); //method chaining is awesome!

        //Events
        while (const optional event = window.pollEvent()) //optional; container that may or may not contain a value.
        {
            if (event->is<Event::Closed>()) { // not a pointer..."if...event contains sth..."
                window.close();
            }

            if (const auto* mouseButton = event->getIf<Event::MouseButtonPressed>()) {
                if (mouseButton->button == Mouse::Button::Left) {
                    Vector2f mousePos = window.mapPixelToCoords(mouseButton->position,
                        window.getDefaultView());

                    for (MenuChoice& choice : mainMenu) {
                        if (choice.actualButton.isClicked(mousePos)) {
                            currentMenu = choice.opens;
                        }
                    }

                    if (surfaceButton.isClicked(mousePos)) {
                        currentView = MapView::Surface;
                    }

                    if (undergroundButton.isClicked(mousePos)) {
                        currentView = MapView::Underground;
                    }

                    if (quitButton.isClicked(mousePos)) {
                        window.close();
                    }

                    if (setProdButton.isClicked(mousePos)) {
                        showSetProdText = false;
                    }

                    if (attackButton.isClicked(mousePos)) {
                        settingAttackLength = true;
                        attackLengthInputActive = true;
                    }

                    


                    /* if (
                         jetsPlusButton.isClicked(mousePos) &&
                         showSetProdText == false) {
                         annualPlan[JETS_INDEX].percentage += 5;

                         if (annualPlan[JETS_INDEX].percentage > 100) {
                             annualPlan[JETS_INDEX].percentage = 100;
                         }
                     }*/
                     /*if (jetsMinusButton.isClicked(mousePos) &&
                         showSetProdText == false) {
                         annualPlan[JETS_INDEX].percentage -= 5;

                         if (annualPlan[JETS_INDEX].percentage < 0) {
                             annualPlan[JETS_INDEX].percentage = 0;
                         }
                     }*/
                     for (ControlRow& row : productionControls) {
                         if (row.plusButton.isClicked(mousePos) &&
                             showSetProdText == false &&
                             totalProductionPercent(annualPlan) <= 95) {

                             annualPlan[row.sectorIndex].percentage += 5;

                             /*if (annualPlan[row.sectorIndex].percentage > 100) {
                                 annualPlan[row.sectorIndex].percentage = 100;
                             }*/
                         }
                     }

                     for (ControlRow& row : productionControls) {
                         if (row.minusButton.isClicked(mousePos) &&
                             showSetProdText == false) {
                             annualPlan[row.sectorIndex].percentage -= 5;

                             if (annualPlan[row.sectorIndex].percentage < 0) {
                                 annualPlan[row.sectorIndex].percentage = 0;
                             }
                         }
                     }

                }

            }
            if (const auto* textEntered = event->getIf<Event::TextEntered>()) {
                char typed = static_cast<char>(textEntered->unicode);

                if (attackLengthInputActive && typed >= '0' && typed <= '9') {
                    attackLengthText.push_back(typed);
                }

                else if (attackLengthInputActive && typed == '\b' &&
                    !attackLengthText.empty()) {
                    attackLengthText.pop_back();
                }
            }

            if (const auto* keyPressed = event->getIf<Event::KeyPressed>()) {
                if (attackLengthInputActive && keyPressed->code == Keyboard::Key::Enter &&
                    !attackLengthText.empty()) {
                    attackLengthDays = stoi(attackLengthText);

                    attackLengthInputActive = false;
                    settingAttackLength = false;
                }

            }
        }
            //Camera input
            if (Keyboard::isKeyPressed(Keyboard::Key::Tab)) {
                toggle_map_view(currentView);
            }

            if (Keyboard::isKeyPressed(Keyboard::Key::Left)) {
                camera.move(Vector2f(-speed * deltaTime, 0.f));
            }
            if (Keyboard::isKeyPressed(Keyboard::Key::Right)) {
                camera.move(Vector2f(speed * deltaTime, 0.f));
            }
            if (Keyboard::isKeyPressed(Keyboard::Key::Up)) {
                camera.move(Vector2f(0.f, -speed * deltaTime));
            }
            if (Keyboard::isKeyPressed(Keyboard::Key::Down)) {
                camera.move(Vector2f(0.f, speed * deltaTime));
            }
            if (Keyboard::isKeyPressed(Keyboard::Key::Q)) {
                camera.zoom(1.01f);
            }
            if (Keyboard::isKeyPressed(Keyboard::Key::E)) {
                camera.zoom(0.99f);
            }

            // Update / Simulation
            gameState.dayTimer += deltaTime;

            //Draw
            window.clear();
            window.setView(camera);

            for (int y = 0; y < world.getHeight(); y++) {
                for (int x = 0; x < world.getWidth(); x++) {
                    sq_tile.setPosition(Vector2f(x * sq_tile_size, y * sq_tile_size));

                    Tile& tile = world.get(x, y);
                    sq_tile.setFillColor(get_tile_color(tile, currentView));

                    window.draw(sq_tile);
                }
            }
            window.draw(capitalMarker);

            //Draw buttons
            window.setView(window.getDefaultView());

            if (currentMenu == MenuView::Production) {
                //window.draw(prod_panel);  //window.draw(sth); when sth is a SFML
                                        //drawable type:RectangleShape, Text, etc...
                prod_panel.draw(window);
                window.draw(productionTitle);
                setProdButton.draw(window);
                if (showSetProdText) {
                    window.draw(setProd);
                }
                if (totalProductionPercent(annualPlan) >= 100) {
                    window.draw(hundredProText);
                }
                /*jetsPlusButton.draw(window);
                jetsMinusButton.draw(window);*/
                for (ControlRow& row:productionControls) {
                    row.minusButton.draw(window);
                    row.plusButton.draw(window);
                }

                //Vector2f prodListStart(1340.f, 340.f);

                for (int i = 0; i < annualPlan.size(); i++) {
                    Text row(uiFont);
                    row.setString(annualPlan[i].name + ": " + to_string(annualPlan[i].percentage) + "%");
                    row.setCharacterSize(18);
                    row.setFillColor(Color::White);
                    row.setPosition(stackedPosition(panelListStart,
                        i,
                        28.f));
                    window.draw(row);
                }
            }

            if (currentMenu == MenuView::Economy) {
                econ_panel.draw(window);
                window.draw(economyTitle);

                for (ControlRow& row:economyControls) {
                    row.minusButton.draw(window);
                    row.plusButton.draw(window);
                }

                for (int i = 0; i < economyPanel.size(); i++) {
                    Text row(uiFont);
                    if (economyPanel[i].isPercent == true) {
                        row.setString(economyPanel[i].name + ": " + formatDouble(*economyPanel[i].value * 100) + "%");
                    }
                    else {
                        row.setString(economyPanel[i].name + ": " + to_string(static_cast<int>(*economyPanel[i].value)));
                    }
                    row.setCharacterSize(18);
                    row.setFillColor(Color::White);
                    row.setPosition(stackedPosition(panelListStart,
                        i,
                        28.f));
                    window.draw(row);
                }
            }

            if (currentMenu == MenuView::War) {
                war_panel.draw(window);
                window.draw(warTitle);

                if (settingAttackLength) {
                    war_panel.draw(window);
                    window.draw(howMany);

                    Text attackInput = makeText(attackLengthText, stackedPosition(panelListStart, 1, 28.f));
                    window.draw(attackInputBox);
                    window.draw(attackInput);
                }
                else {
                    attackButton.draw(window);
                
                    for (int i = 1; i < warMenu.size(); i++) {
                        Text row = makeText(
                            warMenu[i],
                            stackedPosition(panelListStart, i, 28.f)
                        );

                        window.draw(row);
                    }
                }
            }

            quitButton.draw(window);
            mainMenuButton.draw(window);
            surfaceButton.draw(window);
            undergroundButton.draw(window);

            for (const MenuChoice& choice : mainMenu) {
                choice.actualButton.draw(window);
            }

            //Display
            window.display();

    }
    return 0;
}
