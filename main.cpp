#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <chrono>
#include <functional>
#include <deque>
#include <memory>
#include <cassert>
#include <cmath>

#include "game.hpp"
#include "game_event.hpp"
#include "map.hpp"
#include "chaiscript_creator.hpp"
#include "ChaiScript/include/chaiscript/chaiscript.hpp"

Game build_chai_game(chaiscript::ChaiScript &chai)
{
  Game game;

  chai.boxed_cast<std::function<void (Game &)>>(chai.eval_file("main.chai"))(game);

  return game;
}

Game build_game()
{
  Game game;

  // create the tilemap from the level definition
  Tile_Map map(game, "resources/Maps/worldmap.json", {{2, Tile_Properties(false)}});
//  Tile_Map map(game, "resources/Maps/test.json", {{2, Tile_Properties(false)}});


  const auto collision_action = [](const float t_game_time, const float t_simulation_time, Game &t_game, Object &t_obj, sf::Sprite &/*t_collision*/)
  {
    t_game.show_object_interaction_menu(t_game_time, t_simulation_time, t_obj);
  };

  const auto bobby_pinhead_conversation_tree = 
    Conversation({
      Question("Pinhead Town?",
        {Answer("Bobby Pinhead", "Pinhead town is where we call home. Be sure to check out the shops.")},
        [](const float, const float, Game &t_game, Object &) { return t_game.get_flag("pinhead_town"); },
        [](const float, const float, Game &, Object &) {  }
      ),
      Question("Treasure in forest?",
        {Answer("Bobby Pinhead", "I've heard about it...\nbut I don't know anyone who has found it yet."),
         Answer("Random Pinhead", "Bah, treasure in the forest.")},
        [](const float, const float, Game &t_game, Object &) { return t_game.get_flag("treasure_adventure"); },
        [](const float, const float, Game &, Object &) {  }
      )
    });



  const auto random_pinhead_conversation_tree = 
    Conversation({
      Question("Where am I?",
        {Answer("Random Pinhead", "You are in the Pinhead Town Center."),
         Answer("Bobby Pinhead", "Pinhead Town is a great place, you'll like it here.\nThere is lots of adventure.")},
        [](const float, const float, Game &, Object &) { return true; },
        [](const float, const float, Game &t_game, Object &) { t_game.set_flag("pinhead_town", true); }
      ),
      Question("Treasure In Forest?",
        {Answer("Random Pinhead", "I'm skeptical about that treasure.\nSome people say they have seen it,\nbut why haven't they become rich?"),
         Answer("Random Pinhead", "I think it's just a pipe dream of that Baker.\nHe's the one who likes to talk about it so much.")},
        [](const float, const float, Game &t_game, Object &) { return t_game.get_flag("treasure_adventure"); },
        [](const float, const float, Game &, Object &) {  }
      ),
    });


  const auto bobby_pinhead_actions = [bobby_pinhead_conversation_tree](const float /*t_game_time*/, const float /*t_simulation_time*/, Game &/*t_game*/, Object &/*t_obj*/){
      return std::vector<Object_Action>{
        { "Look", 
          [](const float, const float, Game &t_game, Object &) 
          {
            t_game.show_message_box("Bobby Pinhead.\n\nA pinhead youth.");
          }
        },
        { "Talk To", 
          [bobby_pinhead_conversation_tree](const float t_game_time, const float t_simulation_time, Game &t_game, Object &t_obj) 
          {
            t_game.show_conversation(t_game_time, t_simulation_time, t_obj, bobby_pinhead_conversation_tree);
          }
        },
      };
  };

  const auto random_pinhead_actions = [random_pinhead_conversation_tree](const float /*t_game_time*/, const float /*t_simulation_time*/, Game &/*t_game*/, Object &/*t_obj*/){
      return std::vector<Object_Action>{
        { "Look", 
          [](const float, const float, Game &t_game, Object &) 
          {
            t_game.show_message_box("A Random Pinhead.\n\nOne of the local pinheads.");
          }
        },
        { "Talk To", 
          [random_pinhead_conversation_tree](const float t_game_time, const float t_simulation_time, Game &t_game, Object &t_obj) 
          {
            t_game.show_conversation(t_game_time, t_simulation_time, t_obj, random_pinhead_conversation_tree);
          }
        },
      };
  };

  const auto left_door_actions = [](const float /*t_game_time*/, const float /*t_simulation_time*/, Game &/*t_game*/, Object &/*t_obj*/){
      return std::vector<Object_Action>{
        { "Look", 
          [](const float, const float, Game &t_game, Object &) 
          {
            t_game.show_message_box("The Bakery");
          }
        },
      };
  };

  const auto path_actions = [](const float /*t_game_time*/, const float /*t_simulation_time*/, Game &/*t_game*/, Object &/*t_obj*/){
      return std::vector<Object_Action>{
        { "Look", 
          [](const float, const float, Game &t_game, Object &) 
          {
            t_game.show_message_box("Forest Path");
          }
        },
      };
  };

  const auto gate_actions = [](const float /*t_game_time*/, const float /*t_simulation_time*/, Game &/*t_game*/, Object &/*t_obj*/){
      return std::vector<Object_Action>{
        { "Look", 
          [](const float, const float, Game &t_game, Object &) 
          {
            t_game.show_message_box("Garden Gate");
          }
        },
      };
  };


  const auto up_door_actions = [](const float /*t_game_time*/, const float /*t_simulation_time*/, Game &/*t_game*/, Object &/*t_obj*/){
      return std::vector<Object_Action>{
        { "Look", 
          [](const float, const float, Game &t_game, Object &) 
          {
            t_game.show_message_box("The General Store");
          }
        },
        { "Talk To", 
          [](const float , const float , Game &t_game, Object &) 
          {
            t_game.show_message_box("You hear a voice from beyond the door:\n'Come in, we're open!'");
          }
        },
      };
  };


  Object general_store_door("General Store Door", game.get_texture("resources/pinheads_up_door.png"), 48, 26, 0, collision_action, up_door_actions);
  general_store_door.setPosition(299,134);
  map.add_object(general_store_door);

  Object bakery_door("Bakery Store Door", game.get_texture("resources/pinheads_left_door.png"), 22, 40, 0, collision_action, left_door_actions);
  bakery_door.setPosition(138, 191);
  map.add_object(bakery_door);

  Object path("Forest Path", game.get_texture("resources/pinheads_path.png"), 22, 11, 0, collision_action, path_actions);
  path.setPosition(137, 168);
  map.add_object(path);

  Object gate("Garden Gate", game.get_texture("resources/pinheads_gate.png"), 39, 12, 0, collision_action, gate_actions);
  gate.setPosition(344, 148);
  map.add_object(gate);

  Object bobby_pinhead("Bobby Pinhead", game.get_texture("resources/pinheads_pinhead.png"), 26, 26, 0, collision_action, bobby_pinhead_actions);
  bobby_pinhead.setPosition(270,220);
  map.add_object(bobby_pinhead);

  Object random_pinhead("Random Pinhead", game.get_texture("resources/pinheads_pinhead.png"), 26, 26, 0, collision_action, random_pinhead_actions);
  random_pinhead.setPosition(240,250);
  map.add_object(random_pinhead);


  map.add_enter_action(
      [](Game &t_game){
        t_game.teleport_to(200, 200);
        t_game.show_message_box("Welcome to Pinhead Town.");
      }
    );

  game.add_map("town", map);
  sf::Sprite m_avatar(game.get_texture("resources/pinheads_marble.png"));
  game.set_avatar(m_avatar);


  game.add_start_action(
      [](Game &t_game) {
        t_game.show_message_box("Welcome to\nPinheads:\n   Everything You Need.");
        t_game.add_queued_action([](const float, const float, Game &t_t_game) {
          t_t_game.enter_map("town");
          });
      }
    );

  return game;
}

