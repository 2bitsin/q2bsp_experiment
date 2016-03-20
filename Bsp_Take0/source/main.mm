//
//  main.m
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/16/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//


#include "file_utils.hpp"
#include "bsp.hpp"
#include "debug.hpp"

#include <string>
#include <sstream>
#include <regex>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/ext.hpp>

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#define CHECK()\
if (auto error = glGetError()) {\
	xtk::Debug::log ("%s(%d) : glError = %u\n", __FILE__, __LINE__, error);\
}

//////////////////////////
/// basic.vertex.source
//////////////////////////
char __basic_vertex_source [] = R"(
    #version 410

    uniform mat4 g_model_view_projection;
    uniform mat3 g_normal_matrix;
    uniform vec3 g_constant_color;
    uniform float g_constant_alpha;

    layout (location = 0) in vec3 vs_vertex_position;
    layout (location = 1) in vec3 vs_vertex_normal;
    layout (location = 2) in vec2 vs_vertex_uv_tex;
    layout (location = 3) in vec3 vs_vertex_color;

    out vec4 fs_vertex_position;
    out vec3 fs_vertex_normal;
    out vec2 fs_vertex_uv_tex;
    out vec3 fs_vertex_color;

    void main () {
        fs_vertex_position = g_model_view_projection * vec4 (vs_vertex_position, 1.0);
        fs_vertex_normal = g_normal_matrix * vs_vertex_position;
        fs_vertex_uv_tex = vs_vertex_uv_tex;
        fs_vertex_color = vs_vertex_color;
        
        gl_Position = fs_vertex_position;
        
    }
)";

///////////////////////////
/// basic.fragment.source
///////////////////////////
char __basic_fragment_source [] = R"(
    #version 410

    uniform mat4 g_model_view_projection;
    uniform mat3 g_normal_matrix;
    uniform vec3 g_constant_color;
    uniform float g_constant_alpha;

    in vec4 fs_vertex_position;
    in vec3 fs_vertex_normal;
    in vec2 fs_vertex_uv_tex;
    in vec3 fs_vertex_color;

    layout (location = 0) out vec4 out_fragment_color;

    void main () {
        out_fragment_color = vec4 (fs_vertex_color, g_constant_alpha);
    }
)";

struct global_state {
    SDL_Window* pWindow;
    SDL_GLContext pContext;
    NSWindow* pCocoaWindow;
    
    xtk::bsp_data_quake2* map_data;
    
    double last_timestamp;
    double last_deltatime;
        
    GLsizei gl_element_count;
    GLuint gl_buffer_vertex;
    GLuint gl_buffer_edge;
    GLuint gl_varray;
    GLuint gl_program;
    
    glm::vec3 player_velocity;
    glm::vec3 player_position;
    glm::vec3 player_euler_angles;
    glm::vec3 player_forward;
    glm::vec3 player_sideways;
    glm::vec3 player_upwards;
};


std::string annotate_shader_source (const std::string& source, const std::string& info_log) {
    std::stringstream oss ;
    std::stringstream iss ;
    std::string line;
    std::size_t line_counter = 0u;
    
    std::unordered_map<std::uint64_t, std::vector<std::string>> m_log;
    
    if (!info_log.empty ()) {
        iss.str (info_log);
        while (std::getline (iss, line)) {
            static const std::regex ereg (R"(^ERROR:\s+(\d+):(\d+):(.*?)$)", std::regex_constants::optimize);
            std::smatch result;
            if (std::regex_search (line, result, ereg)) {
                auto eline = std::stoul (result [2].str ());
                m_log [eline].push_back(result [3].str ());
            }
        }
    }
    iss.clear ();
    iss.str (source);
    oss << std::setfill(' ');
    
    while (std::getline (iss, line)) {
        ++line_counter;
        oss << std::setw (4) << line_counter << " : " << line << std::endl;
        auto ait = m_log.find (line_counter);
        if (ait != m_log.end ()) {
            for (const auto& aline : ait->second) {
                oss << "\t\t^^" << aline << "\n";
            }
        }
    }
    
    return oss.str ();
}


