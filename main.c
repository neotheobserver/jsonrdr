#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>

#include "./raylib/include/raylib.h"
#include "app.h"
#include "./raylib/include/raygui.h"

#define RAYGUI_IMPLEMENTATION
#include "./raylib/include/raygui.h"

#undef RAYGUI_IMPLEMENTATION    
#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "file_dialog.h" 

 app_s app = {0};

int get_screen_width()
{
  if(IsWindowFullscreen()) {
    int monitor = GetCurrentMonitor();
    return GetMonitorWidth(monitor);
  } else {
    return GetScreenWidth();
  }
  
}

int get_screen_height()
{
  if(IsWindowFullscreen()) {
    int monitor = GetCurrentMonitor();
    return GetMonitorHeight(monitor);
  } else {
    return GetScreenHeight();
  }
}


void init_structure(app_s* app, uint64_t file_size, char * file_name) {
    app->file_size = file_size;
    app->file_name = file_name;
    app->contents = NULL;
    app->lines_ptr = NULL;
    app->total_lines = 0;
    app->state = INIT;
    app->matches_count = 0;
    app->matches_ptr = NULL;
    app->current_index = 0;
}

int load_file(app_s* app, float * progress) {
  *progress = 5;
  app->contents = malloc(app->file_size + 1);
  if (NULL == app->contents) {
      printf("Not enough memory. Buy RAM!!!\n");
      return 1;
  }
  *progress = 20;
  uint8_t status = init_app(app, progress);
  
  app->state = RUN;
  return status;
}

void* load_file_thread(void* vargp)
{
  // might seem wierd but did not want to create any new variables
  app.status_code = load_file(&app, (float*) vargp);
  app.state = RUN;
  return NULL;
}

void remove_spaces (char* restrict str_trimmed, const char* restrict str_untrimmed)
{
  while (*str_untrimmed != '\0')
  {
    if(!isspace(*str_untrimmed))
    {
      *str_trimmed = *str_untrimmed;
      str_trimmed++;
    }
    str_untrimmed++;
  }
  *str_trimmed = '\0';
}

void run_app(app_s* app, char* to_search, int* scroll_index, int* active_state, Vector2* scroll) {
  int i = GuiTextInputBox((Rectangle){20, 10, get_screen_width()-40, get_screen_height()*0.1}, "Enter Text to Search...",
                        "", "Search;Cancel", to_search, 10, NULL);

  char * text_to_show = app->matches_ptr == NULL?"":app->lines_ptr[app->matches_ptr[app->current_index]];

  GuiTextBox((Rectangle){5, get_screen_height()*0.12, get_screen_width()-10, get_screen_height()*0.8}, text_to_show, 10, false);
//   Rectangle view = { 0 };
//   GuiScrollPanel(
//               (Rectangle){5, get_screen_height()*0.12, get_screen_width()-10, get_screen_height()*0.8},
//               "Search Result/s",
//               (Rectangle){10, get_screen_height()*0.14, get_screen_width()-20, get_screen_height()*0.78},
//               scroll,
//               &view);
              
//   float wheelMove = GetMouseWheelMove();
//   if (wheelMove != 0) {
//       printf( "Mouse Wheel Move: %f\n", wheelMove);
//   }

//   BeginScissorMode(
//       view.x,
//       view.y,
//       view.width,
//       view.height
//   );
//   DrawText(
//     text_to_show, 
//     view.x - scroll->x + 10, 
//     view.y - scroll->y + 10, 
//     20, 
//     BLACK
// );
//   EndScissorMode();
  char matches_str[64];
  
  snprintf(matches_str, 64, "%lu/%lu",app->current_index,app->matches_count);
  GuiLabel((Rectangle){(get_screen_width()/2)-50, get_screen_height()*0.92, 100, get_screen_height()*0.025}, matches_str); 
  
  if (app->current_index > 0)
  {
    if (GuiButton((Rectangle){(get_screen_width()/2)-100, get_screen_height()*0.96, 100, get_screen_height()*0.03}, "Prev")) {
      app->current_index--;
    }
  }
  if (app->current_index < app->matches_count)
  {
    if(GuiButton((Rectangle){(get_screen_width()/2)+1, get_screen_height()*0.96, 100, get_screen_height()*0.03}, "Next")) {
      app->current_index++;
    }
  }
  
  if (1 == i && (to_search[0] != '\0') && (to_search[1] != '\0') && (to_search[2] != '\0')) {
    GuiLock();
    app->matches_count = 0;
    for (uint64_t i = 0; i < app->total_lines; i++) {
      if (to_search[0] == '\0') {
        break;
      }
      if (strstr(app->lines_ptr[i], to_search) != NULL) {
        app->matches_count++;
      }
    }
    printf("Found occurences: %lu\n", app->matches_count);
    if( app->matches_count > 0) {
      app->matches_ptr = malloc(app->matches_count * sizeof(uint64_t));
      if(NULL == app->matches_ptr)
      {
        printf("OUt of Memory! Buy more RAM!!\n");
        app->status_code = -1;
        return;
      }
      uint64_t index = 0;
      for (uint64_t i = 0; i < app->total_lines; i++) {
      
        if (strstr(app->lines_ptr[i], to_search) != NULL) {
          app->matches_ptr[index++] = i;
        }
      }
    }
    GuiUnlock();
  } else if (2 == i) {
    memset(to_search, 0, 512);
    app->matches_count = 0;
    app->current_index = 0;
    free(app->matches_ptr);
    app->matches_ptr = NULL;
  }
}

