#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <chrono>
#include <functional>
#include <deque>
#include <memory>
#include <cassert>
#include <cmath>

class Game_Event : public sf::Drawable, public sf::Transformable
{
  public:
    Game_Event()
    {
    }

    virtual ~Game_Event() = default;

    virtual bool is_done() const = 0;

    virtual void update(const float t_game_time, const float t_simulation_time) = 0;

};

class Queued_Action : public Game_Event
{
  public:
    Queued_Action(std::function<void ()> t_action)
      : m_action(std::move(t_action))
    { }

    virtual void update(const float, const float)
    {
      m_action();
      m_done = true;
    }

    virtual bool is_done() const
    {
      return m_done;
    }

    virtual void draw(sf::RenderTarget& /*target*/, sf::RenderStates /*states*/) const
    { /*nothing to do*/ }

  private:
    bool m_done = false;
    std::function<void ()> m_action;
};

class Message_Box : public Game_Event
{
  public:
    Message_Box(sf::String t_string, sf::Font t_font, int t_fontSize,
        sf::Color t_font_color, sf::Color t_fill_color, sf::Color t_outline_color, float t_outlineThickness)
      : Game_Event(),
        m_string(std::move(t_string)), m_font(std::move(t_font)), m_font_color(std::move(t_font_color)), 
        m_fill_color(std::move(t_fill_color)), m_outline_color(std::move(t_outline_color)),
        m_outline_thickness(t_outlineThickness),
        m_text(t_string, m_font, t_fontSize)
    {
      setPosition(10,10);
      m_text.setColor(m_font_color);
    }

    virtual ~Message_Box() = default;

    virtual void update(const float t_game_time, const float /*t_simulation_time*/)
    {
      if (m_start_time == 0) m_start_time = t_game_time;

      if (t_game_time - m_start_time >= 1 && sf::Keyboard::isKeyPressed(sf::Keyboard::Return))
      {
        m_is_done = true;
      }
    }

    virtual bool is_done() const
    {
      return m_is_done;
    }

  protected:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
      auto size = target.getView().getSize();
      size.x -= 20;
      size.y -= 20;

      // apply the transform
      states.transform *= getTransform();

      sf::RectangleShape rect(size);
      rect.setFillColor(m_fill_color);
      rect.setOutlineColor(m_outline_color);
      rect.setOutlineThickness(m_outline_thickness);

      target.draw(rect, states);
      target.draw(m_text, states);
    }

  private:
    sf::String m_string;
    sf::Font m_font;
    sf::Color m_font_color;
    sf::Color m_fill_color;
    sf::Color m_outline_color;
    float m_outline_thickness;
    sf::Text m_text;

    float m_start_time = 0;
    bool m_is_done = false;
};



class Object : public sf::Sprite
{
  public:
    Object(const sf::Texture &t_texture, const int width, const int height, const float fps)
      : m_texture(std::cref(t_texture)), m_width(width), m_height(height), m_fps(fps)
    {
      setTexture(m_texture);
      setTextureRect(sf::IntRect(0,0,width,height));
      m_num_frames = m_texture.get().getSize().x / width;
    }

    virtual ~Object() = default;

    void update(const float t_game_time, const float /*t_simulation_time*/)
    {
      auto i = 0.0f;
      const auto remainder = modff(t_game_time, &i);
      const auto seconds_per_frame = 1/m_fps;
      const auto cur_step = int(remainder / seconds_per_frame);
      const auto cur_frame = cur_step % m_num_frames;
      assert(cur_frame >= 0 && cur_frame < m_num_frames);
      setTextureRect(sf::IntRect(m_width * cur_frame, 0, m_width, m_height));
    }

  private:
    std::reference_wrapper<const sf::Texture> m_texture;
    int m_width;
    int m_height;
    int m_num_frames;
    float m_fps;
};

struct Tile_Properties
{
  Tile_Properties(bool t_passable = true, 
      std::function<void (float, float)> t_movement_action = std::function<void (float, float)>())
    : passable(t_passable), movement_action(std::move(t_movement_action))
  {
  }

