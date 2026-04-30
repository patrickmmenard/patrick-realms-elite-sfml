#include <SFML/Graphics.hpp>
#include <optional>
#include <random>
#include <vector>
#include <string>
#include <utility>

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

struct Tile {                           // in a struct, members are public by default.
    TerrainType terrain = TerrainType::Plain;
    ResourceType resource = ResourceType::None;
    int ownerId = -1;
};

class Grid {                            //in a class, members are private by default. 

private:
    int width;
    int height;
    vector<Tile> tiles;

public:
    Grid(int w, int h) : width(w), height(h), tiles(w*h) { // : != ::.Here, ':' does not mean "belong
                                                      //to a class; instead ": starts the member initializer list". 
    }

    int getWidth() const {
        return width;
    }

    int getHeight() const {
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
    buffer.eventhead = (buffer.eventhead + 1) % EventBuffer::MAXEVENTS;
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

void create_lake(Grid& world, LakeData lake) { //simplify parameters by putting lake_seed and size in a class
    int lx = lake.seed.first;
    int ly = lake.seed.second;

    for (int y = ly; y < ly + lake.size; y++) {
            for (int x = lx; x < lx + lake.size; x++) {
                world.get(x, y).terrain = TerrainType::Water;
            }
        }
}

void create_lakes(Grid& world, EventBuffer& eventBuffer, int lake_count) {
    for (int i = 0; i < lake_count; i++) {
        LakeData lake = generate_lake(world);
        create_lake(world, lake);

        D_PostEvent(eventBuffer, { "Lake generated" });
    }
}




int main()
{
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

    int lake_count = rand_int(3 , 7);
    create_lakes(world, eventBuffer, lake_count);

    D_PostEvent(eventBuffer, { "Lake generated" });
    
    //world.get(5, 4).terrain = TerrainType::Water;

    //VideoMode mode({ 800u, 600u });
    VideoMode desktop = VideoMode::getDesktopMode();
    RenderWindow window(desktop, "SFML works!", State::Fullscreen);
    window.setFramerateLimit(60);
    View camera = window.getDefaultView();

    Clock clock;

    float sq_tile_size = 32.f;
    RectangleShape sq_tile(Vector2f(sq_tile_size, sq_tile_size));
    sq_tile.setOutlineThickness(1.f);
    sq_tile.setOutlineColor(Color(30,80,30));   //RGB: red, green, blue values

    float capitalMarker_size = 8.f;
    CircleShape capitalMarker(capitalMarker_size);
    capitalMarker.setFillColor(Color::Red);
    capitalMarker.setPosition(Vector2f(playerCapital.first * sq_tile_size ,
        playerCapital.second * sq_tile_size));

    
   
    while (window.isOpen())
    {
        float deltaTime = clock.restart().asSeconds(); //method chaining is awesome!

        //Events
        while (const optional event = window.pollEvent()) //optional; container that may or may not contain a value.
        {
            if (event->is<Event::Closed>()) // not a pointer..."if...event contains sth..."
                window.close();
        }
        
        //Camera input
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

                if (world.get(x, y).terrain == TerrainType::Water) {
                    sq_tile.setFillColor(Color::Blue);
                }
                else { sq_tile.setFillColor(Color::Green); }

                window.draw(sq_tile);
                window.draw(capitalMarker);
            }
        }
   
       //Display
        window.display();
    }

    return 0;
}
