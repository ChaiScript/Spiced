#include "game.hpp"
#include "map.hpp"
#include "game_event.hpp"
#include "chaiscript_bindings.hpp"

#include "ChaiScript/include/chaiscript/chaiscript.hpp"


#define ADD_FUN(Class, Name) module->add(chaiscript::fun(&Class::Name), #Name )

namespace spiced {

  std::shared_ptr<chaiscript::Module> create_chaiscript_bindings()
  {
    auto module = std::make_shared<chaiscript::Module>();
    module->add(chaiscript::vector_conversion<std::vector<Tile_Defaults>>());
    module->add(chaiscript::vector_conversion<std::vector<Answer>>());
    module->add(chaiscript::vector_conversion<std::vector<Question>>());
    module->add(chaiscript::vector_conversion<std::vector<Object_Action>>());
    module->add(chaiscript::vector_conversion<std::vector<Game_Action>>());


    module->add(chaiscript::user_type<Game>(), "Game");
    ADD_FUN(Game, get_texture);
    ADD_FUN(Game, get_font);
    ADD_FUN(Game, teleport_to);
    ADD_FUN(Game, teleport_to_tile);
    ADD_FUN(Game, set_avatar);
    ADD_FUN(Game, add_map);
    ADD_FUN(Game, add_start_action);
    ADD_FUN(Game, add_queued_action);
    ADD_FUN(Game, show_message_box);
    module->add(
      chaiscript::fun([](Game &t_game, const std::string &t_msg)
          {
            t_game.show_message_box(t_msg);
          }), "show_message_box");
    ADD_FUN(Game, show_object_interaction_menu);
    ADD_FUN(Game, show_selection_menu);
    module->add(
      chaiscript::fun([](const Game_State &t_game, const std::vector<Game_Action> &t_selections)
          {
            t_game.game().show_selection_menu(t_game.state(), t_selections);
          }), "show_selection_menu");
    ADD_FUN(Game, show_conversation);
    ADD_FUN(Game, has_pending_events);
    ADD_FUN(Game, get_current_event);
    ADD_FUN(Game, update);
    ADD_FUN(Game, draw);
    ADD_FUN(Game, get_avatar_position);
    ADD_FUN(Game, enter_map);
    ADD_FUN(Game, has_current_map);
    ADD_FUN(Game, get_current_map);
    ADD_FUN(Game, start);
    ADD_FUN(Game, set_flag);
    ADD_FUN(Game, get_flag);
    ADD_FUN(Game, set_value);
    ADD_FUN(Game, get_value);
    ADD_FUN(Game, set_rotate);
    ADD_FUN(Game, set_zoom);
    ADD_FUN(Game, rotate);
    ADD_FUN(Game, zoom);

    module->add(chaiscript::fun(&Game::get_input_direction_vector), "get_input_direction_vector");

    module->add(chaiscript::user_type<Answer>(), "Answer");
    module->add(chaiscript::constructor<Answer(std::string, std::string)>(), "Answer");

    module->add(chaiscript::user_type<Question>(), "Question");

    module->add(chaiscript::constructor<
      Question(std::string, std::vector<Answer>,
        std::function<bool(const Game_State &, Object &)>,
        std::function<void(const Game_State &, Object &)>)>(), "Question");
    module->add(chaiscript::constructor<
      Question(std::string, std::vector<Answer>,
        std::function<bool(const Game_State &)>,
        std::function<void(const Game_State &)>)>(), "Question");
    module->add(chaiscript::constructor<
      Question(std::string, std::vector<Answer>,
        std::function<bool(const Game_State &)>)>(), "Question");
    module->add(chaiscript::constructor<
      Question(std::string, std::vector<Answer>)>(), "Question");

    module->add(chaiscript::user_type<Conversation>(), "Conversation");
    module->add(chaiscript::constructor<Conversation(std::vector<Question>)>(), "Conversation");

    module->add(chaiscript::user_type<Game_Action>(), "Game_Action");
    module->add(chaiscript::constructor<Game_Action(std::string, std::function<void(const Game_State &)>)>(), "Game_Action");
    ADD_FUN(Game_Action, description);
    ADD_FUN(Game_Action, action);

    module->add(chaiscript::user_type<Object_Action>(), "Object_Action");
    module->add(chaiscript::constructor<Object_Action(std::string, std::function<void(const Game_State &, Object &)>)>(), "Object_Action");
    ADD_FUN(Object_Action, description);
    ADD_FUN(Object_Action, action);


    module->add(chaiscript::user_type<Object>(), "Game_Object");
    module->add(chaiscript::constructor<Object(std::string, Tileset, const int, const bool,
      std::function<void(const Game_State &, Object &, sf::Sprite &)>,
      std::function<std::vector<Object_Action>(const Game_State &, Object &)>)>(), "Game_Object");

    ADD_FUN(Object, update);
    ADD_FUN(Object, get_actions);
    ADD_FUN(Object, do_collision);
    ADD_FUN(Object, set_position);

    module->add(chaiscript::constructor<Tile_Properties(bool)>(), "Tile_Properties");
    module->add(chaiscript::constructor<Tile_Properties(bool, bool, std::function<void(const Game_State &, const float)>, std::function<void(const Game_State &, sf::Sprite &)>)>(), "Tile_Properties");
    ADD_FUN(Tile_Properties, do_movement_action);
    ADD_FUN(Tile_Properties, passable);
    ADD_FUN(Tile_Properties, movement_action);


    module->add(chaiscript::user_type<Game_State>(), "Game_State");
    ADD_FUN(Game_State, game);
    ADD_FUN(Game_State, state);

    module->add(chaiscript::user_type<Simulation_State>(), "Simulation_State");
    ADD_FUN(Simulation_State, game_time);
    ADD_FUN(Simulation_State, simulation_time);


    module->add(chaiscript::constructor<Tile_Defaults(const int, Tile_Properties)>(), "Tile_Defaults");

    module->add(chaiscript::user_type<Script_Parser>(), "Script_Parser");
    module->add(chaiscript::constructor<Script_Parser()>(), "Script_Parser");
    ADD_FUN(Script_Parser, collision_action_parser);


    module->add(chaiscript::constructor<Tile_Map(Game &, const std::string &, std::vector<Tile_Defaults>, const Script_Parser &)>(), "Tile_Map");

    ADD_FUN(Tile_Map, add_enter_action);
    ADD_FUN(Tile_Map, enter);
    ADD_FUN(Tile_Map, dimensions_in_pixels);
    ADD_FUN(Tile_Map, add_object);
    ADD_FUN(Tile_Map, get_bounding_box);
    ADD_FUN(Tile_Map, test_move);
    ADD_FUN(Tile_Map, get_collisions);
    ADD_FUN(Tile_Map, adjust_move);
    ADD_FUN(Tile_Map, do_move);
    ADD_FUN(Tile_Map, update);
    ADD_FUN(Tile_Map, set_collision_action);
    ADD_FUN(Tile_Map, set_action_generator);
    ADD_FUN(Tile_Map, set_portrait);

    module->add(chaiscript::type_conversion<std::string, sf::String>());

    return module;
  }

}