  void do_movement_action(const float t_game_time, const float t_simulation_time)
  {
    if (movement_action)
    {
      movement_action(t_game_time, t_simulation_time);
    }
  }

  bool passable;
  std::function<void (float, float)> movement_action;
};

struct Line_Segment
{
  Line_Segment(sf::Vector2f t_p1, sf::Vector2f t_p2)
    : p1(std::move(t_p1)), p2(std::move(t_p2)), valid(true)
  {
  }

  Line_Segment()
    : valid(false)
  {
  }

  Line_Segment(const Line_Segment &) = default;
  Line_Segment(Line_Segment &&) = default;

  Line_Segment &operator=(const Line_Segment &) = default;
  Line_Segment &operator=(Line_Segment &&) = default;

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

  float distance_to_p1(const sf::Vector2f &t_point) const
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

  Line_Segment clipTo(const sf::FloatRect &t_rect) const
  {
    auto contains_p1 = t_rect.contains(p1);
    auto contains_p2 = t_rect.contains(p2);

    if (contains_p1 && contains_p2)
    {
      return *this;
    }

    const auto bounds = boundingRect();

    auto validate = [&t_rect, &bounds](const float x, const float y) {
      return    t_rect.left <= x && t_rect.top <= y && (t_rect.left + t_rect.width) >= x && (t_rect.top + t_rect.height) >= y
             && bounds.left <= x && bounds.top <= y && (bounds.left + bounds.width) >= x && (bounds.top + bounds.height) >= y;
    };

    auto edge_x = [&t_rect](const sf::Vector2f &t_p1, const sf::Vector2f &t_p2)
    {
      if (t_p2.x > t_p1.x) { //moving left to right
        return (t_rect.left + t_rect.width) - std::numeric_limits<float>::epsilon(); // try right edge
      } else {
        return t_rect.left; // try left edge
      }
    };

    auto edge_y = [&t_rect](const sf::Vector2f &t_p1, const sf::Vector2f &t_p2)
    {
      if (t_p2.y > t_p1.y) { // moving top to bottom
        return (t_rect.top + t_rect.height) - std::numeric_limits<float>::epsilon(); // try bottom edge
      } else {
        return t_rect.top; // try top edge
      }
    };

    if (contains_p1)
    {
      auto possible_x = edge_x(p1, p2);
      auto possible_y = edge_y(p1, p2);

      if (validate(x(possible_y), possible_y)) {
        return Line_Segment(p1, sf::Vector2f(x(possible_y), possible_y));
      } else if (validate(possible_x, y(possible_x))) {
        return Line_Segment(p1, sf::Vector2f(possible_x, y(possible_x)));
      } else {
        return Line_Segment(); // no possible match
      }
    }

    if (contains_p2)
    {
      auto possible_x = edge_x(p2, p1);
      auto possible_y = edge_y(p2, p1);

      if (validate(x(possible_y), possible_y)) {
        return Line_Segment(sf::Vector2f(x(possible_y), possible_y), p2);
      } else if (validate(possible_x, y(possible_x))) {
        return Line_Segment(sf::Vector2f(possible_x, y(possible_x)), p2);
      } else {
        return Line_Segment(); // no possible match
      }
    }

    // it contains neither, so let's now try to figure it out
    auto possible_x1 = edge_x(p1, p2);
    auto possible_y1 = edge_y(p1, p2);

    sf::Vector2f result_p1;
    if (validate(x(possible_y1), possible_y1)) {
      result_p1 = sf::Vector2f(x(possible_y1), possible_y1);
    } else if (validate(possible_x1, y(possible_x1))) {
      result_p1 = sf::Vector2f(possible_x1, y(possible_x1));
    } else {
      return Line_Segment(); // no possible match
    }

    auto possible_x2 = edge_x(p2, p1);
    auto possible_y2 = edge_y(p2, p1);

    sf::Vector2f result_p2;
    if (validate(x(possible_y2), possible_y2)) {
      result_p2 = sf::Vector2f(x(possible_y2), possible_y2);
    } else if (validate(possible_x2, y(possible_x2))) {
      result_p2 = sf::Vector2f(possible_x2, y(possible_x2));
    } else {
      return Line_Segment(); // no possible match
    }

    return Line_Segment(std::move(result_p1), std::move(result_p2));
  }

