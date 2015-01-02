#ifndef GAME_ENGINE_GAME_EVENT_HPP
#define GAME_ENGINE_GAME_EVENT_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <functional>

class Object;
class Game;
class Object_Action;

class Game_Event : public sf::Drawable, public sf::Transformable
{
  public:
    virtual ~Game_Event() = default;

    virtual bool is_done() const = 0;

    virtual void update(const float t_game_time, const float t_simulation_time, Game &t_game) = 0;

};

class Queued_Action : public Game_Event
{
  public:
    Queued_Action(std::function<void ()> t_action);

    virtual void update(const float, const float, Game &);

    virtual bool is_done() const;

    virtual void draw(sf::RenderTarget& /*target*/, sf::RenderStates /*states*/) const
    { /*nothing to do*/ }

  private:
    bool m_done = false;
    std::function<void ()> m_action;
};

class Message_Box : public Game_Event
{
  public:
    Message_Box(sf::String t_string, sf::Font t_font, int t_font_size,
        sf::Color t_font_color, sf::Color t_fill_color, sf::Color t_outline_color, float t_outlineThickness);

    virtual ~Message_Box() = default;

    virtual void update(const float t_game_time, const float /*t_simulation_time*/, Game &t_game);

    virtual bool is_done() const;

  protected:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

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


class Object_Interaction_Menu : public Game_Event
{
  public:
    Object_Interaction_Menu(Object &t_obj, sf::Font t_font, int t_font_size,
        sf::Color t_font_color, sf::Color t_selected_font_color, sf::Color t_fill_color, sf::Color t_outline_color, float t_outlineThickness,
        std::vector<Object_Action> t_actions);

    virtual ~Object_Interaction_Menu() = default;

    virtual void update(const float t_game_time, const float t_simulation_time, Game &t_game);

    virtual bool is_done() const;

  protected:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

  private:
    std::reference_wrapper<Object> m_obj;
    sf::Font m_font;
    sf::Color m_font_color;
    sf::Color m_selected_color;
    sf::Color m_selected_cur_color;
    sf::Color m_fill_color;
    sf::Color m_outline_color;
    float m_outline_thickness;
    std::vector<sf::Text> m_texts;

    int m_current_item = 0;
    int m_last_direction = 0;
    float m_start_time = 0;
    bool m_is_done = false;

    std::vector<Object_Action> m_actions;
};

#endif



