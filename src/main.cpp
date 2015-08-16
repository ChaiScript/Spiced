#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <chrono>
#include <functional>
#include <memory>

#include "game.hpp"
#include "game_event.hpp"
#include "map.hpp"
#include "chaiscript_creator.hpp"
#include "ChaiScript/include/chaiscript/chaiscript.hpp"

void show_error(const std::string &t_what)
{
  std::cout << "An unhandled error has occured:\n" << t_what << std::endl;

#ifdef _WIN32
  MessageBox(nullptr, t_what.c_str(), nullptr, MB_ICONERROR | MB_OK);
#endif
}

spiced::Game build_chai_game(chaiscript::ChaiScript &chai)
{
  spiced::Game game;
  chai.boxed_cast<std::function<void (spiced::Game &)>>(chai.eval_file("spiced.chai"))(game);
  return game;
}


int main()
{
  try {
    // create the window
    sf::RenderWindow window(sf::VideoMode(800, 600), "Tilemap");

    window.setVerticalSyncEnabled(true);
    auto chaiscript = spiced::create_chaiscript();

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
        const auto avgfps = frame_count / game_time;
        const auto curfps = 1 / time_elapsed;
        std::cout << curfps << "fps avg fps: " << avgfps << '\n';
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
          fixed = sf::View(sf::FloatRect(0,0, float(event.size.width), float(event.size.height)));
        }
      }

      game.update(spiced::Simulation_State(game_time, time_elapsed));

      const auto window_size = window.getSize();
      sf::View mainView(game.get_avatar_position(), sf::Vector2f(window_size));
      mainView.zoom(game.zoom());
      mainView.rotate(game.rotate());
      window.setView(mainView);

      window.clear();

      // main frame
      window.draw(game);


      if (game.show_mini_map() && game.has_current_map())
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
  catch (const chaiscript::exception::eval_error &ee) {
    show_error(ee.pretty_print());
  } catch (const std::exception &e) {
    show_error(e.what());
  }
}


