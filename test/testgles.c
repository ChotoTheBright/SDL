/*
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
#include <stdlib.h>

#include <SDL3/SDL_test_common.h>
#include <SDL3/SDL_main.h>

#if defined(SDL_PLATFORM_IOS) || defined(SDL_PLATFORM_ANDROID)
#define HAVE_OPENGLES
#endif

#ifdef HAVE_OPENGLES

#include <SDL3/SDL_opengles.h>

static SDLTest_CommonState *state;
static SDL_GLContext *context = NULL;
static int depth = 16;

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    int i;

    if (context) {
        for (i = 0; i < state->num_windows; i++) {
            if (context[i]) {
                SDL_GL_DestroyContext(context[i]);
            }
        }

        SDL_free(context);
    }

    SDLTest_CommonQuit(state);
    /* Let 'main()' return normally */
    if (rc != 0) {
        exit(rc);
    }
}

static void
Render(void)
{
    static GLubyte color[8][4] = { { 255, 0, 0, 0 },
                                   { 255, 0, 0, 255 },
                                   { 0, 255, 0, 255 },
                                   { 0, 255, 0, 255 },
                                   { 0, 255, 0, 255 },
                                   { 255, 255, 255, 255 },
                                   { 255, 0, 255, 255 },
                                   { 0, 0, 255, 255 } };
    static GLfloat cube[8][3] = { { 0.5, 0.5, -0.5 },
                                  { 0.5f, -0.5f, -0.5f },
                                  { -0.5f, -0.5f, -0.5f },
                                  { -0.5f, 0.5f, -0.5f },
                                  { -0.5f, 0.5f, 0.5f },
                                  { 0.5f, 0.5f, 0.5f },
                                  { 0.5f, -0.5f, 0.5f },
                                  { -0.5f, -0.5f, 0.5f } };
    static GLubyte indices[36] = { 0, 3, 4,
                                   4, 5, 0,
                                   0, 5, 6,
                                   6, 1, 0,
                                   6, 7, 2,
                                   2, 1, 6,
                                   7, 4, 3,
                                   3, 2, 7,
                                   5, 4, 7,
                                   7, 6, 5,
                                   2, 3, 1,
                                   3, 0, 1 };

    /* Do our drawing, too. */
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Draw the cube */
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, color);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, cube);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, indices);

    glMatrixMode(GL_MODELVIEW);
    glRotatef(5.0, 1.0, 1.0, 1.0);
}