GLuint build_gl_shaders (const std::initializer_list<std::pair<GLenum, std::string>> args, std::string& log) {
    GLuint uProgram, uObject;
    GLint param;
    
    uProgram = glCreateProgram ();

    for (const auto& varg: args) {
        uObject = glCreateShader (varg.first);
        
        const auto& sSource = varg.second;
        
        GLint pLength [] = { GLint (sSource.length ()) };
        const GLchar* pSource [] = { sSource.c_str () };
        
        glShaderSource (uObject, 1u, pSource, pLength);
        glCompileShader(uObject);
        
        glGetShaderiv (uObject, GL_COMPILE_STATUS, &param);
        if (!param) {
            glGetShaderiv (uObject, GL_INFO_LOG_LENGTH, &param);
            auto infoLog = std::make_unique<GLchar []> (param + 1);
            glGetShaderInfoLog (uObject, param + 1, &param, infoLog.get ());
            
            glDeleteShader (uObject);
            
            log.append ("\nShader compile error: \n");
            log.append (annotate_shader_source (sSource, infoLog.get()));
            log.append ("\n");
            continue;
        }
        
        glAttachShader (uProgram, uObject);
        glDeleteShader (uObject);
        
    }
    
    glLinkProgram (uProgram);
    glGetProgramiv (uProgram, GL_LINK_STATUS, &param);
    if (!param) {
        glGetProgramiv (uProgram, GL_INFO_LOG_LENGTH, &param);
        auto infoLog = std::make_unique<GLchar []> (param + 1);
        glGetProgramInfoLog (uProgram, param + 1, &param, infoLog.get ());
        
        glDeleteProgram (uProgram);
        
        log.append ("\nProgram link error: \n");
        log.append (infoLog.get ());
        log.append ("\n");
        
        return 0u;
    }
    
    return uProgram;
}

void setup_sdl_and_gl (global_state& state) {
    SDL_LogSetOutputFunction (SDL_LogOutputFunction ([] (
        void*           userdata,
        int             category,
        SDL_LogPriority priority,
        const char*     message)
    {
        xtk::Debug::log ("%d, %d, %s\n", category, priority, message);
    }), nullptr);
    
    SDL_LogSetAllPriority (SDL_LOG_PRIORITY_VERBOSE);
    
    SDL_Init (SDL_INIT_EVERYTHING);
    std::atexit (SDL_Quit);
    
    state.pWindow = SDL_CreateWindow ("Hello",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280u, 720u,
        SDL_WINDOW_OPENGL|SDL_WINDOW_ALLOW_HIGHDPI);
    
    SDL_EventState (SDL_SYSWMEVENT, SDL_ENABLE);
    
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 32);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    
    state.pContext = SDL_GL_CreateContext (state.pWindow);
    SDL_GL_MakeCurrent (state.pWindow, state.pContext);
 
    SDL_SysWMinfo wmInfo;
    std::memset (&wmInfo, 0, sizeof (wmInfo));
    SDL_GetWindowWMInfo (state.pWindow, &wmInfo);
    state.pCocoaWindow = wmInfo.info.cocoa.window;
    
    SDL_SetRelativeMouseMode (SDL_TRUE);
    
}

void handle_mouse_event (global_state& state, int idx, int idy) {
    auto mq = 1.0f/8.0f;
    
    auto dx = mq*idx;
	auto dy = mq*idy;
    auto tx = state.player_euler_angles.x - dx;
    auto ty = state.player_euler_angles.y - dy;
          
    state.player_euler_angles.x = std::abs (tx) > 180.0f ? tx - std::copysign (360.0f, tx) : tx;
    state.player_euler_angles.y = glm::clamp (ty, -89.0f, 89.0f);

	auto ey = glm::radians (state.player_euler_angles.x);
	auto ep = glm::radians (state.player_euler_angles.y);
			
	auto fx = cos (ey)*cos(ep);
	auto fy = sin (ey)*cos(ep);
	auto fz = sin (ep);
			
	state.player_forward = glm::normalize (glm::vec3 (fx, fy, fz));
    state.player_upwards = glm::vec3 (0.0f, 0.0f, -1.0f);
    state.player_sideways = glm::normalize (glm::cross (state.player_forward, state.player_upwards));
    state.player_upwards = glm::normalize (glm::cross (state.player_forward, state.player_sideways));
}