  sf::Vector2f p1;
  sf::Vector2f p2;
  bool valid;
};

struct Tile_Data
{
  Tile_Data(int t_x, int t_y, Tile_Properties t_props, sf::FloatRect t_bounds)
    : x(t_x), y(t_y), properties(std::move(t_props)), bounds(std::move(t_bounds))
  {
  }

  int x;
  int y;
  Tile_Properties properties;
  sf::FloatRect bounds;
};

class Game;

class Tile_Map : public sf::Drawable, public sf::Transformable
{
  public:
    Tile_Map(const sf::Texture &t_tileset,
            const sf::Vector2u &t_tile_size, const std::vector<int> &tiles, const unsigned int width, const unsigned int height, std::map<int, Tile_Properties> t_map_defaults)
      : m_tileset(std::cref(t_tileset)), m_map_defaults(std::move(t_map_defaults))
    {
      load(t_tile_size, tiles, width, height);
    }

    virtual ~Tile_Map() = default;

    void add_enter_action(const std::function<void (Game &)> t_action)
    {
      m_enter_actions.push_back(t_action);
    }

    void enter(Game &t_game)
    {
      for (auto &action : m_enter_actions)
      {
        action(t_game);
      }
    }

    sf::Vector2u dimensions_in_pixels() const
    {
      return sf::Vector2u(m_tile_size.x * m_map_size.x, m_tile_size.y * m_map_size.y);
    }


    bool load(sf::Vector2u t_tile_size, const std::vector<int> &tiles, const unsigned int width, const unsigned int height)
    {
      // resize the vertex array to fit the level size
      m_vertices.setPrimitiveType(sf::Quads);
      m_vertices.resize(width * height * 4);
      m_tile_data.reserve(width * height);
      m_map_size = sf::Vector2u(width, height);
      m_tile_size = t_tile_size;

      // populate the vertex array, with one quad per tile
      for (unsigned int i = 0; i < width; ++i)
      {
        for (unsigned int j = 0; j < height; ++j)
        {
          // get the current tile number
          auto tileNumber = tiles[i + j * width];


          auto tilePropsFunc = [tileNumber, this](){
            auto defaults_itr = m_map_defaults.find(tileNumber);
            if (defaults_itr != m_map_defaults.end()) {
              return defaults_itr->second;
            } else {
              return Tile_Properties();
            }
          };

          m_tile_data.emplace_back(i, j, tilePropsFunc(), sf::FloatRect(i * t_tile_size.x, j * t_tile_size.y, t_tile_size.x, t_tile_size.y));

          // find its position in the tileset texture
          const auto tu = tileNumber % (m_tileset.get().getSize().x / t_tile_size.x);
          const auto tv = tileNumber / (m_tileset.get().getSize().x / t_tile_size.x);

          // get a pointer to the current tile's quad
          auto quad = &m_vertices[(i + j * width) * 4];


          // define its 4 corners
          quad[0].position = sf::Vector2f(i * t_tile_size.x, j * t_tile_size.y);
          quad[1].position = sf::Vector2f((i + 1) * t_tile_size.x, j * t_tile_size.y);
          quad[2].position = sf::Vector2f((i + 1) * t_tile_size.x, (j + 1) * t_tile_size.y);
          quad[3].position = sf::Vector2f(i * t_tile_size.x, (j + 1) * t_tile_size.y);


          // define its 4 texture coordinates
          quad[0].texCoords = sf::Vector2f(tu * t_tile_size.x, tv * t_tile_size.y);
          quad[1].texCoords = sf::Vector2f((tu + 1) * t_tile_size.x, tv * t_tile_size.y);
          quad[2].texCoords = sf::Vector2f((tu + 1) * t_tile_size.x, (tv + 1) * t_tile_size.y);
          quad[3].texCoords = sf::Vector2f(tu * t_tile_size.x, (tv + 1) * t_tile_size.y);
        }
      }

      return true;
    }

    void add_object(const Object &t_o)
    {
      m_objects.push_back(t_o);
    }

