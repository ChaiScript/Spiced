
#include "game.hpp"
#include "game_event.hpp"
#include "map.hpp"

#include <SFML/Graphics.hpp>
#include <functional>
#include <deque>
#include <memory>
#include <cassert>
#include <cmath>

namespace spiced {
  Game::Game()
    : m_map(m_maps.end()),
    m_rotate(0),
    m_zoom(1)
  {
  }


  const sf::Texture &Game::get_texture(const std::string &t_filename) const
  {
    auto texture_itr = m_textures.find(t_filename);
    if (texture_itr != m_textures.end())
    {
      return texture_itr->second;
    }
    else {
      sf::Texture texture;
      if (!texture.loadFromFile(t_filename))
      {
        throw std::runtime_error("Unable to load texture: " + t_filename);
      }
      auto itr = m_textures.emplace(t_filename, std::move(texture));
      return itr.first->second;
    }
  }

  void Game::teleport_to(const float x, const float y)
  {
    m_avatar.setPosition(x, y);
  }

  void Game::teleport_to_tile(const int t_x, const int t_y)
  {
    if (m_map == m_maps.end()) throw std::logic_error("No map currently defined when attempting teleport_to_tile");

    const auto tile_size = m_map->second.tile_size();
    const auto avatar_size = m_avatar.getTextureRect();

    const auto x = (tile_size.x * t_x) + ((tile_size.x - avatar_size.width) / 2);
    const auto y = (tile_size.y * t_y) + ((tile_size.y - avatar_size.height) / 2);

    teleport_to(float(x), float(y));
  }

  void Game::set_avatar(const sf::Texture &t_avatar)
  {
    m_avatar = sf::Sprite(t_avatar);
  }

  void Game::add_map(const std::string &t_name, const Tile_Map &t_map)
  {
    auto itr = m_maps.emplace(t_name, t_map);
    if (!itr.second) throw std::runtime_error("Map '" + t_name + "' already exists");
  }

  void Game::add_start_action(const std::function<void(Game &)> &t_action)
  {
    m_start_actions.push_back(t_action);
  }

  const sf::Font &Game::get_font(const std::string &t_filename) const
  {
    auto font_itr = m_fonts.find(t_filename);
    if (font_itr != m_fonts.end())
    {
      return font_itr->second;
    }
    else {
      sf::Font font;
      if (!font.loadFromFile(t_filename))
      {
        throw std::runtime_error("Unable to load font: " + t_filename);
      }
      auto itr = m_fonts.emplace(t_filename, std::move(font));
      return itr.first->second;
    }
  }

  void Game::add_queued_action(const std::function<void(const float, const float, Game &)> &t_action)
  {
    m_game_events.emplace_back(new Queued_Action(t_action));
  }

  void Game::show_message_box(const sf::String &t_msg)
  {
    m_game_events.emplace_back(new Message_Box(t_msg, get_font("resources/FreeMonoBold.ttf"), 17, sf::Color(255, 255, 255, 255), sf::Color(0, 0, 0, 128), sf::Color(255, 255, 255, 200), 3, Location::Bottom));
  }

  void Game::show_conversation(const float t_game_time, const float t_simulation_time, Object &t_obj, const Conversation &t_conversation)
  {
    std::vector<Object_Action> actions;
    for (const auto &q : t_conversation.questions)
    {
      if (!q.is_available || q.is_available(t_game_time, t_simulation_time, *this, t_obj))
      {
        actions.emplace_back(q.question,
          [q, t_game_time, t_simulation_time, &t_obj, t_conversation](const float, const float, Game &t_game, Object &obj)
        {
          for (const auto &answer : q.answers) {
            t_game.show_message_box(answer.speaker + ":\n\n" + answer.answer);
          }
          if (q.action) {
            using namespace std::placeholders;
            t_game.add_queued_action(std::bind(q.action, _1, _2, _3, std::ref(obj)));
          }
        }
        );
      }
    }

    // add a default action
    if (actions.empty()) {
      actions.emplace_back("...",
        [](const float, const float, Game &, Object &)
      {}
      );
    }


    m_game_events.emplace_back(new Object_Interaction_Menu(t_obj, get_font("resources/FreeMonoBold.ttf"), 17, sf::Color(255, 255, 255, 255), sf::Color(0, 200, 200, 255), sf::Color(0, 0, 0, 128), sf::Color(255, 255, 255, 200), 3, actions, Location::Bottom));
  }

  void Game::show_object_interaction_menu(const float t_game_time, const float t_simulation_time, Object &t_obj)
  {
    m_game_events.emplace_back(new Object_Interaction_Menu(t_obj, get_font("resources/FreeMonoBold.ttf"), 17, sf::Color(255, 255, 255, 255), sf::Color(0, 200, 200, 255), sf::Color(0, 0, 0, 128), sf::Color(255, 255, 255, 200), 3, t_obj.get_actions(t_game_time, t_simulation_time, *this), Location::Right));
  }

