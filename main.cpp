#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <chrono>

struct TileProperties
{
  TileProperties(bool t_passable = true)
    : passable(t_passable)
  {
  }

  bool passable;
};

struct TileData
{
  TileData(int t_x, int t_y, TileProperties t_props, sf::FloatRect t_bounds)
    : x(t_x), y(t_y), properties(std::move(t_props)), bounds(std::move(t_bounds))
  {
  }

  int x;
  int y;
  TileProperties properties;
  sf::FloatRect bounds;
};

class TileMap : public sf::Drawable, public sf::Transformable
{
  public:
    TileMap(std::map<int, TileProperties> t_mapDefaults)
      : m_mapDefaults(std::move(t_mapDefaults))
    {
    }

    bool load(const std::string& tileset, sf::Vector2u tileSize, const int* tiles, unsigned int width, unsigned int height)
    {
      // load the tileset texture
      if (!m_tileset.loadFromFile(tileset))
        return false;

      // resize the vertex array to fit the level size
      m_vertices.setPrimitiveType(sf::Quads);
      m_vertices.resize(width * height * 4);
      m_tileData.reserve(width * height);

      // populate the vertex array, with one quad per tile
      for (unsigned int i = 0; i < width; ++i)
      {
        for (unsigned int j = 0; j < height; ++j)
        {
          // get the current tile number
          int tileNumber = tiles[i + j * width];


          auto tilePropsFunc = [tileNumber, this](){
            auto defaults_itr = m_mapDefaults.find(tileNumber);
            if (defaults_itr != m_mapDefaults.end()) {
              return defaults_itr->second;
            } else {
              return TileProperties();
            }
          };


          m_tileData.emplace_back(i, j, tilePropsFunc(), sf::FloatRect(i * tileSize.x, j * tileSize.y, tileSize.x, tileSize.y));


          // find its position in the tileset texture
          int tu = tileNumber % (m_tileset.getSize().x / tileSize.x);
          int tv = tileNumber / (m_tileset.getSize().x / tileSize.x);

          // get a pointer to the current tile's quad
          sf::Vertex* quad = &m_vertices[(i + j * width) * 4];


          // define its 4 corners
          quad[0].position = sf::Vector2f(i * tileSize.x, j * tileSize.y);
          quad[1].position = sf::Vector2f((i + 1) * tileSize.x, j * tileSize.y);
          quad[2].position = sf::Vector2f((i + 1) * tileSize.x, (j + 1) * tileSize.y);
          quad[3].position = sf::Vector2f(i * tileSize.x, (j + 1) * tileSize.y);


          // define its 4 texture coordinates
          quad[0].texCoords = sf::Vector2f(tu * tileSize.x, tv * tileSize.y);
          quad[1].texCoords = sf::Vector2f((tu + 1) * tileSize.x, tv * tileSize.y);
          quad[2].texCoords = sf::Vector2f((tu + 1) * tileSize.x, (tv + 1) * tileSize.y);
          quad[3].texCoords = sf::Vector2f(tu * tileSize.x, (tv + 1) * tileSize.y);
        }
      }

      return true;
    }

    bool testMove(const sf::Sprite &t_s, const sf::Vector2f &distance) const
    {
      auto newBoundingBox = sf::Transform().translate(distance).transformRect(t_s.getGlobalBounds());

      for (const auto &data : m_tileData)
      {
        if (!data.properties.passable && data.bounds.intersects(newBoundingBox))
        {
          return false;
        }
      }

      return true;
    }

    sf::Vector2f adjustMove(const sf::Sprite &t_s, const sf::Vector2f &distance)
    {
      if (testMove(t_s, distance)) {
        return distance;
      }

      auto xOnly = sf::Vector2f(distance.x, 0);
      if (testMove(t_s, xOnly)) {
        return xOnly;
      }

      auto yOnly = sf::Vector2f(distance.y, 0);
      if (testMove(t_s, yOnly)) {
        return yOnly;
      }

      return sf::Vector2f(0,0);

    }

  private:

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
      // apply the transform
      states.transform *= getTransform();

      // apply the tileset texture
      states.texture = &m_tileset;

      // draw the vertex array
      target.draw(m_vertices, states);
    }

    sf::VertexArray m_vertices;
    sf::Texture m_tileset;
    std::vector<TileData> m_tileData;
    std::map<int, TileProperties> m_mapDefaults;
};