    bool test_move(const sf::Sprite &t_s, const sf::Vector2f &distance) const
    {
      auto newBoundingBox = sf::Transform().translate(distance).transformRect(t_s.getGlobalBounds());
      newBoundingBox = sf::FloatRect(newBoundingBox.left + .05f, newBoundingBox.top + .05f, newBoundingBox.width - .10f, newBoundingBox.height - .10f);

      for (const auto &data : m_tile_data)
      {
        if (!data.properties.passable && data.bounds.intersects(newBoundingBox))
        {
          return false;
        }
      }

      return true;
    }

    sf::Vector2f adjust_move(const sf::Sprite &t_s, const sf::Vector2f &distance) const
    {
      if (test_move(t_s, distance)) {
        return distance;
      }

      const auto xOnly = sf::Vector2f(distance.x, 0);
      if (test_move(t_s, xOnly)) {
        return xOnly;
      }

      const auto yOnly = sf::Vector2f(0, distance.y);
      if (test_move(t_s, yOnly)) {
        return yOnly;
      }

      return sf::Vector2f(0,0);
    }

    void do_move(const float t_time, sf::Sprite &t_s, const sf::Vector2f &distance)
    {
      auto bounds = t_s.getGlobalBounds();

      auto center = sf::Vector2f(bounds.left + bounds.width/2, bounds.top + bounds.height/2);
      auto endCenter = center + distance;

      auto movementBounds = sf::FloatRect(std::min(center.x, endCenter.x)-1, std::min(center.y, endCenter.y)-1, distance.x+2, distance.y+2);

      auto segment = Line_Segment(center, endCenter);

      std::vector<std::tuple<std::reference_wrapper<Tile_Data>, Line_Segment, float>> segments;
      for (auto &data : m_tile_data)
      {
        if (data.bounds.intersects(movementBounds))
        {
          // this is a potential box that we've passed through
          if (auto passedSegment = segment.clipTo(data.bounds))
          {
            // it's a valid segment
            segments.push_back(std::make_tuple(std::ref(data), passedSegment, passedSegment.distance_to_p1(center)));
          }
        }
      }

      std::sort(segments.begin(), segments.end(),
          [](const std::tuple<std::reference_wrapper<Tile_Data>, Line_Segment, float> &t_lhs,
             const std::tuple<std::reference_wrapper<Tile_Data>, Line_Segment, float> &t_rhs)
          {
            return std::get<2>(t_lhs) < std::get<2>(t_rhs);
          }
        );

      auto total_length = segment.length();


      auto total = 0.0;
      // segments should now contain a sorted list of tiles that this movement passes through
      for (auto &cur_segment : segments)
      {
        auto length = std::get<1>(cur_segment).length();
        auto percent = total_length==0?1:(length / total_length);
        total += percent;

        std::get<0>(cur_segment).get().properties.do_movement_action(t_time * percent, length);
      }

      assert(total >= 0.999);
      assert(total <= 1.001);
    }

    void update(const float t_game_time, const float t_simulation_time)
    {
      for (auto &obj : m_objects)
      {
        obj.update(t_game_time, t_simulation_time);
      }
    }

  private:

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
      // apply the transform
      states.transform *= getTransform();

      // apply the tileset texture
      states.texture = &m_tileset.get();

      // draw the vertex array
      target.draw(m_vertices, states);

      for (auto &obj : m_objects)
      {
        target.draw(obj, states);
      }
    }

    sf::VertexArray m_vertices;
    std::reference_wrapper<const sf::Texture> m_tileset;
    std::vector<Tile_Data> m_tile_data;
    std::map<int, Tile_Properties> m_map_defaults;
    std::vector<Object> m_objects;
    std::vector<std::function<void (Game &)>> m_enter_actions;
    sf::Vector2u m_map_size;
    sf::Vector2u m_tile_size;
};

class Game : public sf::Drawable
{
  public:
    Game()
      : m_map(m_maps.end())
    {
    }