  void Game::show_selection_menu(const float /*t_game_time*/, const float /*t_simulation_time*/, const std::vector<Game_Action> &t_selections, const size_t t_selection)
  {
    m_game_events.emplace_back(new Selection_Menu(get_font("resources/FreeMonoBold.ttf"), 17, sf::Color(255, 255, 255, 255), sf::Color(0, 200, 200, 255), sf::Color(0, 0, 0, 128), sf::Color(255, 255, 255, 200), 3, t_selections, t_selection, Location::Right));
  }

  bool Game::has_pending_events() const
  {
    return !m_game_events.empty();
  }

  Game_Event &Game::get_current_event() const
  {
    if (m_game_events.empty())
    {
      throw std::runtime_error("No pending event!");
    }

    return *m_game_events.front();
  }

  void Game::update(const float t_game_time, const float t_simulation_time)
  {
    float simulation_time = t_simulation_time;

    if (has_pending_events())
    {
      if (m_game_events.front()->is_done())
      {
        m_game_events.pop_front();
      }
      else {
        simulation_time = 0; // pause simulation during game event
      }
    }

    if (m_map != m_maps.end())
    {
      auto &map = m_map->second;
      auto distance = Game::get_input_direction_vector() * 45.0f * simulation_time;
      for (auto &collision : map.get_collisions(m_avatar, distance))
      {
        collision.get().do_collision(t_game_time, simulation_time, *this, m_avatar);
        break; // only handle one collision
      }

      distance = map.adjust_move(m_avatar, distance);
      map.do_move(simulation_time, m_avatar, distance);
      map.update(t_game_time, simulation_time, *this);
      m_avatar.move(distance);
    }

    if (!m_game_events.empty())
    {
      m_game_events.front()->update(t_game_time, t_simulation_time, *this);
    }
  }

  sf::Vector2f Game::get_input_direction_vector()
  {
    sf::Vector2f velocity(0, 0);

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

    velocity += sf::Vector2f(sf::Joystick::getAxisPosition(2, sf::Joystick::Axis::X) / 100, sf::Joystick::getAxisPosition(2, sf::Joystick::Axis::Y) / 100);

    if (velocity.x > 1.0) velocity.x = 1.0;
    if (velocity.x < -1.0) velocity.x = -1.0;
    if (velocity.y > 1.0) velocity.y = 1.0;
    if (velocity.y < -1.0) velocity.y = -1.0;

    return velocity;
  }

  bool Game::show_mini_map() const {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::M)) {
      return true;
    }
    else {
      return false;
    }
  }

  bool Game::show_invisible() const {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::V)) {
      return true;
    }
    else {
      return false;
    }
  }

  void Game::draw(sf::RenderTarget& target, sf::RenderStates states) const
  {

    if (m_map != m_maps.end())
    {
      target.draw(m_map->second, states);
    }

    target.draw(m_avatar, states);
  }


  sf::Vector2f Game::get_avatar_position() const
  {
    return m_avatar.getPosition();
  }

  void Game::enter_map(const std::string &t_name)
  {
    m_map = m_maps.find(t_name);

    if (m_map != m_maps.end())
    {
      m_map->second.enter(*this);
    }
  }

  bool Game::has_current_map() const
  {
    return m_map != m_maps.end();
  }

  const Tile_Map &Game::get_current_map() const
  {
    if (has_current_map())
    {
      return m_map->second;
    }
    else {
      throw std::runtime_error("No currently selected map");
    }
  }

  void Game::start()
  {
    for (auto &action : m_start_actions)
    {
      action(*this);
    }
  }


  void Game::set_value(const std::string &t_name, int t_value)
  {
    m_values[t_name] = t_value;
  }

  int Game::get_value(const std::string &t_name) const
  {
    const auto itr = m_values.find(t_name);
    if (itr != m_values.end())
    {
      return itr->second;
    }
    else {
      return 0;
    }
  }


  void Game::set_flag(const std::string &t_name, bool t_value)
  {
    m_flags[t_name] = t_value;
  }

  bool Game::get_flag(const std::string &t_name) const
  {
    const auto itr = m_flags.find(t_name);
    if (itr != m_flags.end())
    {
      return itr->second;
    }
    else {
      return false;
    }
  }

  void Game::set_rotate(const float t_r)
  {
    m_rotate = t_r;
  }

  void Game::set_zoom(const float t_z)
  {
    m_zoom = t_z;
  }

  float Game::rotate()
  {
    return m_rotate;
  }

  float Game::zoom()
  {
    return m_zoom;
  }

}

