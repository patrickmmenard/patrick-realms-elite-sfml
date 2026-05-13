#include <SFML/Graphics.hpp>
#include <optional>
#include <random>
#include <vector>
#include <string>
#include <utility>
#include <iostream>

using namespace sf;
using namespace std;

enum class TerrainType {
    Plain, 
    Water
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
    Grid(int w, int h) : width(w), height(h), tiles(w*h) { // : != ::   Here, ':' does not mean "belong
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

class Button {
private:
    RectangleShape shape;
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
        label.setPosition(Vector2f(position.x + 18.f,
            position.y + 8.f));
    }

    bool isClicked(Vector2f mousePos) const {
        return shape.getGlobalBounds().contains(mousePos);
    }

    void draw(RenderWindow& window) const {
        window.draw(shape);
        window.draw(label);
    }
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

struct MenuButton {
    Button button;
    MenuView targetMenu;
};

Vector2f stackedPosition(Vector2f start,
                        int index,
                        float spacing) {
    return Vector2f(start.x,
                    start.y + index * spacing);
}

struct ProductionItem {
    string name;
    int percentage = 0;
};

int totalProductionPercent(const vector<ProductionItem>& items) {
    int total = 0;

    for (const ProductionItem& item: items) {
        total += item.percentage;
    }

    return total;
}

struct GameEvent {
    string message;
};

struct EventBuffer { //"Carousel of Doom. inspired by Doom's circular buffer/event queue. Interesting for learning purposes. 
    static constexpr int MAXEVENTS = 16;

    GameEvent events[MAXEVENTS]; //Array.
    int eventhead = 0;
    int eventtail = 0;
};

void D_PostEvent(EventBuffer& buffer, GameEvent event) {
    buffer.events[buffer.eventhead] = event;
    buffer.eventhead = (buffer.eventhead + 1) % EventBuffer::MAXEVENTS; //New events are logged at eventhead; eventhead rotates thru fixed array.
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

int main()
{
    //SETUP

    int x = 0;
    int y = 0;
    const float speed = 700.f;

    EventBuffer eventBuffer;

    Grid world(160, 90);

    int grid_width = world.getWidth();
    int grid_height = world.getHeight();

    pair<int, int> playerCapital = random_coord(world);
    world.get(playerCapital.first, playerCapital.second).ownerId = 0;

    D_PostEvent(eventBuffer, { "Capital founded" });

    LakeData lake = generate_lake(world);
    int lake_count = rand_int(3, 7);
    create_lakes(world, eventBuffer, lake_count, paint_water, "Lake generated");

    LakeData oilLake = generate_lake(world);
    int oilLake_count = rand_int(2, 5);
    create_lakes(world, eventBuffer, oilLake_count, paint_oil, "Oil lake generated");

    //world.get(5, 4).terrain = TerrainType::Water;

    //VideoMode mode({ 800u, 600u });
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
    capitalMarker.setPosition(Vector2f(playerCapital.first* sq_tile_size,
        playerCapital.second* sq_tile_size));


    Vector2f menuButtonStart(1620.f, 200.f);
    Vector2f buttonSize(140.f, 40.f);
    float buttonSpacing = 50.f;
    float textSpacing = 40.f;

    Font uiFont;
    if (!uiFont.openFromFile("arial.ttf")) {
        return 1;
    }

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

    Button menuButton(
        Vector2f(1720.f, 760.f),
        Vector2f(140.f, 40.f),
        Color(90, 90, 90),
        uiFont, "Menu"
    );

    /*Text productionTitle(uiFont);
    productionTitle.setString("Production");
    productionTitle.setCharacterSize(22);
    productionTitle.setFillColor(Color::White);
    productionTitle.setPosition(Vector2f(1300.f, 220.f));*/
    Panel prod_panel(Vector2f(1280.f, 200.f));

    Text productionTitle = makeText("Production", prod_panel.nextRowPosition());

     //RectangleShape prod_panel(Vector2f(320.f, 260.f));
     //prod_panel.setPosition(Vector2f(1280.f, 200.f));
     //prod_panel.setFillColor(Color(45, 45, 60));
    

     Vector2f setProdButtonStart(1320.f, 250.f);

     Button setProdButton(
         prod_panel.nextRowPosition(),
            
        Vector2f(200.f, 40.f),
        Color(90, 90, 90),
        uiFont, "Set Annual Production"

    );

    vector<ProductionItem> productionItems = {
           {"Jets" , 0},
           {"Tanks" , 0},
           {"Troops" , 0},
           {"Trucks" , 0},
           {"Industrial Robots" , 0},
           {"Cars" , 0},
    };


    Text setProd = makeText("*Production has not\n"
                            "been allocated yet.*",
                   prod_panel.nextRowPosition());

    prod_panel.resizeForRows(3 + static_cast<int>(productionItems.size()));

    bool showSetProdText = true;

    vector<MenuButton> menuButtons = {
        {Button(menuButtonStart, 
                buttonSize, 
                Color(90, 90, 90),
                uiFont, 
                "Production"
                ),
                MenuView::Production
        },
        {
        Button(Vector2f(
                menuButtonStart.x,
                menuButtonStart.y + buttonSpacing
            ), 
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

                    for (MenuButton& menuButton : menuButtons) {
                        if (menuButton.button.isClicked(mousePos)) {
                            currentMenu = menuButton.targetMenu;
                        }
                    }

                    if (surfaceButton.isClicked(mousePos)) {
                        currentView = MapView::Surface;
                    }

                    if (undergroundButton.isClicked(mousePos)) {
                        currentView = MapView::Underground;
                    }

                    if (setProdButton.isClicked(mousePos)) {
                        showSetProdText = false;

                    }
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

            Vector2f prodListStart(1320.f, 340.f);

            for (int i = 0; i < productionItems.size(); i++) {
                Text row(uiFont);
                row.setString(productionItems[i].name + ": " +
                    to_string(productionItems[i].percentage) + "%");
                row.setCharacterSize(18);
                row.setFillColor(Color::White);
                row.setPosition(stackedPosition(prodListStart,
                                                i,
                                                28.f ));
                window.draw(row);
            }
        }

        menuButton.draw(window);
        surfaceButton.draw(window);
        undergroundButton.draw(window);
       
        for (const MenuButton& menuButton : menuButtons) {
            menuButton.button.draw(window);
        }

            //Display
            window.display();
        
    }
    return 0;
}