    const sf::Texture &get_texture(const std::string &t_filename) const
    {
      auto texture_itr = m_textures.find(t_filename);
      if (texture_itr != m_textures.end())
      {
        return texture_itr->second;
      } else {
        sf::Texture texture;
        if (!texture.loadFromFile(t_filename))
        {
          throw std::runtime_error("Unable to load texture: " + t_filename);
        }
        auto itr = m_textures.emplace(t_filename, std::move(texture));
        return itr.first->second;
      }
    }

    void teleport_to(const float x, const float y)
    {
      m_avatar.setPosition(x, y);
    }

    void set_avatar(const sf::Sprite &t_avatar)
    {
      m_avatar = t_avatar;
    }

    void add_map(const std::string &t_name, const Tile_Map &t_map)
    {
      auto itr = m_maps.emplace(t_name, t_map);
      if (!itr.second) throw std::runtime_error("Map '" + t_name + "' already exists");
    }

    void add_start_action(const std::function<void (Game &)> &t_action)
    {
      m_start_actions.push_back(t_action);
    }

    const sf::Font &get_font(const std::string &t_filename) const
    {
      auto font_itr = m_fonts.find(t_filename);
      if (font_itr != m_fonts.end())
      {
        return font_itr->second;
      } else {
        sf::Font font;
        if (!font.loadFromFile(t_filename))
        {
          throw std::runtime_error("Unable to load font: " + t_filename);
        }
        auto itr = m_fonts.emplace(t_filename, std::move(font));
        return itr.first->second;
      }
    }

    void add_queued_action(const std::function<void (Game &)> &t_action)
    {
      m_game_events.emplace_back(new Queued_Action([this, t_action](){ t_action(*this); }));
    }

    void show_message_box(const sf::String &t_msg)
    {
      m_game_events.emplace_back(new Message_Box(t_msg, get_font("FreeMonoBold.ttf"), 20, sf::Color(0,0,255,255), sf::Color(0,0,0,128), sf::Color(255,255,255,200), 3));
    }

    bool has_pending_events() const
    {
      return !m_game_events.empty();
    }

    Game_Event &get_current_event() const
    {
      if (m_game_events.empty())
      {
        throw std::runtime_error("No pending event!");
      }

      return *m_game_events.front();
    }

    void update(const float t_game_time, const float t_simulation_time)
    {
      float simulation_time = t_simulation_time;

      if (has_pending_events())
      {
        if (m_game_events.front()->is_done())
        {
          m_game_events.pop_front();
        } else {
          simulation_time = 0; // pause simulation during game event
        }
      }

      if (m_map != m_maps.end())
      {
        auto &map = m_map->second;
        auto distance = map.adjust_move(m_avatar, Game::get_input_direction_vector() * 20.0f * simulation_time);
        map.do_move(simulation_time, m_avatar, distance);
        map.update(t_game_time, simulation_time);
        m_avatar.move(distance);
      }

      if (!m_game_events.empty())
      {
        m_game_events.front()->update(t_game_time, t_simulation_time);
      }
    }

    static sf::Vector2f get_input_direction_vector()
    {
      sf::Vector2f velocity(0,0);

      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
      {
        velocity += sf::Vector2f(-1, 0);
      }
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
      {
        velocity += sf::Vector2f(1, 0);
      }
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
      {
        velocity += sf::Vector2f(0, -1);
      }
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
      {
        velocity += sf::Vector2f(0, 1);
      }

      velocity += sf::Vector2f(sf::Joystick::getAxisPosition(2, sf::Joystick::Axis::X)/100, sf::Joystick::getAxisPosition(2, sf::Joystick::Axis::Y)/100);

      if (velocity.x >  1.0) velocity.x =  1.0;
      if (velocity.x < -1.0) velocity.x = -1.0;
      if (velocity.y >  1.0) velocity.y =  1.0;
      if (velocity.y < -1.0) velocity.y = -1.0;

      return velocity;
    }

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
      if (m_map != m_maps.end())
      {
        target.draw(m_map->second, states);
      }

