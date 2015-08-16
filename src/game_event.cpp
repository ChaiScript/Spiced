
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
  Queued_Action::Queued_Action(std::function<void(const Game_State &)> t_action)
    : m_action(std::move(t_action))
  { }

  void Queued_Action::update(const Game_State &t_game)
  {
    m_action(t_game);
    m_done = true;
  }

  bool Queued_Action::is_done() const
  {
    return m_done;
  }



  Message_Box::Message_Box(sf::String t_string, sf::Font t_font, int t_font_size,
    sf::Color t_font_color, sf::Color t_fill_color, sf::Color t_outline_color, float t_outlineThickness, Location t_loc,
    const sf::Texture *t_texture)
    : Game_Event(),
      m_string(std::move(t_string)), m_font(std::move(t_font)), m_font_color(std::move(t_font_color)),
      m_fill_color(std::move(t_fill_color)), m_outline_color(std::move(t_outline_color)),
      m_outline_thickness(t_outlineThickness),
      m_text(t_string, m_font, t_font_size),
      m_location(std::move(t_loc)),
      m_portrait(t_texture?new sf::Sprite(*t_texture):nullptr)
  {
    /// \todo un-hardcode this
    if (m_portrait) m_portrait->setScale(2,2);
    m_text.setColor(m_font_color);
  }


  void Message_Box::update(const Game_State &t_game)
  {
    if (m_start_time == 0) m_start_time = t_game.state().game_time;

    if (t_game.state().game_time - m_start_time >= .5 && sf::Keyboard::isKeyPressed(sf::Keyboard::Return))
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
    const auto size = m_location.get_size(target.getView().getSize());

    auto transform = getTransform();
    transform.translate(m_location.get_position(target.getView().getSize()));
    // apply the transform
    states.transform *= transform;

    sf::RectangleShape rect(size);
    rect.setFillColor(m_fill_color);
    rect.setOutlineColor(m_outline_color);
    rect.setOutlineThickness(m_outline_thickness);

    target.draw(rect, states);
    target.draw(m_text, states);
    if (m_portrait) {
      target.draw(*m_portrait);
    }
  }



  Selection_Menu::Selection_Menu(sf::Font t_font, int t_font_size,
    sf::Color t_font_color, sf::Color t_selected_font_color, sf::Color t_fill_color, sf::Color t_outline_color, float t_outlineThickness,
    std::vector<Game_Action> t_actions, const size_t t_selection, Location t_location)
    : Game_Event(),
    m_font(std::move(t_font)), m_font_color(std::move(t_font_color)),
    m_selected_color(std::move(t_selected_font_color)),
    m_selected_cur_color(m_selected_color),
    m_fill_color(std::move(t_fill_color)), m_outline_color(std::move(t_outline_color)),
    m_outline_thickness(t_outlineThickness), m_actions(std::move(t_actions)),
    m_current_item(t_selection),
    m_location(std::move(t_location))
  {
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

  void Selection_Menu::update(const Game_State &t_game)
  {
    if (m_start_time == 0) m_start_time = t_game.state().game_time;

    if (t_game.state().game_time - m_start_time >= .5 && sf::Keyboard::isKeyPressed(sf::Keyboard::Return))
    {
      m_actions[m_current_item].action(t_game);
      m_is_done = true;
    }

    if (std::remainder(t_game.state().game_time, .5f) > 0)
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
    const auto size = m_location.get_size(target.getView().getSize());
    auto transform = getTransform();
    transform.translate(m_location.get_position(target.getView().getSize()));

    // apply the transform
    states.transform *= transform;

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
    const std::vector<Object_Action> &t_actions, Location t_location)
    : Selection_Menu(std::move(t_font), t_font_size, std::move(t_font_color), std::move(t_selected_font_color), std::move(t_fill_color), std::move(t_outline_color),
      t_outlineThickness,
      [](const std::vector<Object_Action> &t_act, Object &t_o) {
        std::vector<Game_Action> res;
        for (auto &act : t_act) {
          auto func = act.action;
          using namespace std::placeholders;
          res.emplace_back(act.description, std::bind(act.action, _1, std::ref(t_o)));
        }

        return res;
      }(t_actions, t_obj), 0, std::move(t_location))
  {
  }

}