int main()
{
  // create the window
  sf::RenderWindow window(sf::VideoMode(512, 256), "Tilemap");

  sf::Texture spritetexture;
  spritetexture.loadFromFile("sprite.png");

  sf::Sprite stickMan(spritetexture);
  stickMan.setPosition(200, 200);


  // define the level with an array of tile indices
  const int level[] =
  {
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3,
    0, 1, 0, 0, 2, 0, 3, 3, 3, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 2, 0, 3, 3, 3, 0, 1, 1, 1, 0, 0, 0,
    0, 1, 1, 0, 3, 3, 3, 0, 0, 0, 1, 1, 1, 2, 0, 0, 0, 1, 1, 0, 3, 3, 3, 0, 0, 0, 1, 1, 1, 2, 0, 0,
    0, 0, 1, 0, 3, 0, 2, 2, 0, 0, 1, 1, 1, 1, 2, 0, 0, 0, 1, 0, 3, 0, 2, 2, 0, 0, 1, 1, 1, 1, 2, 0,
    2, 0, 1, 0, 3, 0, 2, 2, 2, 0, 1, 1, 1, 1, 1, 1, 2, 0, 1, 0, 3, 0, 2, 2, 2, 0, 1, 1, 1, 1, 1, 1,
    0, 0, 1, 0, 3, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 3, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3,
    0, 1, 0, 0, 2, 0, 3, 3, 3, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 2, 0, 3, 3, 3, 0, 1, 1, 1, 0, 0, 0,
    0, 1, 1, 0, 3, 3, 3, 0, 0, 0, 1, 1, 1, 2, 0, 0, 0, 1, 1, 0, 3, 3, 3, 0, 0, 0, 1, 1, 1, 2, 0, 0,
    0, 0, 1, 0, 3, 0, 2, 2, 0, 0, 1, 1, 1, 1, 2, 0, 0, 0, 1, 0, 3, 0, 2, 2, 0, 0, 1, 1, 1, 1, 2, 0,
    2, 0, 1, 0, 3, 0, 2, 2, 2, 0, 1, 1, 1, 1, 1, 1, 2, 0, 1, 0, 3, 0, 2, 2, 2, 0, 1, 1, 1, 1, 1, 1,
    0, 0, 1, 0, 3, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 3, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1,

  };

  // create the tilemap from the level definition
  TileMap map({{1, TileProperties(false)}});

  if (!map.load("tileset.png", sf::Vector2u(32, 32), level, 32, 16))
    return -1;

  auto last_frame = std::chrono::steady_clock::now();
  uint64_t frame_count = 0;

  // run the main loop
  while (window.isOpen())
  {
    ++frame_count;
    auto cur_frame = std::chrono::steady_clock::now();
    auto time_elapsed = std::chrono::duration_cast<std::chrono::duration<float>>(cur_frame - last_frame).count();
    last_frame = cur_frame;

    if (frame_count % 100 == 0)
    {
      std::cout << 1/time_elapsed << "fps\n";
    }

    // handle events
    sf::Event event;
    while (window.pollEvent(event))
    {
      if(event.type == sf::Event::Closed)
        window.close();
    }

    sf::Vector2f velocity(0,0);

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
    {
      velocity += sf::Vector2f(-10, 0);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
    {
      velocity += sf::Vector2f(10, 0);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
    {
      velocity += sf::Vector2f(0, -10);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
    {
      velocity += sf::Vector2f(0, 10);
    }

    velocity += sf::Vector2f(sf::Joystick::getAxisPosition(2, sf::Joystick::Axis::X)/100 * 20, sf::Joystick::getAxisPosition(2, sf::Joystick::Axis::Y)/100 * 20);

    auto distance = map.adjustMove(stickMan, velocity * time_elapsed);
    stickMan.move(distance);

    sf::View mainView(stickMan.getPosition(), sf::Vector2f(512,256));
    window.setView(mainView);

    // draw the map
    window.clear();
    window.draw(map);
    window.draw(stickMan);

    sf::View miniView(sf::FloatRect(0,0,1024,512));
    miniView.setViewport(sf::FloatRect(0.75f, 0, 0.25f, 0.25f));
    window.setView(miniView);

    // draw the map
    window.draw(map);
    window.draw(stickMan);
    window.display();
  }

  return 0;
}


