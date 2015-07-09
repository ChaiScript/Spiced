#include "map.hpp"
#include "game.hpp"
#include "SimpleJSON/json.hpp"

#include <SFML/Graphics.hpp>
#include <functional>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>

#include <iostream>


Object::Object(std::string t_name, Tileset t_tileset,
               const int t_tile_id,
               const bool t_visible,
               std::function<void (const float, const float, Game &, Object &, sf::Sprite &)> t_collision_action,
               std::function<std::vector<Object_Action> (const float, const float, Game &, Object &)> t_action_generator)
  : m_name(std::move(t_name)),
    m_tileset(std::move(t_tileset)),
    m_tile_id(t_tile_id),
    m_visible(t_visible),
    m_collision_action(std::move(t_collision_action)),
    m_action_generator(std::move(t_action_generator))
{
  setTexture(m_tileset.texture.get());
  setTextureRect(m_tileset.get_rect(m_tile_id, 0));
}


void Object::update(const float t_game_time, const float /*t_simulation_time*/, Game &t_game)
{
  setTextureRect(m_tileset.get_rect(m_tile_id, t_game_time));

  if (!m_visible) {
    if (t_game.show_invisible()) {
      setColor(sf::Color(255,255,255,128));
    } else {
      setColor(sf::Color(255,255,255,0));
    }
  }
}

std::vector<Object_Action> Object::get_actions(const float t_game_time, const float t_simulation_time, Game &t_game)
{
  return m_action_generator(t_game_time, t_simulation_time, t_game, *this);
}

void Object::set_collision_action(std::function<void (const float, const float, Game &, Object &, sf::Sprite &)> t_collision_action)
{
  m_collision_action = std::move(t_collision_action);
}

void Object::set_action_generator(std::function<std::vector<Object_Action> (const float, const float, Game &, Object &)> t_action_generator)
{
  m_action_generator = std::move(t_action_generator);
}


void Object::do_collision(const float t_game_time, const float t_simulation_time, Game &t_game, sf::Sprite &t_collided_with)
{
  if (m_collision_action)
  {
    m_collision_action(t_game_time, t_simulation_time, t_game, *this, t_collided_with);
  }
}

void Object::set_position(const float x, const float y)
{
  setPosition(x,y);
}

std::string Object::name() const
{
  return m_name;
}

Tile_Properties::Tile_Properties(bool t_passable,
    std::function<void (float, float)> t_movement_action)
  : passable(t_passable), movement_action(std::move(t_movement_action))
{
}

void Tile_Properties::do_movement_action(const float t_game_time, const float t_simulation_time)
{
  if (movement_action)
  {
    movement_action(t_game_time, t_simulation_time);
  }
}


Line_Segment::Line_Segment(sf::Vector2f t_p1, sf::Vector2f t_p2)
  : p1(std::move(t_p1)), p2(std::move(t_p2)), valid(true)
{
}

Line_Segment::Line_Segment()
  : valid(false)
{
}


float Line_Segment::x(const float t_y) const
{
  return ((t_y-p1.y)*(p2.x-p1.x))/(p2.y - p1.y) + p1.x;
}

float Line_Segment::y(const float t_x) const
{
  return ((p2.y-p1.y)*(t_x - p1.x))/(p2.x - p1.x) + p1.y;
}

Line_Segment::operator bool() const
{
  return valid;
}

float Line_Segment::distance_to_p1(const sf::Vector2f &t_point) const
{
  return std::hypot(t_point.x - p1.x, t_point.y - p1.y);
}

float Line_Segment::length() const 
{
  if (p1 == p2) return 0;

  return std::hypot(p2.x - p1.x, p2.y - p1.y);
}

sf::FloatRect Line_Segment::boundingRect() const
{
  const auto x1 = std::min(p1.x, p2.x);
  const auto y1 = std::min(p1.y, p2.y);

  const auto width = std::max(p1.x, p2.x) - x1;
  const auto height = std::max(p1.y, p2.y) - y1;

  return sf::FloatRect(x1, y1, width, height);
}