bool drain_mouse_events (global_state& state) {
    __block bool didHandleEvent = false;
    [state.pCocoaWindow trackEventsMatchingMask:NSMouseMovedMask|NSLeftMouseDraggedMask|NSRightMouseDraggedMask
        timeout:0
           mode:NSEventTrackingRunLoopMode
        handler:^(NSEvent * _Nonnull event, BOOL * _Nonnull stop) {
            handle_mouse_event(state, [event deltaX], [event deltaY]);
            didHandleEvent = true;
            *stop = YES;
        }
    ];
    return didHandleEvent;
}

void handle_keyboard_event (global_state& state) {

    auto keyboard = SDL_GetKeyboardState (nullptr);
    
    auto kq = 16.0f;
    auto ks = 1.0f - 1.0f/16.0f;
    auto tv = 512.0f;
    
    auto kx = 1.0f*keyboard [SDL_SCANCODE_A]
            - 1.0f*keyboard [SDL_SCANCODE_D] ;
    auto ky = 1.0f*keyboard [SDL_SCANCODE_W]
            - 1.0f*keyboard [SDL_SCANCODE_S] ;
    auto kz = 1.0f*keyboard [SDL_SCANCODE_SPACE]
            - 1.0f*keyboard [SDL_SCANCODE_C] ;

    auto vf = state.player_forward;
    auto vu = state.player_upwards;
    auto vl = state.player_sideways;
    
    auto vt = glm::vec3 (vf*ky + vl*kx + vu*kz);
    
    if (glm::length (vt) > 0) {
        vt = kq*glm::normalize (vt);
    }
    
    state.player_velocity += vt;
    
    if (glm::length (state.player_velocity) > tv) {
        state.player_velocity = glm::normalize (state.player_velocity)*tv;
    }
    
    state.player_position += state.last_deltatime*state.player_velocity;
    state.player_velocity *= ks;
}


bool execute_run_loop (global_state& state) {

    SDL_Event pEvent;
    SDL_GL_SwapWindow (state.pWindow);
    
    drain_mouse_events (state);
    
    while (SDL_PollEvent (&pEvent)) {
        if (SDL_QuitRequested())
            return false;
        
        switch (pEvent.type) {
            SDL_KEYDOWN:
            SDL_KEYUP:
                break;
            default:
                break;
        }
    }
    
    auto current_timestamp = SDL_GetTicks()*0.001;
    state.last_deltatime = current_timestamp - state.last_timestamp;
    state.last_timestamp = current_timestamp;
    
    return true;
}

void teardown_sdl_and_gl (global_state& state) {
    SDL_GL_MakeCurrent (state.pWindow, nullptr);
    SDL_GL_DeleteContext (state.pContext);
    SDL_DestroyWindow (state.pWindow);
}

void teardown_gl_resources (global_state& state) {
    glDeleteProgram (state.gl_program);
    
    CHECK();
}

void build_gl_resources (global_state& state) {
    auto& map_data = *state.map_data;
    
    std::vector<xtk::quake2::bsp_vertex_attribute> buff;
    xtk::quake2::build_bsp_faces (buff, map_data);
    
    glGenBuffers (1u, &state.gl_buffer_vertex);
    glBindBuffer (GL_ARRAY_BUFFER, state.gl_buffer_vertex);
    glBufferData (GL_ARRAY_BUFFER, buff.size()* sizeof (buff [0]), buff.data(), GL_STATIC_DRAW);
    
    glGenVertexArrays (1u, &state.gl_varray);
    glBindVertexArray (state.gl_varray);

    glEnableVertexAttribArray (0u);
    glEnableVertexAttribArray (1u);
    glEnableVertexAttribArray (2u);
    glEnableVertexAttribArray (3u);

    auto tempor = reinterpret_cast<const xtk::quake2::bsp_vertex_attribute*>(0);
    auto stride = (GLsizei)sizeof (xtk::quake2::bsp_vertex_attribute);
    
    glVertexAttribPointer (0u, 3u, GL_FLOAT, GL_FALSE, stride, &tempor->vertex);
    glVertexAttribPointer (1u, 3u, GL_FLOAT, GL_FALSE, stride, &tempor->normal);
    glVertexAttribPointer (2u, 2u, GL_FLOAT, GL_FALSE, stride, &tempor->uv_tex);
    glVertexAttribPointer (3u, 3u, GL_FLOAT, GL_FALSE, stride, &tempor->color);
    
    state.gl_element_count = (GLsizei)buff.size();
    CHECK ();
    
    for (const auto& tex: map_data.texture_info) {
        char buffer [33] = {0};
        
        std::strncpy (buffer, tex.texture_name, 32);
        
        xtk::Debug::log ("%s\n", buffer);
    }
    
}

