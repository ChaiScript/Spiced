#ifndef GAME_ENGINE_GAME_EVENT_HPP
#define GAME_ENGINE_GAME_EVENT_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <functional>

namespace spiced
{

  class Object;
  class Game;
  struct Object_Action;
  struct Game_Action;

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
      std::function<bool(const float, const float, Game &, Object &)> t_is_available,
      std::function<void(const float, const float, Game &, Object &)> t_action)
      : question(std::move(t_question)),
      answers(std::move(t_answers)),
      is_available(std::move(t_is_available)),
      action(std::move(t_action))
    {
    }

    std::string question;
    std::vector<Answer> answers;
    std::function<bool(const float, const float, Game &, Object &)> is_available;
    std::function<void(const float, const float, Game &, Object &)> action;

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

    virtual void update(const float t_game_time, const float t_simulation_time, Game &t_game) = 0;

  };

  class Queued_Action : public Game_Event
  {
  public:
    Queued_Action(std::function<void(const float, const float, Game &)> t_action);

    virtual void update(const float, const float, Game &);

    virtual bool is_done() const;

    virtual void draw(sf::RenderTarget& /*target*/, sf::RenderStates /*states*/) const
    { /*nothing to do*/
    }

  private:
    bool m_done = false;
    std::function<void(const float, const float, Game &)> m_action;
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

  class Selection_Menu : public Game_Event
  {
  public:
    Selection_Menu(sf::Font t_font, int t_font_size,
      sf::Color t_font_color, sf::Color t_selected_font_color, sf::Color t_fill_color, sf::Color t_outline_color, float t_outlineThickness,
      std::vector<Game_Action> t_actions,
      const size_t t_selection);

    virtual ~Selection_Menu() = default;

    virtual void update(const float t_game_time, const float t_simulation_time, Game &t_game) override;

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
    std::vector<sf::Text> m_texts;

    std::vector<Game_Action> m_actions;

    size_t m_current_item = 0;
    int m_last_direction = 0;
    float m_start_time = 0;
    bool m_is_done = false;

  };


  class Object_Interaction_Menu : public Selection_Menu
  {
  public:
    Object_Interaction_Menu(Object &t_obj, sf::Font t_font, int t_font_size,
      sf::Color t_font_color, sf::Color t_selected_font_color, sf::Color t_fill_color, sf::Color t_outline_color, float t_outlineThickness,
      const std::vector<Object_Action> &t_actions);
  };

}

#endif



