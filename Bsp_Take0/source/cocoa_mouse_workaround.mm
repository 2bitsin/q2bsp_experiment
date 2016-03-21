//
//  cocoa_mouse_workaround.cpp
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/21/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#include "cocoa_mouse_workaround.hpp"

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <functional>
#include <cstring>


bool cocoa_drain_mouse_events (SDL_Window* pWindow, const std::function<void (int dx, int dy)>& callback) {
    __block bool didHandleEvent = false;
    
    static NSWindow* cWindow = ([pWindow] () {
        SDL_SysWMinfo wmInfo;
        std::memset (&wmInfo, 0, sizeof (wmInfo));
        SDL_GetWindowWMInfo (pWindow, &wmInfo);
        return wmInfo.info.cocoa.window;
    }) ();
    
    
    [cWindow trackEventsMatchingMask:NSMouseMovedMask|NSLeftMouseDraggedMask|NSRightMouseDraggedMask
        timeout:0
           mode:NSEventTrackingRunLoopMode
        handler:^(NSEvent * _Nonnull event, BOOL * _Nonnull stop) {
            callback ([event deltaX], [event deltaY]);
            didHandleEvent = true;
            *stop = YES;
        }
    ];
    return didHandleEvent;
}