      target.draw(m_avatar, states);
    }


    sf::Vector2f get_avatar_position() const
    {
      return m_avatar.getPosition();
    }

    void enter_map(const std::string &t_name)
    {
      m_map = m_maps.find(t_name);

      if (m_map != m_maps.end())
      {
        m_map->second.enter(*this);
      }
    }

    bool has_current_map() const
    {
      return m_map != m_maps.end();
    }

    const Tile_Map &get_current_map() const
    {
      if (has_current_map())
      {
        return m_map->second;
      } else {
        throw std::runtime_error("No currently selected map");
      }
    }

    void start()
    {
      for (auto &action : m_start_actions)
      {
        action(*this);
      }
    }

  private:

    mutable std::map<std::string, sf::Texture> m_textures;
    mutable std::map<std::string, sf::Font> m_fonts;

    std::deque<std::unique_ptr<Game_Event>> m_game_events;
    std::map<std::string, Tile_Map> m_maps;

    sf::Sprite m_avatar;
    std::map<std::string, Tile_Map>::iterator m_map;
    std::vector<std::function<void (Game &)>> m_start_actions;
};

Game build_game()
{
  Game game;
  // define the level with an array of tile indices
  const std::vector<int> level {
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
  };

  // create the tilemap from the level definition
  Tile_Map map(game.get_texture("tileset.png"), sf::Vector2u(32, 32), level, 32, 14, {{1, Tile_Properties(false)}});

  Object candle(game.get_texture("candle.png"), 32, 32, 3);
  candle.setPosition(100,200);
  map.add_object(candle);

  Object candle2(game.get_texture("candle.png"), 32, 32, 3);
  candle2.setPosition(150,200);
  map.add_object(candle2);

  map.add_enter_action(
      [](Game &t_game){
        t_game.teleport_to(200, 200);
        t_game.show_message_box("Welcome to 'map.'");
      }
    );

  game.add_map("map", map);
  sf::Sprite m_avatar(game.get_texture("sprite.png"));
  game.set_avatar(m_avatar);

  game.add_start_action(
      [](Game &t_game) {
        t_game.show_message_box("Welcome to the Game!");
        t_game.add_queued_action([](Game &t_game) {
          t_game.enter_map("map");
          });
      }
    );

  return game;
}

int main()
{
  // create the window
  sf::RenderWindow window(sf::VideoMode(512, 400), "Tilemap");


  auto game = build_game();

  auto start_time = std::chrono::steady_clock::now();

  auto last_frame = std::chrono::steady_clock::now();
  uint64_t frame_count = 0;

  sf::View fixed = window.getView();

  game.start();

  // run the main loop
  while (window.isOpen())
  {
    ++frame_count;
    auto cur_frame = std::chrono::steady_clock::now();
    auto time_elapsed = std::chrono::duration_cast<std::chrono::duration<float>>(cur_frame - last_frame).count();
    auto game_time =  std::chrono::duration_cast<std::chrono::duration<float>>(cur_frame - start_time).count();

    if (frame_count % 100 == 0)
    {
      std::cout << 1/time_elapsed << "fps avg fps: " << frame_count / game_time << '\n';
    }

    last_frame = cur_frame;
    // handle events
    sf::Event event;
    while (window.pollEvent(event))
    {
      if(event.type == sf::Event::Closed) {
        window.close();
      }

      if (event.type == sf::Event::Resized)
      {
        // update the view to the new size of the window
        window.setSize(sf::Vector2u(event.size.width, event.size.height));
        fixed = sf::View(sf::FloatRect(0,0,event.size.width, event.size.height));
      }
    }

    game.update(game_time, time_elapsed);

    const auto window_size = window.getSize();
    sf::View mainView(game.get_avatar_position(), sf::Vector2f(window_size));
    window.setView(mainView);

    window.clear();

    // main frame
    window.draw(game);

    if (game.has_current_map())
    {
      // mini view
      const auto dimensions = sf::Vector2f(game.get_current_map().dimensions_in_pixels());
      sf::View miniView(sf::FloatRect(sf::Vector2f(0,0), dimensions));
      miniView.setViewport(sf::FloatRect(0.75f, 0, 0.25f, (.25f * window_size.x) * (dimensions.y / dimensions.x) / window_size.y ));
      window.setView(miniView);
      window.draw(game);
    }

    // fixed overlays
    window.setView(fixed);

    if (game.has_pending_events())
    {
      window.draw(game.get_current_event());
    }

    window.display();
  }
}