Line_Segment Line_Segment::clipTo(const sf::FloatRect &t_rect) const
{
  auto contains_p1 = t_rect.contains(p1);
  auto contains_p2 = t_rect.contains(p2);

  if (contains_p1 && contains_p2)
  {
    return *this;
  }

  const auto bounds = boundingRect();

  auto validate = [&t_rect, &bounds](const float t_x, const float t_y) {
    return    t_rect.left <= t_x && t_rect.top <= t_y && (t_rect.left + t_rect.width) >= t_x && (t_rect.top + t_rect.height) >= t_y
           && bounds.left <= t_x && bounds.top <= t_y && (bounds.left + bounds.width) >= t_x && (bounds.top + bounds.height) >= t_y;
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


Tile_Data::Tile_Data(int t_x, int t_y, Tile_Properties t_props, sf::FloatRect t_bounds)
  : x(t_x), y(t_y), properties(std::move(t_props)), bounds(std::move(t_bounds))
{
}

std::map<int, Tile_Properties> Tile_Map::to_map(std::vector<Tile_Defaults> &&t_vec)
{
  std::map<int, Tile_Properties> retmap;
  for (auto &d : t_vec)
  {
    retmap.emplace(std::move(d.tile_id), std::move(d.props));
  }
  return retmap;
}


Tile_Map::Tile_Map(Game &t_game, const std::string &t_file_path, std::vector<Tile_Defaults> t_map_defaults)
  : m_map_defaults(to_map(std::move(t_map_defaults)))
{
  std::ifstream ifs(t_file_path);
  std::stringstream buff;
  buff << ifs.rdbuf();
  auto json = json::JSON::Load(buff.str());

  const auto tile_size = sf::Vector2u(json.at("tilewidth").ToInt(), json.at("tileheight").ToInt());
  const auto map_width = json.at("width").ToInt();
  const auto map_height = json.at("height").ToInt();

  const auto parent_path = t_file_path.substr(0, t_file_path.rfind('/'));

  std::map<int, std::map<std::string, std::string>> tile_properties;

  for (const auto &tileset : json.at("tilesets").ArrayRange())
  {
    const auto first_gid = tileset.at("firstgid").ToInt();


    if (tileset.hasKey("tileproperties")) {
      for (const auto &tile : tileset.at("tileproperties").ObjectRange()) {
        auto id = std::stoi(tile.first) + first_gid;

        //std::map<std::string, std::string> props;
        for (const auto &property : tile.second.ObjectRange()) {
          const std::string &prop_name = property.first;
          const std::string value = property.second.ToString();
          if (prop_name == "passable" && value == "false") {
            m_map_defaults[id].passable = false;
          } else {
            std::cerr << "Unhandled property: " << prop_name << ": " << value << '\n';
          }
          //props.emplace(property.first, property.second.ToString());
        }
        //tile_properties.emplace(id, std::move(props));
      }
    }

    std::map<int, Animation> animations;
    if (tileset.hasKey("tiles")) {
      for (const auto &tile : tileset.at("tiles").ObjectRange()) {
        auto id = std::stoi(tile.first) + first_gid;

        if (tile.second.hasKey("animation")) {
          Animation anim;
          for (const auto &frame : tile.second.at("animation").ArrayRange()) {
            const auto duration = frame.at("duration").ToInt();
            const auto tileid = frame.at("tileid").ToInt() + first_gid;
            anim.emplace_back(tileid, duration);
          }

          animations.emplace(std::make_pair(id, anim));
        }
      }
    }

    m_tilesets.emplace_back(t_game.get_texture(parent_path + '/' + tileset.at("image").ToString()),
        first_gid,
        tileset.at("tilewidth").ToInt(), tileset.at("tileheight").ToInt(),
        std::move(animations));

  }

  std::vector<Layer> layers;

  for (const auto &layer : json.at("layers").ArrayRange()) {


    if (layer.at("type").ToString() == "tilelayer") {
      bool visible = layer.at("visible").ToBool();

      if (layer.hasKey("properties")) {
        for (const auto &property : layer.at("properties").ObjectRange()) {
          const std::string &prop_name = property.first;
          const std::string value = property.second.ToString();
          if (prop_name == "visible" && value == "false") {
            visible = false;
          } else {
            std::cerr << "Unhandled property: " << prop_name << ": " << value << '\n';
          }
          //props.emplace(property.first, property.second.ToString());
        }
      }

      std::vector<int> data;
      for (const auto &val : layer.at("data").ArrayRange()) {
        data.push_back(val.ToInt());
      }
      layers.emplace_back(std::move(data), visible);
    } else if (layer.at("type").ToString() == "objectgroup") {
      for (const auto &obj : layer.at("objects").ArrayRange()) {
        const auto gid = obj.at("gid").ToInt();
        const auto name = obj.at("name").ToString();

        const auto tileset = std::find_if(m_tilesets.begin(), m_tilesets.end(), 
            [gid](const Tileset &t_tileset) {
              return gid >= t_tileset.min_gid() && gid <= t_tileset.max_gid();
            });
        assert(tileset != m_tilesets.end());

        bool visible = obj.at("visible").ToBool();

        for (const auto &property : obj.at("properties").ObjectRange()) {
          const std::string &prop_name = property.first;
          const std::string value = property.second.ToString();
          if (prop_name == "visible" && value == "false") {
            visible = false;
          } else {
            std::cerr << "Unhandled property: " << prop_name << ": " << value << '\n';
          }
          //props.emplace(property.first, property.second.ToString());
        }

        const auto get_float = [&](const std::string &t_name){
          auto v = obj.at(t_name);
          if (v.JSONType() == json::JSON::Class::Floating) {
            return v.ToFloat();
          } else {
            return double(v.ToInt());
          }
        };

        const auto x = get_float("x");
        const auto y = get_float("y") - tileset->tile_height;

        std::cout << "Placing object: " << name << "(" << x << ", " << y << ")\n";

        Object gameobj(name, *tileset, gid, visible, {}, {});
        gameobj.set_position(x, y);
        add_object(gameobj);
      }
    }
  }

  load(tile_size, layers, map_width, map_height);
}

void Tile_Map::add_enter_action(const std::function<void (Game &)> t_action)
{
  m_enter_actions.push_back(t_action);
}

void Tile_Map::enter(Game &t_game)
{
  for (auto &action : m_enter_actions)
  {
    action(t_game);
  }
}

sf::Vector2u Tile_Map::dimensions_in_pixels() const
{
  return sf::Vector2u(m_tile_size.x * m_map_size.x, m_tile_size.y * m_map_size.y);
}


void Tile_Map::load(sf::Vector2u t_tile_size, const std::vector<Layer> &layers, const unsigned int width, const unsigned int height)
{
  m_map_size = sf::Vector2u(width, height);
  m_tile_size = t_tile_size;

  for (const auto &layer : layers) 
  {
    for (const auto &tileset : m_tilesets)
    {
      const auto min_tile = tileset.min_gid();
      const auto max_tile = tileset.max_gid();

      sf::VertexArray vertices(sf::Quads);

      // populate the vertex array, with one quad per tile
      for (unsigned int i = 0; i < width; ++i)
      {
        for (unsigned int j = 0; j < height; ++j)
        {
          // get the current tile number
          const auto tileNumber = layer.data[i + j * width];

          if (tileNumber >= min_tile && tileNumber <= max_tile)
          {
            auto tilePropsFunc = [tileNumber, this](){
              auto defaults_itr = m_map_defaults.find(tileNumber);
              if (defaults_itr != m_map_defaults.end()) {
                return defaults_itr->second;
              } else {
                return Tile_Properties();
              }
            };

            m_tile_data.emplace_back(i, j, tilePropsFunc(), sf::FloatRect(i * t_tile_size.x, j * t_tile_size.y, t_tile_size.x, t_tile_size.y));
            const auto tilesetvertices = tileset.vertices(tileNumber, i, j);
            for (size_t index = 0; index < tilesetvertices.getVertexCount(); ++index) 
            {
              vertices.append(tilesetvertices[index]);
              if (!layer.visible) {
                vertices[vertices.getVertexCount() - 1].color = sf::Color(255,255,255,0);
              }
            }

          }
        }
      }

      m_layers.push_back(vertices);
    }
  }
}

void Tile_Map::add_object(const Object &t_o)
{
  m_objects.push_back(t_o);
}

sf::FloatRect Tile_Map::get_bounding_box(const sf::Sprite &t_s, const sf::Vector2f &t_distance)
{
  auto bounding_box = sf::Transform().translate(t_distance).transformRect(t_s.getGlobalBounds());
  bounding_box = sf::FloatRect(bounding_box.left + .05f, bounding_box.top + .05f, bounding_box.width - .10f, bounding_box.height - .10f);
  return bounding_box;
}

bool Tile_Map::test_move(const sf::Sprite &t_s, const sf::Vector2f &distance) const
{
  auto bounding_box = get_bounding_box(t_s, distance);

  for (const auto &data : m_tile_data)
  {
    if (!data.properties.passable && data.bounds.intersects(bounding_box))
    {
      return false;
    }
  }

  for (const auto &object : m_objects)
  {
    if (object.getGlobalBounds().intersects(bounding_box))
    {
      return false;
    }
  }

  return true;
}

void Tile_Map::set_collision_action(const std::string &t_obj_name,
    std::function<void (const float, const float, Game &, Object &, sf::Sprite &)> t_collision_action)
{

  const auto obj = std::find_if(m_objects.begin(), m_objects.end(), [&](const Object &t_obj) { return t_obj.name() == t_obj_name; });
  if (obj == m_objects.end()) throw std::logic_error("Attempt to set collision action on non-existent object: " + t_obj_name);
  obj->set_collision_action(t_collision_action);
}

void Tile_Map::set_action_generator(const std::string &t_obj_name,
    std::function<std::vector<Object_Action> (const float, const float, Game &, Object &)> t_action_generator)
{
  const auto obj = std::find_if(m_objects.begin(), m_objects.end(), [&](const Object &t_obj) { return t_obj.name() == t_obj_name; });
  if (obj == m_objects.end()) throw std::logic_error("Attempt to set collision action on non-existent object: " + t_obj_name);
  obj->set_action_generator(t_action_generator);
}

std::vector<std::reference_wrapper<Object>> Tile_Map::get_collisions(const sf::Sprite &t_s, const sf::Vector2f &t_distance) 
{
  std::vector<std::reference_wrapper<Object>> retval;
  auto bounding_box = get_bounding_box(t_s, t_distance);

  for (auto &object : m_objects)
  {
    if (object.getGlobalBounds().intersects(bounding_box))
    {
      retval.push_back(std::ref(object));
    }
  }

  return retval;
}

sf::Vector2f Tile_Map::adjust_move(const sf::Sprite &t_s, const sf::Vector2f &distance) const
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

void Tile_Map::do_move(const float t_time, sf::Sprite &t_s, const sf::Vector2f &distance)
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

  //assert(total >= 0.999);
  //assert(total <= 1.001);
}

void Tile_Map::update(const float t_game_time, const float t_simulation_time, Game &t_game)
{
  for (auto &obj : m_objects)
  {
    obj.update(t_game_time, t_simulation_time, t_game);
  }
}


void Tile_Map::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  // apply the transform
  states.transform *= getTransform();

  for (size_t i = 0; i < m_layers.size(); ++i)
  {
    auto state = states;
    state.texture = &m_tilesets[i % m_tilesets.size()].texture.get();
    target.draw(m_layers[i], state);
  }

  for (auto &obj : m_objects)
  {
    target.draw(obj, states);
  }
}


sf::Vector2u Tile_Map::tile_size() const
{
  return m_tile_size;
}


