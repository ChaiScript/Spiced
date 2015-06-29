#ifndef GAME_ENGINE_MAP_HPP
#define GAME_ENGINE_MAP_HPP

#include <SFML/Graphics.hpp>
#include <functional>

class Game;

class Object;

struct Game_Action
{
  Game_Action(std::string t_description, std::function<void (const float, const float, Game &)> t_action)
    : description(std::move(t_description)), action(std::move(t_action))
  {
  }

  std::string description;
  std::function<void (const float, const float, Game &)> action;
};


struct Object_Action
{
  Object_Action(std::string t_description, std::function<void (const float, const float, Game &, Object &)> t_action)
    : description(std::move(t_description)), action(std::move(t_action))
  {
  }

  std::string description;
  std::function<void (const float, const float, Game &, Object &)> action;
};

class Object : public sf::Sprite
{
  public:
    Object(std::string t_name, const sf::Texture &t_texture, const int width, const int height, const float fps,
           std::function<void (const float, const float, Game &, Object &, sf::Sprite &)> t_collision_action,
           std::function<std::vector<Object_Action> (const float, const float, Game &, Object &)> t_action_generator);

    virtual ~Object() = default;

    void update(const float t_game_time, const float /*t_simulation_time*/, Game &t_game);

    std::vector<Object_Action> get_actions(const float t_game_time, const float t_simulation_time, Game &t_game);

    void do_collision(const float t_game_time, const float t_simulation_time, Game &t_game, sf::Sprite &t_collided_with);

  private:
    std::string m_name;
    std::reference_wrapper<const sf::Texture> m_texture;
    int m_width;
    int m_height;
    int m_num_frames;
    float m_fps;
    std::function<void (const float, const float, Game &, Object &, sf::Sprite &)> m_collision_action;
    std::function<std::vector<Object_Action> (const float, const float, Game &, Object &)> m_action_generator;
};


struct Tile_Properties
{
  Tile_Properties(bool t_passable = true, 
      std::function<void (float, float)> t_movement_action = std::function<void (float, float)>());

  void do_movement_action(const float t_game_time, const float t_simulation_time);

  bool passable;
  std::function<void (float, float)> movement_action;
};

struct Line_Segment
{
  Line_Segment(sf::Vector2f t_p1, sf::Vector2f t_p2);

  Line_Segment();

  Line_Segment(const Line_Segment &) = default;
  Line_Segment(Line_Segment &&) = default;

  Line_Segment &operator=(const Line_Segment &) = default;
  Line_Segment &operator=(Line_Segment &&) = default;

  float x(const float y) const;

  float y(const float x) const;

  explicit operator bool() const;

  float distance_to_p1(const sf::Vector2f &t_point) const;

  float length() const;

  sf::FloatRect boundingRect() const;

  Line_Segment clipTo(const sf::FloatRect &t_rect) const;

  sf::Vector2f p1;
  sf::Vector2f p2;
  bool valid;
};

struct Tile_Data
{
  Tile_Data(int t_x, int t_y, Tile_Properties t_props, sf::FloatRect t_bounds);

  int x;
  int y;
  Tile_Properties properties;
  sf::FloatRect bounds;
};


class Tile_Map : public sf::Drawable, public sf::Transformable
{
  public:
    Tile_Map(const std::vector<std::reference_wrapper<const sf::Texture>> &t_tilesets,
            const sf::Vector2u &t_tile_size, const std::vector<std::vector<int>> &layers, const unsigned int width, const unsigned int height, std::map<int, Tile_Properties> t_map_defaults);

    Tile_Map(Game &t_game, const std::string &t_file_path, std::map<int, Tile_Properties> t_map_defaults);

    virtual ~Tile_Map() = default;

    void add_enter_action(const std::function<void (Game &)> t_action);

    void enter(Game &t_game);

    sf::Vector2u dimensions_in_pixels() const;

    bool load(sf::Vector2u t_tile_size, const std::vector<std::vector<int>> &layers, const unsigned int width, const unsigned int height);

    void add_object(const Object &t_o);

    static sf::FloatRect get_bounding_box(const sf::Sprite &t_s, const sf::Vector2f &t_distance);

    bool test_move(const sf::Sprite &t_s, const sf::Vector2f &distance) const;

    std::vector<std::reference_wrapper<Object>> get_collisions(const sf::Sprite &t_s, const sf::Vector2f &t_distance);

    sf::Vector2f adjust_move(const sf::Sprite &t_s, const sf::Vector2f &distance) const;

    void do_move(const float t_time, sf::Sprite &t_s, const sf::Vector2f &distance);

    void update(const float t_game_time, const float t_simulation_time, Game &t_game);

  private:

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;


    std::vector<sf::VertexArray> m_layers;
    std::vector<std::reference_wrapper<const sf::Texture>> m_tilesets;
    std::vector<Tile_Data> m_tile_data;
    std::map<int, Tile_Properties> m_map_defaults;
    std::vector<Object> m_objects;
    std::vector<std::function<void (Game &)>> m_enter_actions;
    sf::Vector2u m_map_size;
    sf::Vector2u m_tile_size;
};

#endif

