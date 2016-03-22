//
//  main.m
//  Bsp_Take0
//
//  Created by Aleksandras Sevcenko on 3/16/16.
//  Copyright © 2016 Aleksandr Ševčenko. All rights reserved.
//

#include "debug.hpp"
#include "bsp.hpp"
#include "q2pak.hpp"
#include "cocoa_mouse_workaround.hpp"

#include <string>
#include <regex>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <unordered_map>

#include <SDL2/SDL.h>

#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/ext.hpp>

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
    layout (location = 2) in vec3 vs_vertex_uv_tex;
    layout (location = 3) in vec3 vs_vertex_color;
    layout (location = 4) in vec2 vs_vertex_uvlmap;

    out vec4 fs_vertex_position;
    out vec3 fs_vertex_normal;
    out vec3 fs_vertex_uv_tex;
	out vec2 fs_vertex_uvlmap;
    out vec3 fs_vertex_color;

    void main () {
        fs_vertex_position = g_model_view_projection * vec4 (vs_vertex_position, 1.0);
        fs_vertex_normal = g_normal_matrix * vs_vertex_position;
        fs_vertex_uv_tex = vs_vertex_uv_tex;
        fs_vertex_uvlmap = vs_vertex_uvlmap;
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
    uniform sampler2DArray g_texture_array;
    uniform sampler2D g_lightmap_array;

    in vec4 fs_vertex_position;
    in vec3 fs_vertex_normal;
    in vec3 fs_vertex_uv_tex;
    in vec2 fs_vertex_uvlmap;
    in vec3 fs_vertex_color;

    layout (location = 0) out vec4 out_fragment_color;

    void main () {
        vec3 lmapc = texture (g_lightmap_array, fs_vertex_uvlmap.xy).rgb;
        vec3 color = texture (g_texture_array, fs_vertex_uv_tex).rgb * lmapc;
        out_fragment_color = pow (vec4 (mix (1 - color, color, g_constant_alpha), 1.0f), vec4 (1.0/1.5));
    }
)";

struct global_state {
    SDL_Window* pWindow;
    SDL_GLContext pContext;
    
    const xtk::bsp_data* map_data;
    
    double last_timestamp;
    double last_deltatime;
        
    GLsizei gl_element_count;
    GLuint gl_buffer_vertex;
    GLuint gl_buffer_edge;
    GLuint gl_varray;
    GLuint gl_program;
    GLuint gl_textures;
    GLuint gl_lightmaps;
    
    glm::vec3 player_velocity;
	glm::vec3 player_position = {100.0f, 630.0f, 630.0f};
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
        CHECK ();
        
