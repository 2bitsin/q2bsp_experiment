//
//  cocoa_mouse_workaround.hpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/21/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#ifndef cocoa_mouse_workaround_hpp
#define cocoa_mouse_workaround_hpp

#include <SDL2/SDL.h>
#include <functional>

bool cocoa_drain_mouse_events (SDL_Window* pWindow, const std::function<void (int dx, int dy)>& callback);

#endif /* cocoa_mouse_workaround_hpp */
