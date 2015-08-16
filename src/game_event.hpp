#ifndef GAME_ENGINE_GAME_EVENT_HPP
#define GAME_ENGINE_GAME_EVENT_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <functional>
#include <memory>

namespace spiced
{

  class Object;
  class Game;
  struct Object_Action;
  struct Game_Action;
  class Game_State;

  struct Answer
  {
    Answer(std::string t_speaker, std::string t_answer)
      : speaker(std::move(t_speaker)),
      answer(std::move(t_answer))
    {
    }

    std::string speaker;
    std::string answer;
  };

  struct Question
  {
    Question(std::string t_question,
      std::vector<Answer> t_answers,
      std::function<bool(const Game_State &, Object &)> t_is_available,
      std::function<void(const Game_State &, Object &)> t_action)
      : question(std::move(t_question)),
        answers(std::move(t_answers)),
        is_available(std::move(t_is_available)),
        action(std::move(t_action))
    {
    }

    Question(std::string t_question,
      std::vector<Answer> t_answers,
      std::function<bool(const Game_State &)> t_is_available = [](const Game_State &){return true;},
      std::function<void(const Game_State &)> t_action = {})
      : question(std::move(t_question)),
        answers(std::move(t_answers)),
        is_available([t_is_available](const Game_State &t_game, Object &){ return t_is_available(t_game); }),
        action(
            [t_action]() -> std::function<void(const Game_State &, Object &)> {
              if (t_action) {
                return [t_action](const Game_State &t_game, Object &){ t_action(t_game); };
              } else {
                return std::function<void(const Game_State &, Object &)>();
              }
            }())
    {
    }




    std::string question;
    std::vector<Answer> answers;
    std::function<bool(const Game_State &, Object &)> is_available;
    std::function<void(const Game_State &, Object &)> action;

  };

  struct Conversation
  {
    Conversation(std::vector<Question> t_questions)
      : questions(std::move(t_questions))
    {
    }

    std::vector<Question> questions;
  };


  class Game_Event : public sf::Drawable, public sf::Transformable
  {
  public:
    virtual ~Game_Event() = default;

    virtual bool is_done() const = 0;

    virtual void update(const Game_State &t_game) = 0;

  };

  class Queued_Action : public Game_Event
  {
  public:
    Queued_Action(std::function<void(const Game_State &)> t_action);

    virtual void update(const Game_State &);

    virtual bool is_done() const;

    virtual void draw(sf::RenderTarget& /*target*/, sf::RenderStates /*states*/) const
    { /*nothing to do*/
    }

  private:
    bool m_done = false;
    std::function<void(const Game_State &)> m_action;
  };


  struct Location
  {
    enum Position {
      Top,
      Bottom,
      Left,
      Right,
      Center
    };

    Location(Position t_p) 
      : position(t_p) {
    }

    sf::Vector2f get_position(const sf::Vector2f &t_size) const {
      switch (position) {
        case Bottom:
          return sf::Vector2f(margin, t_size.y / 2);
        case Center:
          return sf::Vector2f(margin, t_size.y / 4);
        case Right:
          return sf::Vector2f(t_size.x / 2, margin);
        default:
          return sf::Vector2f(margin, margin);
      };
    }

    sf::Vector2f get_size(const sf::Vector2f &t_size) const {
      switch (position) {
        case Top:
        case Bottom:
        case Center:
          return sf::Vector2f(t_size.x - (margin * 2), t_size.y / 2 - margin);
        default:
          return sf::Vector2f(t_size.x / 2 - margin, t_size.y - (margin * 2));
      };
    }

    Position position;
    int margin = 10;
  };


  class Message_Box : public Game_Event
  {
  public:
    Message_Box(sf::String t_string, sf::Font t_font, int t_font_size,
      sf::Color t_font_color, sf::Color t_fill_color, sf::Color t_outline_color, float t_outlineThickness, Location t_location, const sf::Texture *t_texture);

    virtual ~Message_Box() = default;

    virtual void update(const Game_State &t_game);

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
    Location m_location;
    std::unique_ptr<sf::Sprite> m_portrait;

    float m_start_time = 0;
    bool m_is_done = false;
  };

  class Selection_Menu : public Game_Event
  {
  public:
    Selection_Menu(sf::Font t_font, int t_font_size,
      sf::Color t_font_color, sf::Color t_selected_font_color, sf::Color t_fill_color, sf::Color t_outline_color, float t_outlineThickness,
      std::vector<Game_Action> t_actions,
      const size_t t_selection, Location t_location);

    virtual ~Selection_Menu() = default;

    virtual void update(const Game_State &) override;

    virtual bool is_done() const override;

  protected:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  private:
    sf::Font m_font;
    sf::Color m_font_color;
    sf::Color m_selected_color;
    sf::Color m_selected_cur_color;
    sf::Color m_fill_color;
    sf::Color m_outline_color;
    float m_outline_thickness;

    std::vector<Game_Action> m_actions;
    std::vector<sf::Text> m_texts;
    size_t m_current_item = 0;
    Location m_location;

    int m_last_direction = 0;
    float m_start_time = 0;
    bool m_is_done = false;

  };


  class Object_Interaction_Menu : public Selection_Menu
  {
  public:
    Object_Interaction_Menu(Object &t_obj, sf::Font t_font, int t_font_size,
      sf::Color t_font_color, sf::Color t_selected_font_color, sf::Color t_fill_color, sf::Color t_outline_color, float t_outlineThickness,
      const std::vector<Object_Action> &t_actions, Location t_location);
  };

}

#endif



