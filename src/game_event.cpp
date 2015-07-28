
#include "game_event.hpp"
#include "game.hpp"
#include "map.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <chrono>
#include <functional>
#include <deque>
#include <memory>
#include <cassert>
#include <cmath>

namespace spiced {
  Queued_Action::Queued_Action(std::function<void(const float, const float, Game &)> t_action)
    : m_action(std::move(t_action))
  { }

  void Queued_Action::update(const float t_game_time, const float t_simulation_time, Game &t_game)
  {
    m_action(t_game_time, t_simulation_time, t_game);
    m_done = true;
  }

  bool Queued_Action::is_done() const
  {
    return m_done;
  }



  Message_Box::Message_Box(sf::String t_string, sf::Font t_font, int t_font_size,
    sf::Color t_font_color, sf::Color t_fill_color, sf::Color t_outline_color, float t_outlineThickness)
    : Game_Event(),
    m_string(std::move(t_string)), m_font(std::move(t_font)), m_font_color(std::move(t_font_color)),
    m_fill_color(std::move(t_fill_color)), m_outline_color(std::move(t_outline_color)),
    m_outline_thickness(t_outlineThickness),
    m_text(t_string, m_font, t_font_size)
  {
    setPosition(10, 10);
    m_text.setColor(m_font_color);
  }


  void Message_Box::update(const float t_game_time, const float /*t_simulation_time*/, Game &/*t_game*/)
  {
    if (m_start_time == 0) m_start_time = t_game_time;

    if (t_game_time - m_start_time >= .5 && sf::Keyboard::isKeyPressed(sf::Keyboard::Return))
    {
      m_is_done = true;
    }
  }

  bool Message_Box::is_done() const
  {
    return m_is_done;
  }

  void Message_Box::draw(sf::RenderTarget& target, sf::RenderStates states) const
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



  Selection_Menu::Selection_Menu(sf::Font t_font, int t_font_size,
    sf::Color t_font_color, sf::Color t_selected_font_color, sf::Color t_fill_color, sf::Color t_outline_color, float t_outlineThickness,
    std::vector<Game_Action> t_actions, const size_t t_selection)
    : Game_Event(),
    m_font(std::move(t_font)), m_font_color(std::move(t_font_color)),
    m_selected_color(std::move(t_selected_font_color)),
    m_selected_cur_color(m_selected_color),
    m_fill_color(std::move(t_fill_color)), m_outline_color(std::move(t_outline_color)),
    m_outline_thickness(t_outlineThickness), m_actions(std::move(t_actions)),
    m_current_item(t_selection)
  {
    setPosition(10, 10);

    auto pos = 0.0f;

    for (const auto &action : m_actions)
    {
      m_texts.push_back([t_font_size, t_font_color, &action, &pos, this]()
      {
        sf::Text txt(action.description, m_font, t_font_size);
        txt.setColor(t_font_color);
        txt.setPosition(15, pos);
        pos += t_font_size*1.1f;
        return txt;
      }()
        );
    }

  }

  void Selection_Menu::update(const float t_game_time, const float t_simulation_time, Game &t_game)
  {
    if (m_start_time == 0) m_start_time = t_game_time;

    if (t_game_time - m_start_time >= .5 && sf::Keyboard::isKeyPressed(sf::Keyboard::Return))
    {
      m_actions[m_current_item].action(t_game_time, t_simulation_time, t_game);
      m_is_done = true;
    }

    if (std::remainder(t_game_time, .5f) > 0)
    {
      m_selected_cur_color = m_selected_color;
    }
    else {
      m_selected_cur_color = m_font_color;
    }

    const auto direction = [](const sf::Vector2f &inp) {
      if (inp.y == 0) {
        return 0;
      }
      else if (inp.y > 0) {
        return 1;
      }
      else {
        return -1;
      }
    }(Game::get_input_direction_vector());

    const auto new_item = [&]() {
      if (m_last_direction != direction)
      {
        if (m_current_item == 0 && direction == -1) {
          return m_actions.size() - 1;
        }
        else if (m_current_item == (m_actions.size() - 1) && direction == 1) {
          return size_t(0);
        }
        else {
          return m_current_item += direction;
        }
      }
      else {
        return m_current_item;
      }
    }();

    m_current_item = new_item;
    m_last_direction = direction;
  }

  bool Selection_Menu::is_done() const
  {
    return m_is_done;
  }

  void Selection_Menu::draw(sf::RenderTarget& target, sf::RenderStates states) const
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

    auto item = 0u;
    for (auto txt : m_texts)
    {
      if (m_current_item == item)
      {
        txt.setColor(m_selected_cur_color);
      }
      else {
        txt.setColor(m_font_color);
      }
      target.draw(txt, states);
      ++item;
    }

  }






  Object_Interaction_Menu::Object_Interaction_Menu(Object &t_obj, sf::Font t_font, int t_font_size,
    sf::Color t_font_color, sf::Color t_selected_font_color, sf::Color t_fill_color, sf::Color t_outline_color, float t_outlineThickness,
    const std::vector<Object_Action> &t_actions)
    : Selection_Menu(std::move(t_font), t_font_size, std::move(t_font_color), std::move(t_selected_font_color), std::move(t_fill_color), std::move(t_outline_color),
      t_outlineThickness,
      [](const std::vector<Object_Action> &t_act, Object &t_o) {
        std::vector<Game_Action> res;
        for (auto &act : t_act) {
          auto func = act.action;
          using namespace std::placeholders;
          res.emplace_back(act.description, std::bind(act.action, _1, _2, _3, std::ref(t_o)));
        }

        return res;
      }(t_actions, t_obj), 0)
  {
  }

}