int main()
{
  // create the window
  sf::RenderWindow window(sf::VideoMode(512, 400), "Tilemap");

  auto chaiscript = create_chaiscript();

  auto game = build_chai_game(*chaiscript);

  auto start_time = std::chrono::steady_clock::now();

  auto last_frame = std::chrono::steady_clock::now();
  uint64_t frame_count = 0;

  sf::View fixed = window.getView();

  game.start();

  // run the main loop
  while (window.isOpen())
  {
    ++frame_count;
    auto cur_frame = std::chrono::steady_clock::now();
    auto time_elapsed = std::chrono::duration_cast<std::chrono::duration<float>>(cur_frame - last_frame).count();
    auto game_time =  std::chrono::duration_cast<std::chrono::duration<float>>(cur_frame - start_time).count();

    if (frame_count % 100 == 0)
    {
      std::cout << 1/time_elapsed << "fps avg fps: " << frame_count / game_time << '\n';
    }

    last_frame = cur_frame;
    // handle events
    sf::Event event;
    while (window.pollEvent(event))
    {
      if(event.type == sf::Event::Closed) {
        window.close();
      }

      if (event.type == sf::Event::Resized)
      {
        // update the view to the new size of the window
        window.setSize(sf::Vector2u(event.size.width, event.size.height));
        fixed = sf::View(sf::FloatRect(0,0,event.size.width, event.size.height));
      }
    }

    game.update(game_time, time_elapsed);

    const auto window_size = window.getSize();
    sf::View mainView(game.get_avatar_position(), sf::Vector2f(window_size));
    mainView.zoom(game.zoom());
    mainView.rotate(game.rotate());
    window.setView(mainView);

    window.clear();

    // main frame
    window.draw(game);

    if (game.has_current_map())
    {
      // mini view
      const auto dimensions = sf::Vector2f(game.get_current_map().dimensions_in_pixels());
      sf::View miniView(sf::FloatRect(sf::Vector2f(0,0), dimensions));
      miniView.setViewport(sf::FloatRect(0.75f, 0, 0.25f, (.25f * window_size.x) * (dimensions.y / dimensions.x) / window_size.y ));
      window.setView(miniView);
      window.draw(game);
    }

    // fixed overlays
    window.setView(fixed);

    if (game.has_pending_events())
    {
      window.draw(game.get_current_event());
    }

    window.display();
  }
}


