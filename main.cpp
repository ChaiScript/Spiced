#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <chrono>
#include <functional>
#include <cassert>

struct TileProperties
{
  TileProperties(bool t_passable = true, 
      std::function<void (float, float)> t_movementAction = [](float time, float distance) { 
//        std::cout << "Traveled " << distance << " pixels in " << time << "seconds\n";
      })

    : passable(t_passable), movementAction(std::move(t_movementAction))
  {
  }

  bool passable;
  std::function<void (float, float)> movementAction;
};

struct LineSegment
{
  LineSegment(sf::Vector2f t_p1, sf::Vector2f t_p2)
    : p1(std::move(t_p1)), p2(std::move(t_p2)), valid(true)
  {
  }

  LineSegment()
    : valid(false)
  {
  }

  LineSegment(const LineSegment &) = default;
  LineSegment(LineSegment &&) = default;

  LineSegment &operator=(const LineSegment &) = default;
  LineSegment &operator=(LineSegment &&) = default;

  float x(const float y) const
  {
    return ((y-p1.y)*(p2.x-p1.x))/(p2.y - p1.y) + p1.x;
  }

  float y(const float x) const
  {
    return ((p2.y-p1.y)*(x - p1.x))/(p2.x - p1.x) + p1.y;
  }

  explicit operator bool() const
  {
    return valid;
  }

  float distanceToP1(const sf::Vector2f &t_point) const
  {
    return sqrtf( (t_point.x - p1.x) * (t_point.x - p1.x) + (t_point.y - p1.y) * (t_point.y - p1.y) );
  }

  float length() const 
  {
    if (p1 == p2) return 0;

    return sqrtf( (p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y) );
  }

  sf::FloatRect boundingRect() const
  {
    const auto x1 = std::min(p1.x, p2.x);
    const auto y1 = std::min(p1.y, p2.y);

    const auto width = std::max(p1.x, p2.x) - x1;
    const auto height = std::max(p1.y, p2.y) - y1;

    return sf::FloatRect(x1, y1, width, height);
  }

  LineSegment clipTo(const sf::FloatRect &t_rect) const
  {
    auto containsP1 = t_rect.contains(p1);
    auto containsP2 = t_rect.contains(p2);
    if (containsP1 && containsP2)
    {
      return *this;
    }

    const sf::FloatRect bounds = boundingRect();

    auto validate = [&t_rect, &bounds](const float x, const float y) {
      return    t_rect.left <= x && t_rect.top <= y && (t_rect.left + t_rect.width) >= x && (t_rect.top + t_rect.height) >= y
             && bounds.left <= x && bounds.top <= y && (bounds.left + bounds.width) >= x && (bounds.top + bounds.height) >= y;
    };

    auto edgeX = [&t_rect](const sf::Vector2f &t_p1, const sf::Vector2f &t_p2)
    {
      if (t_p2.x > t_p1.x) { //moving left to right
        return (t_rect.left + t_rect.width) - std::numeric_limits<float>::epsilon(); // try right edge
      } else {
        return t_rect.left; // try left edge
      }
    };

    auto edgeY = [&t_rect](const sf::Vector2f &t_p1, const sf::Vector2f &t_p2)
    {
      if (t_p2.y > t_p1.y) { // moving top to bottom
        return (t_rect.top + t_rect.height) - std::numeric_limits<float>::epsilon(); // try bottom edge
      } else {
        return t_rect.top; // try top edge
      }
    };

    if (containsP1)
    {
      auto possibleX = edgeX(p1, p2);
      auto possibleY = edgeY(p1, p2);

      if (validate(x(possibleY), possibleY)) {
        return LineSegment(p1, sf::Vector2f(x(possibleY), possibleY));
      } else if (validate(possibleX, y(possibleX))) {
        return LineSegment(p1, sf::Vector2f(possibleX, y(possibleX)));
      } else {
        return LineSegment(); // no possible match
      }
    }

    if (containsP2)
    {
      auto possibleX = edgeX(p2, p1);
      auto possibleY = edgeY(p2, p1);

      if (validate(x(possibleY), possibleY)) {
        return LineSegment(sf::Vector2f(x(possibleY), possibleY), p2);
      } else if (validate(possibleX, y(possibleX))) {
        return LineSegment(sf::Vector2f(possibleX, y(possibleX)), p2);
      } else {
        return LineSegment(); // no possible match
      }
    }

    // it contains neither, so let's now try to figure it out
    auto possibleX1 = edgeX(p1, p2);
    auto possibleY1 = edgeY(p1, p2);

    sf::Vector2f resultP1;
    if (validate(x(possibleY1), possibleY1)) {
      resultP1 = sf::Vector2f(x(possibleY1), possibleY1);
    } else if (validate(possibleX1, y(possibleX1))) {
      resultP1 = sf::Vector2f(possibleX1, y(possibleX1));
    } else {
      return LineSegment(); // no possible match
    }

    auto possibleX2 = edgeX(p2, p1);
    auto possibleY2 = edgeY(p2, p1);

    sf::Vector2f resultP2;
    if (validate(x(possibleY2), possibleY2)) {
      resultP2 = sf::Vector2f(x(possibleY2), possibleY2);
    } else if (validate(possibleX2, y(possibleX2))) {
      resultP2 = sf::Vector2f(possibleX2, y(possibleX2));
    } else {
      return LineSegment(); // no possible match
    }

    return LineSegment(std::move(resultP1), std::move(resultP2));
  }

  sf::Vector2f p1;
  sf::Vector2f p2;
  bool valid;
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

    void doMove(float t_time, sf::Sprite &t_s, const sf::Vector2f &distance)
    {
      auto bounds = t_s.getGlobalBounds();

      auto center = sf::Vector2f(bounds.left + bounds.width/2, bounds.top + bounds.height/2);
      auto endCenter = center + distance;

      auto movementBounds = sf::FloatRect(std::min(center.x, endCenter.x)-1, std::min(center.y, endCenter.y)-1, distance.x+2, distance.y+2);

      auto segment = LineSegment(center, endCenter);

      std::vector<std::tuple<std::reference_wrapper<TileData>, LineSegment, float>> segments;
      for (auto &data : m_tileData)
      {
        if (data.bounds.intersects(movementBounds))
        {
          // this is a potential box that we've passed through
          if (auto passedSegment = segment.clipTo(data.bounds))
          {
            // it's a valid segment
            segments.push_back(std::make_tuple(std::ref(data), passedSegment, passedSegment.distanceToP1(center)));
          }
        }
      }

      std::sort(segments.begin(), segments.end(),
          [](const std::tuple<std::reference_wrapper<TileData>, LineSegment, float> &t_lhs,
             const std::tuple<std::reference_wrapper<TileData>, LineSegment, float> &t_rhs)
          {
            return std::get<2>(t_lhs) < std::get<2>(t_rhs);
          }
        );

      auto totalLength = segment.length();


      auto total = 0.0;
      // segments should now contain a sorted list of tiles that this movement passes through
      for (auto &curSegment : segments)
      {
        auto length = std::get<1>(curSegment).length();
        auto percent = totalLength==0?1:(length / totalLength);
        total += percent;

        std::get<0>(curSegment).get().properties.movementAction(t_time * percent, length);
      }

      assert(total >= 0.999);
      assert(total <= 1.001);
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
    map.doMove(time_elapsed, stickMan, distance);
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