        if (!param) {
            glGetShaderiv (uObject, GL_INFO_LOG_LENGTH, &param);
            auto infoLog = std::make_unique<GLchar []> (param + 1);
            glGetShaderInfoLog (uObject, param + 1, &param, infoLog.get ());
            
            glDeleteShader (uObject);
            
            CHECK ();
            
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
    CHECK ();
    
    if (!param) {
        glGetProgramiv (uProgram, GL_INFO_LOG_LENGTH, &param);
        auto infoLog = std::make_unique<GLchar []> (param + 1);
        glGetProgramInfoLog (uProgram, param + 1, &param, infoLog.get ());
        
        glDeleteProgram (uProgram);
        
        CHECK ();

        log.append ("\nProgram link error: \n");
        log.append (infoLog.get ());
        log.append ("\n");
        
        return 0u;
    }
    CHECK ();
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
    SDL_GL_SetAttribute (SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute (SDL_GL_MULTISAMPLESAMPLES, 16);
    
    state.pContext = SDL_GL_CreateContext (state.pWindow);
    SDL_GL_MakeCurrent (state.pWindow, state.pContext);
    
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
    
    #ifdef __APPLE__
    cocoa_drain_mouse_events (state.pWindow, [&state] (int dx, int dy){
        handle_mouse_event (state, dx, dy);
    });
    #endif
    
    while (SDL_PollEvent (&pEvent)) {
        if (SDL_QuitRequested())
            return false;
        
        switch (pEvent.type) {
            case SDL_MOUSEMOTION:
                handle_mouse_event(state, pEvent.motion.xrel, pEvent.motion.yrel);
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
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

void stretch (const xtk::bitmap& _input, xtk::bitmap::value_type* buffer, std::size_t w, std::size_t h) {
    auto dx = 1.0f / w;
    auto dy = 1.0f / h;

    for (auto y = 0; y < h; ++y) {
        for (auto x = 0; x < w; ++x) {
            buffer [w*y + x] = _input (x*dx, y*dy);
        }
    }
}

void build_gl_resources (global_state& state) {

    const auto& map_data = *state.map_data;
    const auto& vertexes = map_data.vertexes;
    const auto& textures = map_data.textures;
    const auto& lightmaps = map_data.lightmaps;
    
    glGenBuffers (1u, &state.gl_buffer_vertex);
    glBindBuffer (GL_ARRAY_BUFFER, state.gl_buffer_vertex);
    glBufferData (GL_ARRAY_BUFFER, vertexes.size() * sizeof (vertexes [0]), vertexes.data(), GL_STATIC_DRAW);
    
    glGenVertexArrays (1u, &state.gl_varray);
    glBindVertexArray (state.gl_varray);

    glEnableVertexAttribArray (0u);
    glEnableVertexAttribArray (1u);
    glEnableVertexAttribArray (2u);
    glEnableVertexAttribArray (3u);
    glEnableVertexAttribArray (4u);

    auto tempor = reinterpret_cast<const xtk::bsp_data::vertex_attribute*>(0);
    auto stride = (GLsizei)sizeof (xtk::bsp_data::vertex_attribute);
    
    glVertexAttribPointer (0u, 3u, GL_FLOAT, GL_FALSE, stride, &tempor->vertex);
    glVertexAttribPointer (1u, 3u, GL_FLOAT, GL_FALSE, stride, &tempor->normal);
    glVertexAttribPointer (2u, 3u, GL_FLOAT, GL_FALSE, stride, &tempor->uvtex);
    glVertexAttribPointer (3u, 3u, GL_FLOAT, GL_FALSE, stride, &tempor->color);
    glVertexAttribPointer (4u, 2u, GL_FLOAT, GL_FALSE, stride, &tempor->uvlmap);
    
    state.gl_element_count = (GLsizei)vertexes.size();
    CHECK ();
    
    glActiveTexture(GL_TEXTURE0);
    glGenTextures (1u, &state.gl_textures);
    glBindTexture (GL_TEXTURE_2D_ARRAY, state.gl_textures);
    glTexParameteri (GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage3D (GL_TEXTURE_2D_ARRAY, 0, GL_RGBA,
        (GLsizei)textures.width (),
        (GLsizei)textures.height (),
        (GLsizei)textures.depth (),
        0, GL_RGBA, GL_UNSIGNED_BYTE,
        textures.data());
    glGenerateMipmap (GL_TEXTURE_2D_ARRAY);    
    CHECK();
    
    glActiveTexture(GL_TEXTURE1);
    glGenTextures (1u, &state.gl_lightmaps);
    glBindTexture (GL_TEXTURE_2D, state.gl_lightmaps);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA,
        (GLsizei)lightmaps.width (),
        (GLsizei)lightmaps.height (),
        0, GL_RGBA, GL_UNSIGNED_BYTE,
        lightmaps.data());
    glGenerateMipmap (GL_TEXTURE_2D);
    CHECK();
}

void setup_glsl_program (global_state& state) {
    std::string outLog;
    xtk::Debug::log ("Loading shaders...\n");
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
    
    glClearColor (1.0f, 0.0f, 1.0f, 1.0f);
    glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    CHECK();
    
    glEnable (GL_DEPTH_TEST);
    glEnable (GL_CULL_FACE);
    glCullFace (GL_BACK);
    glFrontFace (GL_CW);
    
    CHECK ();
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D_ARRAY, state.gl_textures);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture (GL_TEXTURE_2D, state.gl_lightmaps);
    
    glUniform1f (glGetUniformLocation (state.gl_program, "g_constant_alpha"), 1.0f);
    
    glUniform1i (glGetUniformLocation (state.gl_program, "g_texture_array"), 0);
    glUniform1i (glGetUniformLocation (state.gl_program, "g_lightmap_array"), 1);
    
    CHECK ();
    
    glBindVertexArray (state.gl_varray);
    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays (GL_TRIANGLES, 0, state.gl_element_count);
    
    CHECK ();

//    glUniform1f (glGetUniformLocation (state.gl_program, "g_constant_alpha"), 0.0f);
//    glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
//    glDrawArrays (GL_TRIANGLES, 0, state.gl_element_count);
    
    CHECK();

}

int main (int argc, const char* argv []) try {

    global_state context;
    
    xtk::q2pak& pack_manager = xtk::q2pak::shared();
    
    pack_manager.mount ("data/pak2.pak");
    pack_manager.mount ("data/pak1.pak");
    pack_manager.mount ("data/pak0.pak");
    
    auto map_data = xtk::bsp_decode (pack_manager, "maps/q2dm1.bsp");
    
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
catch (const std::exception& ex) {
    std::cout << "ERROR : " << typeid(ex).name() << " : " << ex.what () << "\n";
    return -1;
}