void setup_glsl_program (global_state& state) {
    std::string outLog;
    state.gl_program = build_gl_shaders ({
        {GL_VERTEX_SHADER, __basic_vertex_source},
        {GL_FRAGMENT_SHADER, __basic_fragment_source}},
        outLog);
    
    if (!state.gl_program) {
        xtk::Debug::log ("%s\n", outLog.c_str ());
    }
    
    glUseProgram (state.gl_program);
    glUniform3f (glGetUniformLocation (state.gl_program, "g_constant_color"), 1.0f, 1.0f, 1.0f);
    glUniform1f (glGetUniformLocation (state.gl_program, "g_constant_alpha"), 1.0f);
    
    CHECK();
}

void update_uniforms (global_state& state) {
	auto g_projection_matrix = glm::perspective (45.0f, 16.0f/9.0f, 1.0f, 10000.0f);
    auto g_model_matrix = glm::mat4 (1.0);
	auto g_view_matrix = glm::lookAt (state.player_position, state.player_position + state.player_forward, state.player_upwards);
	auto g_model_view_projection = g_projection_matrix * g_view_matrix * g_model_matrix;
    auto g_normal_matrix = glm::transpose (glm::inverse (glm::mat3 (g_view_matrix * g_model_matrix)));
        
    glUniformMatrix4fv (glGetUniformLocation (state.gl_program, "g_model_view_projection"), 1u, GL_FALSE, &g_model_view_projection [0][0]);
    glUniformMatrix3fv (glGetUniformLocation (state.gl_program, "g_normal_matrix"), 1u, GL_FALSE, &g_normal_matrix [0][0]);

    CHECK();
}

void draw_frame (global_state& state) {
    auto& map_data = *state.map_data;
    
    glClearColor (0.0f, 0.25f, 0.0f, 1.0f);
    glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    CHECK();
    
    glEnable (GL_DEPTH_TEST);
    //glEnable (GL_CULL_FACE);
    //glCullFace (GL_BACK);
    //glFrontFace (GL_CW);
    
    //glUniform1f (glGetUniformLocation (state.gl_program, "g_constant_alpha"), 1.0f);
    
    //glDisable (GL_DEPTH_TEST);
    //glEnable (GL_BLEND);
    //glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays (GL_TRIANGLES, 0, state.gl_element_count);
    
    glUniform1f (glGetUniformLocation (state.gl_program, "g_constant_alpha"), 1.0f);
    glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays (GL_TRIANGLES, 0, state.gl_element_count);
    
    CHECK();

}

int main (int argc, const char* argv []) try {
    global_state context;
    
    std::ifstream ifs ("data/maps/q2dm1.bsp", std::ios::binary);
    
    if (!ifs) {
        std::cout << "Couldn't open file!\n";
        return -1;
    }
    
    xtk::bsp_data_quake2 map_data (xtk::fetch_data (std::move (ifs)));
    context.map_data = &map_data;
    
    
    setup_sdl_and_gl (context);
    build_gl_resources(context);
    setup_glsl_program (context);
    
    while (execute_run_loop (context)) {
        handle_keyboard_event (context);
        update_uniforms(context);
        draw_frame (context);
    }

    teardown_gl_resources (context);
    teardown_sdl_and_gl (context);

    return 0;
}
catch (std::exception& ex) {
    std::cout << ex.what () << "\n";
    return -1;
}