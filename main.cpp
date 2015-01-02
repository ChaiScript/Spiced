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

Game build_game()
{
  Game game;
  // define the level with an array of tile indices
  const std::vector<int> level {
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3,
    0, 1, 0, 0, 2, 0, 3, 3, 3, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 2, 0, 3, 3, 3, 0, 1, 1, 1, 0, 0, 0,
    0, 1, 1, 0, 3, 3, 3, 0, 0, 0, 1, 1, 1, 2, 0, 0, 0, 1, 1, 0, 3, 3, 3, 0, 0, 0, 1, 1, 1, 2, 0, 0,
    0, 0, 1, 0, 3, 0, 2, 2, 0, 0, 1, 1, 1, 1, 2, 0, 0, 0, 1, 0, 3, 0, 2, 2, 0, 0, 1, 1, 1, 1, 2, 0,
    2, 0, 1, 0, 3, 0, 2, 2, 2, 0, 1, 1, 1, 1, 1, 1, 2, 0, 1, 0, 3, 0, 2, 2, 2, 0, 1, 1, 1, 1, 1, 1,
    0, 0, 1, 0, 3, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 3, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3,
    0, 1, 0, 0, 2, 0, 3, 3, 3, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 2, 0, 3, 3, 3, 0, 1, 1, 1, 0, 0, 0,
    0, 1, 1, 0, 3, 3, 3, 0, 0, 0, 1, 1, 1, 2, 0, 0, 0, 1, 1, 0, 3, 3, 3, 0, 0, 0, 1, 1, 1, 2, 0, 0,
    0, 0, 1, 0, 3, 0, 2, 2, 0, 0, 1, 1, 1, 1, 2, 0, 0, 0, 1, 0, 3, 0, 2, 2, 0, 0, 1, 1, 1, 1, 2, 0,
  };

  // create the tilemap from the level definition
  Tile_Map map(game.get_texture("resources/tileset.png"), sf::Vector2u(32, 32), level, 32, 14, {{1, Tile_Properties(false)}});


  const auto candle_collision_action = [](const float t_game_time, const float t_simulation_time, Game &t_game, Object &t_obj, sf::Sprite &/*t_collision*/)
  {
    t_game.show_object_interaction_menu(t_game_time, t_simulation_time, t_game, t_obj);
  };

  const auto candle_actions = [](const float /*t_game_time*/, const float /*t_simulation_time*/, Game &/*t_game*/, Object &/*t_obj*/){
      return std::vector<Object_Action>{
        { "Look", 
          [](const float, const float, Game &t_game, Object &) 
          {
            t_game.show_message_box("Nothing to see here");
          }
        },
        { "Examine", 
          [](const float, const float, Game &t_game, Object &) 
          {
            t_game.show_message_box("You really don't see anything.");
          }
        },
        { "Take", 
          [](const float, const float, Game &t_game, Object &) 
          {
            t_game.show_message_box("The candle is firmly fixed to the ground.");
          }
        }
      };
  };

  Object candle(game.get_texture("resources/candle.png"), 32, 32, 3, candle_collision_action, candle_actions);
  candle.setPosition(100,200);
  map.add_object(candle);

  Object candle2(game.get_texture("resources/candle.png"), 32, 32, 3, candle_collision_action, candle_actions);
  candle2.setPosition(150,200);
  map.add_object(candle2);

  map.add_enter_action(
      [](Game &t_game){
        t_game.teleport_to(200, 200);
        t_game.show_message_box("Welcome to 'map.'");
      }
    );

  game.add_map("map", map);
  sf::Sprite m_avatar(game.get_texture("resources/sprite.png"));
  game.set_avatar(m_avatar);

  game.add_start_action(
      [](Game &t_game) {
        t_game.show_message_box("Welcome to the Game!");
        t_game.add_queued_action([](Game &t_t_game) {
          t_t_game.enter_map("map");
          });
      }
    );

  return game;
}

int main()
{
  // create the window
  sf::RenderWindow window(sf::VideoMode(512, 400), "Tilemap");


  auto game = build_game();

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