int main(int argc, char *argv[])
{
    int fsaa, accel;
    int value;
    int i, done;
    const SDL_DisplayMode *mode;
    SDL_Event event;
    Uint32 then, now, frames;

    /* Initialize parameters */
    fsaa = 0;
    accel = 0;

    /* Initialize test framework */
    state = SDLTest_CommonCreateState(argv, SDL_INIT_VIDEO);
    if (!state) {
        return 1;
    }

    for (i = 1; i < argc;) {
        int consumed;

        consumed = SDLTest_CommonArg(state, i);
        if (consumed == 0) {
            if (SDL_strcasecmp(argv[i], "--fsaa") == 0) {
                ++fsaa;
                consumed = 1;
            } else if (SDL_strcasecmp(argv[i], "--accel") == 0) {
                ++accel;
                consumed = 1;
            } else if (SDL_strcasecmp(argv[i], "--zdepth") == 0) {
                i++;
                if (!argv[i]) {
                    consumed = -1;
                } else {
                    char *endptr = NULL;
                    depth = (int)SDL_strtol(argv[i], &endptr, 0);
                    if (endptr != argv[i] && *endptr == '\0') {
                        consumed = 1;
                    } else {
                        consumed = -1;
                    }
                }
            } else {
                consumed = -1;
            }
        }
        if (consumed < 0) {
            static const char *options[] = { "[--fsaa]", "[--accel]", "[--zdepth %d]", NULL };
            SDLTest_CommonLogUsage(state, argv[0], options);
            quit(1);
        }
        i += consumed;
    }

    /* Set OpenGL parameters */
    state->window_flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS;
    state->gl_red_size = 5;
    state->gl_green_size = 5;
    state->gl_blue_size = 5;
    state->gl_depth_size = depth;
    state->gl_major_version = 1;
    state->gl_minor_version = 1;
    state->gl_profile_mask = SDL_GL_CONTEXT_PROFILE_ES;
    if (fsaa) {
        state->gl_multisamplebuffers = 1;
        state->gl_multisamplesamples = fsaa;
    }
    if (accel) {
        state->gl_accelerated = 1;
    }
    if (!SDLTest_CommonInit(state)) {
        quit(2);
    }

    context = (SDL_GLContext *)SDL_calloc(state->num_windows, sizeof(*context));
    if (!context) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Out of memory!");
        quit(2);
    }

    /* Create OpenGL ES contexts */
    for (i = 0; i < state->num_windows; i++) {
        context[i] = SDL_GL_CreateContext(state->windows[i]);
        if (!context[i]) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_GL_CreateContext(): %s", SDL_GetError());
            quit(2);
        }
    }

    SDL_GL_SetSwapInterval(state->render_vsync);

    mode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
    if (mode) {
        SDL_Log("Screen bpp: %d", SDL_BITSPERPIXEL(mode->format));
        SDL_Log("%s", "");
    }
    SDL_Log("Vendor     : %s", glGetString(GL_VENDOR));
    SDL_Log("Renderer   : %s", glGetString(GL_RENDERER));
    SDL_Log("Version    : %s", glGetString(GL_VERSION));
    SDL_Log("Extensions : %s", glGetString(GL_EXTENSIONS));
    SDL_Log("%s", "");

    if (SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &value)) {
        SDL_Log("SDL_GL_RED_SIZE: requested %d, got %d", 5, value);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get SDL_GL_RED_SIZE: %s",
                     SDL_GetError());
    }
    if (SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &value)) {
        SDL_Log("SDL_GL_GREEN_SIZE: requested %d, got %d", 5, value);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get SDL_GL_GREEN_SIZE: %s",
                     SDL_GetError());
    }
    if (SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &value)) {
        SDL_Log("SDL_GL_BLUE_SIZE: requested %d, got %d", 5, value);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get SDL_GL_BLUE_SIZE: %s",
                     SDL_GetError());
    }
    if (SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &value)) {
        SDL_Log("SDL_GL_DEPTH_SIZE: requested %d, got %d", depth, value);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get SDL_GL_DEPTH_SIZE: %s",
                     SDL_GetError());
    }
    if (fsaa) {
        if (SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &value)) {
            SDL_Log("SDL_GL_MULTISAMPLEBUFFERS: requested 1, got %d", value);
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get SDL_GL_MULTISAMPLEBUFFERS: %s",
                         SDL_GetError());
        }
        if (SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &value)) {
            SDL_Log("SDL_GL_MULTISAMPLESAMPLES: requested %d, got %d", fsaa,
                    value);
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get SDL_GL_MULTISAMPLESAMPLES: %s",
                         SDL_GetError());
        }
    }
    if (accel) {
        if (SDL_GL_GetAttribute(SDL_GL_ACCELERATED_VISUAL, &value)) {
            SDL_Log("SDL_GL_ACCELERATED_VISUAL: requested 1, got %d", value);
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get SDL_GL_ACCELERATED_VISUAL: %s",
                         SDL_GetError());
        }
    }

    /* Set rendering settings for each context */
    for (i = 0; i < state->num_windows; ++i) {
        float aspectAdjust;

        if (!SDL_GL_MakeCurrent(state->windows[i], context[i])) {
            SDL_Log("SDL_GL_MakeCurrent(): %s", SDL_GetError());

            /* Continue for next window */
            continue;
        }

        aspectAdjust = (4.0f / 3.0f) / ((float)state->window_w / state->window_h);
        glViewport(0, 0, state->window_w, state->window_h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrthof(-2.0, 2.0, -2.0 * aspectAdjust, 2.0 * aspectAdjust, -20.0, 20.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glShadeModel(GL_SMOOTH);
    }

    /* Main render loop */
    frames = 0;
    then = SDL_GetTicks();
    done = 0;
    while (!done) {
        /* Check for events */
        ++frames;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                for (i = 0; i < state->num_windows; ++i) {
                    if (event.window.windowID == SDL_GetWindowID(state->windows[i])) {
                        if (!SDL_GL_MakeCurrent(state->windows[i], context[i])) {
                            SDL_Log("SDL_GL_MakeCurrent(): %s", SDL_GetError());
                            break;
                        }
                        /* Change view port to the new window dimensions */
                        glViewport(0, 0, event.window.data1, event.window.data2);
                        /* Update window content */
                        Render();
                        SDL_GL_SwapWindow(state->windows[i]);
                        break;
                    }
                }
            }
            SDLTest_CommonEvent(state, &event, &done);
        }
        for (i = 0; i < state->num_windows; ++i) {
            if (state->windows[i] == NULL) {
                continue;
            }
            if (!SDL_GL_MakeCurrent(state->windows[i], context[i])) {
                SDL_Log("SDL_GL_MakeCurrent(): %s", SDL_GetError());

                /* Continue for next window */
                continue;
            }
            Render();
            SDL_GL_SwapWindow(state->windows[i]);
        }
    }

    /* Print out some timing information */
    now = SDL_GetTicks();
    if (now > then) {
        SDL_Log("%2.2f frames per second",
                ((double)frames * 1000) / (now - then));
    }
#ifndef SDL_PLATFORM_ANDROID
    quit(0);
#endif
    return 0;
}

#else /* HAVE_OPENGLES */

int main(int argc, char *argv[])
{
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No OpenGL ES support on this system");
    return 1;
}

#endif /* HAVE_OPENGLES */