int main(int argc, char *argv[]) {

  InitWindow(1280, 920, "jsonrdr - read large json files");
  SetTargetFPS(60);

  GuiWindowFileDialogState fileDialogState = InitGuiWindowFileDialog(GetWorkingDirectory());  
  struct stat filestats;
  char fileNameToLoad[1024] = { 0 };
  char to_search[512] = {0};
  int scroll_index = 0;
  int active_state = 0;
  pthread_t thread_id;
  float progress = 0;
  char progress_text[8] = {0};
  bool showMessageBox = true;
  Vector2 scroll = {10, get_screen_height()*0.14};
  while (!WindowShouldClose()) {

    BeginDrawing();
    ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
    if(IsKeyReleased(KEY_F)) {
      // int monitor = GetCurrentMonitor();
      // SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
      ToggleFullscreen();

    }
    switch (app.state)
    {
      case INIT:
      if (fileDialogState.SelectFilePressed) {
        showMessageBox = true;
        if (IsFileExtension(fileDialogState.fileNameText, ".json")) {
          strcpy(fileNameToLoad, TextFormat("%s" PATH_SEPERATOR "%s", fileDialogState.dirPathText, fileDialogState.fileNameText));
          fileDialogState.SelectFilePressed = false;
          app.state = START;
        } else {
          if (showMessageBox) {
              int result = GuiMessageBox((Rectangle){ 85, 70, 250, 100 },
                  "#191#Error", "Incorecct File Format!", "Ok");
              if (result >= 0) {
                fileDialogState.SelectFilePressed = false;
                showMessageBox = false;
              }
          }
        }
      }
   
      if (fileDialogState.windowActive) GuiLock();
      if (GuiButton((Rectangle){(get_screen_width()/2)-70,(get_screen_height()/2)-14, 140, 30}, GuiIconText(ICON_FILE_OPEN, "Open File"))) fileDialogState.windowActive = true;
      GuiUnlock();

      GuiWindowFileDialog(&fileDialogState);
      break;
      case START:
        stat(fileNameToLoad, &filestats);
        printf("Hello World!: %lu\n", filestats.st_size);
        init_structure(&app, filestats.st_size, fileNameToLoad);
        pthread_create(&thread_id, NULL, load_file_thread, &progress);
        app.state = LOAD;
      break;
      case LOAD:
        snprintf(progress_text, 5, "%0.2f",progress);
        GuiProgressBar(
          (Rectangle){(get_screen_width()/2)-200,(get_screen_height()/2)-10, 400, 20},
          "Analyzing File",
          progress_text, 
          &progress,
          0.0,
          100
        );
      break;
      case RUN:
        pthread_join(thread_id, NULL);
        if (app.status_code != 0){
          printf("Error with error code %d occured\n", app.status_code);
        } else {
          run_app(&app, to_search,&scroll_index, &active_state, &scroll);
        }
        
      break;
      case EXIT:
      break;
      default:
      break;
    }
    
    EndDrawing();
  }

  CloseWindow();
  
  free(app.contents);

  return 0;
}
