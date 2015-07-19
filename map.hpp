#ifndef GAME_ENGINE_MAP_HPP
#define GAME_ENGINE_MAP_HPP

#include <SFML/Graphics.hpp>
#include <functional>
#include <cassert>
#include <iostream>

class Game;

class Object;

struct Frame
{
  Frame(const int t_tileid, const int t_duration)
    : tileid(t_tileid), duration(t_duration)
  {
  }

  int tileid;
  int duration;
};

typedef std::vector<Frame> Animation;

struct Tileset
{

  Tileset(std::reference_wrapper<const sf::Texture> t_texture, const int t_first_gid, const int t_tile_width, const int t_tile_height,
      std::map<int, Animation> t_anim)
    : texture(std::move(t_texture)), first_gid(t_first_gid), tile_width(t_tile_width), tile_height(t_tile_height),
      anim(std::move(t_anim))
  {

  }

  int min_gid() const {
    return first_gid;
  }

  int max_gid() const {
    return first_gid + (texture.get().getSize().x / tile_width) * (texture.get().getSize().y / tile_height) - 1;
  }

  sf::IntRect get_rect(const int gid, const float t_game_time) const
  {
    const int frame_gid = [&](){
      auto a = anim.find(gid);
      if (a == anim.end())
      {
        return gid;
      } else {
        // calculate frame
        const auto runtime = std::accumulate(a->second.begin(), a->second.end(), 0, 
            [](int sum, const Frame &f) {
              return sum + f.duration;
            }) / 1000.0f;


        const auto time_into_current_loop = std::fabs(std::fmod(t_game_time, runtime));


        return [&](){
          auto time_left = time_into_current_loop * 1000;
          for (const Frame &f : a->second) {
            time_left -= f.duration;
            if (time_left <= 0) {
              return f.tileid;
            }
          }
          assert(false);
          return a->second.back().tileid;
        }();

      }
    }();

    auto loc = location(frame_gid);
    return sf::IntRect(loc.x * tile_width, loc.y * tile_height, tile_width, tile_height);
  }

  sf::Vector2i location(const int gid) const
  {
    const auto num_horz_tiles = texture.get().getSize().x / tile_width;
    const auto id = gid - first_gid;

    return sf::Vector2i(id % num_horz_tiles, id / num_horz_tiles);
  }

  sf::VertexArray vertices(const int gid, const int i, const int j) const
  {
    const auto loc = location(gid);

    const auto tu = loc.x;
    const auto tv = loc.y;

    sf::VertexArray verts(sf::Quads);

    verts.append(sf::Vertex(
          sf::Vector2f(i * tile_width, j * tile_height),
          sf::Vector2f(tu * tile_width, tv * tile_height)));

    verts.append(sf::Vertex(
          sf::Vector2f((i + 1) * tile_width, j * tile_height),
          sf::Vector2f((tu + 1) * tile_width, tv * tile_height)));

    verts.append(sf::Vertex(
          sf::Vector2f((i + 1) * tile_width, (j + 1) * tile_height),
          sf::Vector2f((tu + 1) * tile_width, (tv + 1) * tile_height)));

    verts.append(sf::Vertex(
          sf::Vector2f(i * tile_width, (j + 1) * tile_height),
          sf::Vector2f(tu * tile_width, (tv + 1) * tile_height)));

    return verts;
  }

  std::reference_wrapper<const sf::Texture> texture;
  int first_gid;
  int tile_width;
  int tile_height;
  std::map<int, Animation> anim;
};

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
    Object(std::string t_name, Tileset t_tileset,
               const int t_tile_id,
               const bool t_visible,
               std::function<void (const float, const float, Game &, Object &, sf::Sprite &)> t_collision_action,
               std::function<std::vector<Object_Action> (const float, const float, Game &, Object &)> t_action_generator);

    virtual ~Object() = default;

    void update(const float t_game_time, const float /*t_simulation_time*/, Game &t_game);

    void set_position(const float x, const float y);

    std::vector<Object_Action> get_actions(const float t_game_time, const float t_simulation_time, Game &t_game);

    void do_collision(const float t_game_time, const float t_simulation_time, Game &t_game, sf::Sprite &t_collided_with);

    void set_collision_action(std::function<void (const float, const float, Game &, Object &, sf::Sprite &)> t_collision_action);
    void set_action_generator(std::function<std::vector<Object_Action> (const float, const float, Game &, Object &)> t_action_generator);

    std::string name() const;

  private:
    std::string m_name;
    Tileset m_tileset;
    int m_tile_id;
    bool m_visible;
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

struct Tile_Defaults
{
  Tile_Defaults(const int t_tile_id, Tile_Properties t_props)
    : tile_id(t_tile_id), props(std::move(t_props))
  {
  }

  int tile_id;
  Tile_Properties props;
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

struct Layer
{
  Layer(std::vector<int> t_data, const bool t_visible)
    : data(std::move(t_data)), visible(t_visible)
  {
  }

  std::vector<int> data;
  bool visible;
};

class Tile_Map : public sf::Drawable, public sf::Transformable
{
  public:

    Tile_Map(Game &t_game, const std::string &t_file_path, std::vector<Tile_Defaults> t_map_defaults);

    virtual ~Tile_Map() = default;

    void add_enter_action(const std::function<void (Game &)> t_action);

    void enter(Game &t_game);

    sf::Vector2u dimensions_in_pixels() const;

    void load(sf::Vector2u t_tile_size, const std::vector<Layer> &layers, const unsigned int width, const unsigned int height);

    void add_object(const Object &t_o);

    void set_collision_action(const std::string &t_obj_name,
        std::function<void (const float, const float, Game &, Object &, sf::Sprite &)> t_collision_action);

    void set_action_generator(const std::string &t_obj_name,
        std::function<std::vector<Object_Action> (const float, const float, Game &, Object &)> t_action_generator);


    static sf::FloatRect get_bounding_box(const sf::Sprite &t_s, const sf::Vector2f &t_distance);

    bool test_move(const sf::Sprite &t_s, const sf::Vector2f &distance) const;

    std::vector<std::reference_wrapper<Object>> get_collisions(const sf::Sprite &t_s, const sf::Vector2f &t_distance);

    sf::Vector2f adjust_move(const sf::Sprite &t_s, const sf::Vector2f &distance) const;

    void do_move(const float t_time, sf::Sprite &t_s, const sf::Vector2f &distance);

    void update(const float t_game_time, const float t_simulation_time, Game &t_game);

    sf::Vector2u tile_size() const;


  private:

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

    static std::map<int, Tile_Properties> to_map(std::vector<Tile_Defaults> &&t_vec);

    std::vector<sf::VertexArray> m_layers;
    std::vector<Tileset> m_tilesets;
    std::vector<Tile_Data> m_tile_data;
    std::map<int, Tile_Properties> m_map_defaults;
    std::vector<Object> m_objects;
    std::vector<std::function<void (Game &)>> m_enter_actions;
    sf::Vector2u m_map_size;
    sf::Vector2u m_tile_size;
};

#endif

