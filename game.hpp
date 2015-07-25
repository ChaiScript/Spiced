#ifndef GAME_ENGINE_GAME_HPP
#define GAME_ENGINE_GAME_HPP

#include <SFML/Graphics.hpp>
#include <functional>
#include <deque>
#include <memory>
#include <map>
#include <string>

class Tile_Map;
class Object;
class Game_Event;
struct Game_Action;
struct Conversation;

class Game : public sf::Drawable
{
  public:
    Game();
    Game(const Game &) = delete;
    Game(Game &&) = default;


    const sf::Texture &get_texture(const std::string &t_filename) const;
    const sf::Font &get_font(const std::string &t_filename) const;

    void teleport_to(const float x, const float y);
    void teleport_to_tile(const int x, const int y);

    void set_avatar(const sf::Texture &t_avatar);

    void add_map(const std::string &t_name, const Tile_Map &t_map);

    void add_start_action(const std::function<void (Game &)> &t_action);

    void add_queued_action(const std::function<void (const float t_game_time, const float t_simulation_time, Game &)> &t_action);

    void show_message_box(const sf::String &t_msg);

    void show_selection_menu(const float t_game_time, const float t_simulation_time, const std::vector<Game_Action> &t_selections, const size_t t_selection = 0);
    void show_object_interaction_menu(const float t_game_time, const float t_simulation_time, Object &t_obj);
    void show_conversation(const float t_game_time, const float t_simulation_time, Object &t_obj, const Conversation &t_conversation);

    bool has_pending_events() const;

    Game_Event &get_current_event() const;

    void update(const float t_game_time, const float t_simulation_time);

    static sf::Vector2f get_input_direction_vector();

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

    sf::Vector2f get_avatar_position() const;

    void enter_map(const std::string &t_name);

    bool has_current_map() const;

    const Tile_Map &get_current_map() const;

    void start();

    void set_flag(const std::string &t_name, bool t_value);
    bool get_flag(const std::string &t_name) const;

    void set_value(const std::string &t_name, int t_value);
    int get_value(const std::string &t_name) const;

    void set_rotate(const float);
    void set_zoom(const float);

    float rotate();
    float zoom();

    bool show_mini_map() const;
    bool show_invisible() const;

  private:

    mutable std::map<std::string, sf::Texture> m_textures;
    mutable std::map<std::string, sf::Font> m_fonts;

    std::deque<std::unique_ptr<Game_Event>> m_game_events;
    std::map<std::string, Tile_Map> m_maps;

    sf::Sprite m_avatar;
    std::map<std::string, Tile_Map>::iterator m_map;
    std::vector<std::function<void (Game &)>> m_start_actions;

    std::map<std::string, bool> m_flags;
    std::map<std::string, int> m_values;

    float m_rotate;
    float m_zoom;
};



#endif

