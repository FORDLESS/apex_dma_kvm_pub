#include "Client/main.h"
#include "Game.h"
#include "apex_sky.h"
#include "vector.h"
#include <array>
#include <cassert>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib> // For the system() function
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map> // Include the unordered_map header
#include <vector>
// this is a test, with seconds
Memory apex_mem;

// Just setting things up, dont edit.
bool active = true;
aimbot_state_t aimbot;
int team_player = 0;
const int toRead = 100;
bool trigger_ready = false;
extern Vector aim_target; // for esp
int map_testing_local_team = 0;

// Removed but not all the way, dont edit.
int glowtype;
int glowtype2;
// float triggerdist = 50.0f;
bool actions_t = false;
bool cactions_t = false;
bool updateInsideValue_t = false;
bool TriggerBotRun_t = false;
bool terminal_t = false;
bool overlay_t = false;
bool esp_t = false;
bool aim_t = false;
bool vars_t = false;
bool item_t = false;
bool control_t = false;
uint64_t g_Base;
bool next2 = false;
bool valid = false;
extern float bulletspeed;
extern float bulletgrav;
Vector esp_local_pos;
int local_held_id = 2147483647;
uint32_t local_weapon_id = 2147483647;
int playerentcount = 61;
int itementcount = 10000;
int map = 0;
std::vector<TreasureClue> treasure_clues;
std::map<uint64_t, uint64_t> centity_to_index; // Map centity to entity index

//^^ Don't EDIT^^

// [del]CONFIG AREA, you must set all the true/false to what you want.[/del]
// No longer needed here. Edit your configuration file!

std::vector<uint64_t> wish_list{191, 209, 210, 220,          234,
                                242, 258, 260, 429496729795, 52776987629977800};

uint32_t button_state[4];
bool isPressed(uint32_t button_code) {
  return (button_state[static_cast<uint32_t>(button_code) >> 5] &
          (1 << (static_cast<uint32_t>(button_code) & 0x1f))) != 0;
}

void memory_io_panic(const char *info) {
  quit_tui_menu();
  std::cout << "Error " << info << std::endl;
  exit(0);
}

// Define rainbow color function
void rainbowColor(int frame_number, std::array<float, 3> &colors) {
  const float frequency = 0.1; // Adjust the speed of color change
  const float amplitude = 0.5; // Adjust the amplitude of color change

  // Use the sine function to generate rainbow color variation
  float r = sin(frequency * frame_number + 0) * amplitude + 0.5;
  float g = sin(frequency * frame_number + 2) * amplitude + 0.5;
  float b = sin(frequency * frame_number + 4) * amplitude + 0.5;

  // Clamp the colors to the range [0, 1]
  colors[0] = fmax(0, fmin(1, r));
  colors[1] = fmax(0, fmin(1, g));
  colors[2] = fmax(0, fmin(1, b));
}

void TriggerBotRun() {
  // 设置随机数生成器
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(150, 300);  // 正常或稍快的反应时间
  // 生成随机时间间隔，防止行为检测
  int randomInterval = dis(gen);
  std::this_thread::sleep_for(std::chrono::milliseconds(randomInterval));
  apex_mem.Write<int>(g_Base + OFFSET_IN_ATTACK + 0x8, 5);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  apex_mem.Write<int>(g_Base + OFFSET_IN_ATTACK + 0x8, 4);
  // printf("TriggerBotRun\n");
}
bool IsInCrossHair(Entity &target) {
  static uintptr_t last_t = 0;
  static float last_crosshair_target_time = -1.f;
  float now_crosshair_target_time = target.lastCrossHairTime();
  bool is_trigger = false;
  if (last_t == target.ptr) {
    if (last_crosshair_target_time != -1.f) {
      if (now_crosshair_target_time > last_crosshair_target_time) {
        is_trigger = true;
        // printf("Trigger\n");
        last_crosshair_target_time = -1.f;
      } else {
        is_trigger = false;
        last_crosshair_target_time = now_crosshair_target_time;
      }
    } else {
      is_trigger = false;
      last_crosshair_target_time = now_crosshair_target_time;
    }
  } else {
    last_t = target.ptr;
    last_crosshair_target_time = -1.f;
  }
  return is_trigger;
}

// Used to change things on a timer
/* unsigned char insidevalueItem = 1;
void updateInsideValue()
{
        updateInsideValue_t = true;
        while (updateInsideValue_t)
        {
                insidevalueItem++;
                insidevalueItem %= 256;
                std::this_thread::sleep_for(std::chrono::seconds(2));
                printf("smooth: %f\n", smooth);
                printf("bone: %i\n", bone);
                printf("glowrnot: %f\n", glowrnot);
                printf("glowgnot: %f\n", glowgnot);
                printf("glowbnot: %f\n", glowbnot);


        }
        updateInsideValue_t = false;
} */

// Visual check and aim check.?
float lastvis_esp[toRead];
float lastvis_aim[toRead];
//std::set<uintptr_t> tmp_specs;
std::vector<Entity> spectators, allied_spectators;
std::mutex spectatorsMtx;

void MapRadarTesting() {
  uintptr_t pLocal;
  apex_mem.Read<uint64_t>(g_Base + OFFSET_LOCAL_ENT, pLocal);
  int dt;
  apex_mem.Read<int>(pLocal + OFFSET_TEAM, dt);
  map_testing_local_team = dt;

  for (uintptr_t i = 0; i <= 80000; i++) {
    apex_mem.Write<int>(pLocal + OFFSET_TEAM, 1);
  }

  for (uintptr_t i = 0; i <= 80000; i++) {
    apex_mem.Write<int>(pLocal + OFFSET_TEAM, dt);
  }
  map_testing_local_team = 0;
}

uint64_t PlayerLocal;
int PlayerLocalTeamID;
int EntTeam;
int LocTeam;

void ClientActions() {
  cactions_t = true;
  while (cactions_t) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    while (g_Base != 0) {
      const auto g_settings = global_settings();

      // read player ptr
      uint64_t local_player_ptr = 0;
      apex_mem.Read<uint64_t>(g_Base + OFFSET_LOCAL_ENT, local_player_ptr);

      // read game states
      apex_mem.Read<typeof(button_state)>(g_Base + OFFSET_INPUT_SYSTEM + 0xb0,
                                          button_state);

      int attack_state = 0, zoom_state = 0, tduck_state = 0, jump_state = 0,
          force_jump = 0, force_toggle_duck = 0, force_duck = 0,
          curFrameNumber = 0;
      apex_mem.Read<int>(g_Base + OFFSET_IN_ATTACK, attack_state);     // 108开火
      apex_mem.Read<int>(g_Base + OFFSET_IN_ZOOM, zoom_state);         // 109瞄准
      apex_mem.Read<int>(g_Base + OFFSET_IN_TOGGLE_DUCK, tduck_state); // 61
      apex_mem.Read<int>(g_Base + OFFSET_IN_JUMP, jump_state);
      apex_mem.Read<int>(g_Base + OFFSET_IN_JUMP + 0x8, force_jump);
      apex_mem.Read<int>(g_Base + OFFSET_IN_TOGGLE_DUCK + 0x8,
                         force_toggle_duck);
      apex_mem.Read<int>(g_Base + OFFSET_IN_DUCK + 0x8, force_duck);
      apex_mem.Read<int>(g_Base + OFFSET_GLOBAL_VARS + 0x0008,
                         curFrameNumber); // GlobalVars + 0x0008

      float world_time, traversal_start_time, traversal_progress;
      if (!apex_mem.Read<float>(local_player_ptr + OFFSET_TIME_BASE,
                                world_time)) {
        //memory_io_panic("read time_base");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        break;
      }
      if (!apex_mem.Read<float>(local_player_ptr + OFFSET_TRAVERSAL_STARTTIME,
                                traversal_start_time)) {
        // memory_io_panic("read traversal_starttime");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        break;
      }
      if (!apex_mem.Read<float>(local_player_ptr + OFFSET_TRAVERSAL_PROGRESS,
                                traversal_progress)) {
        memory_io_panic("read traversal_progress");
      }

      //   printf("Travel Time: %f\n", traversal_progress);
      //   printf("Cur Frame: %i\n", curFrameNumber);
      //   printf("Jump Value: %i\n", jump_state);
      //   printf("Jump Value: %i\n", force_jump);
      //   printf("ToggleDuck Value: %i\n", force_toggle_duck);
      //   printf("Duck Value: %i\n", force_duck);

      if (g_settings.super_key_toggle) {
        /** SuperGlide
         * https://www.unknowncheats.me/forum/apex-legends/578160-external-auto-superglide-3.html
         */
        float hang_on_wall = world_time - traversal_start_time;

        static float start_jump_time = 0;
        static bool start_sg = false;
        static std::chrono::milliseconds last_sg_finish;

        float hang_start, hang_cancel, trav_start, hang_max, action_interval;
        int release_wait;
        {
          // for 75 fps
          hang_start = 0.1;
          hang_cancel = 0.12;
          trav_start = 0.87;
          hang_max = 1.5;
          action_interval = 0.011;
          release_wait = 50;
          if (abs(g_settings.game_fps - 144.0) <
              abs(g_settings.game_fps - 75.0)) {
            // for 144 fps
            hang_start = 0.05;
            hang_cancel = 0.07;
            trav_start = 0.90;
            hang_max = 0.75;
            action_interval = 0.007;
            release_wait = 25;
            if (abs(g_settings.game_fps - 240.0) <
                abs(g_settings.game_fps - 144.0)) {
              // for 240 fps
              hang_start = 0.033;
              hang_cancel = 0.04;
              trav_start = 0.95;
              hang_max = 0.2;
              action_interval = 0.004;
              release_wait = 20;
            }
          }
        }

        if (hang_on_wall > hang_start) {
          if (hang_on_wall < hang_cancel) {
            apex_mem.Write<int>(g_Base + OFFSET_IN_JUMP + 0x8, 4);
          }
          if (traversal_progress > trav_start && hang_on_wall < hang_max &&
              !start_sg) {
            std::chrono::milliseconds now_ms =
                duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
            if ((now_ms - last_sg_finish).count() > 320 && jump_state > 0) {
              // start SG
              start_jump_time = world_time;
              start_sg = true;
            }
          }
        }
        if (start_sg) {
          // press button
          // g_logger += "sg Press jump\n";
          apex_mem.Write<int>(g_Base + OFFSET_IN_JUMP + 0x8, 5);

          float current_time;
          while (true) {
            if (apex_mem.Read<float>(local_player_ptr + OFFSET_TIME_BASE,
                                     current_time)) {
              if (current_time - start_jump_time < action_interval) {
                // keep looping
              } else {
                break;
              }
            }
          }
          apex_mem.Write<int>(g_Base + OFFSET_IN_DUCK + 0x8, 6);
          std::this_thread::sleep_for(std::chrono::milliseconds(release_wait));
          apex_mem.Write<int>(g_Base + OFFSET_IN_JUMP + 0x8, 4);
          // Write<int>(g_Base + OFFSET_IN_DUCK + 0x8, 4);
          last_sg_finish = duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch());
          // g_logger += "sg\n";
          start_sg = false;
        }
      }

      { /* calc game fps */
        static int last_checkpoint_frame = 0;
        static std::chrono::milliseconds checkpoint_time;
        if (g_settings.calc_game_fps && curFrameNumber % 100 == 0) {
          std::chrono::milliseconds ms =
              duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch());
          int delta_frame = curFrameNumber - last_checkpoint_frame;
          if (delta_frame > 90 && delta_frame < 120) {
            auto duration = ms - checkpoint_time;
            auto settings_state = g_settings;
            settings_state.game_fps = delta_frame * 1000.0f / duration.count();
            update_settings(settings_state);
          }
          last_checkpoint_frame = curFrameNumber;
          checkpoint_time = ms;
        }
      }

      // printf("Minimap: %ld\n", minimap);
      // apex_mem.Write(LocalPlayer + 0x270 , 1);

      /*
      108 Left mouse button (mouse1)
      109 Right mouse button (mouse2)
      110 Middle mouse button (mouse3)
      111 Side mouse button (mouse4)
      112 Side mouse button (mouse5)

      79 SHIFT key
      81 ALT key
      83 CTRL key

      1 KEY_0
      2 KEY_1
      3 KEY_2
      4 KEY_3
      5 KEY_4
      6 KEY_5
      7 KEY_6
      8 KEY_7
      9 KEY_8
      10 KEY_9

      11 KEY_A
      12 KEY_B
      13 KEY_C
      14 KEY_D
      15 KEY_E
      16 KEY_F
      17 KEY_G
      18 KEY_H
      19 KEY_I
      20 KEY_J
      21 KEY_K
      22 KEY_L
      23 KEY_M
      24 KEY_N
      25 KEY_O
      26 KEY_P
      27 KEY_Q
      28 KEY_R
      29 KEY_S
      30 KEY_T
      31 KEY_U
      32 KEY_V
      33 KEY_W
      34 KEY_X
      35 KEY_Y
      36 KEY_Z


      37 KEY_PAD_0
      38 KEY_PAD_1
      39 KEY_PAD_2
      40 KEY_PAD_3
      41 KEY_PAD_4
      42 KEY_PAD_5
      43 KEY_PAD_6
      44 KEY_PAD_7
      45 KEY_PAD_8
      46 KEY_PAD_9
      47 KEY_PAD_DIVIDE
      48 KEY_PAD_MULTIPLY
      49 KEY_PAD_MINUS
      50 KEY_PAD_PLUS
      51 KEY_PAD_ENTER
      52 KEY_PAD_DECIMAL


      65 KEY_SPACE
      67 KEY_TAB
      68 KEY_CAPSLOCK
      69 KEY_NUMLOCK
      70 KEY_ESCAPE
      71 KEY_SCROLLLOCK
      72 KEY_INSERT
      73 KEY_DELETE
      74 KEY_HOME
      75 KEY_END
      76 KEY_PAGEUP
      77 KEY_PAGEDOWN
      78 KEY_BREAK


      88 KEY_UP
      89 KEY_LEFT
      90 KEY_DOWN
      91 KEY_RIGHT


      92 KEY_F1
      93 KEY_F2
      94 KEY_F3
      95 KEY_F4
      96 KEY_F5
      97 KEY_F6
      98 KEY_F7
      99 KEY_F8
      100 KEY_F9
      101 KEY_F10
      102 KEY_F11
      103 KEY_F12
      */

      if (local_held_id == -251) {
        if ((g_settings.no_nade_aim && zoom_state == 0) ||  //手雷右键瞄准
            (!g_settings.no_nade_aim && zoom_state > 0)) {  //右键取消手雷自瞄
          aimbot.gun_safety = true;
        } else {
          aimbot.gun_safety = false;
        }
      }

      if (g_settings.keyboard) {
        if (isPressed(g_settings.aimbot_hot_key_1) ||
            isPressed(g_settings.aimbot_hot_key_2)) // Left and Right click(add smooth later)
        {
          aimbot.aiming = true;
        } else {
          aimbot.aiming = false;
        }
        if (isPressed(g_settings.aimbot_hot_key_2)) {
            aimbot.smooth = g_settings.smooth - 30;
        }
        else {
            aimbot.smooth = g_settings.smooth;
        }
      }
      if (g_settings.gamepad) {
        // attackState == 120 || zoomState == 119
        if (attack_state > 0 || zoom_state > 0) {
          aimbot.aiming = true;
        } else {
          aimbot.aiming = false;
        }
      }
      bool triggerbot_shotgun;
      switch (local_weapon_id) {
      case idweapon_eva8:
      case idweapon_mastiff:
      case idweapon_mozambique:
      case idweapon_peacekeeper:
          triggerbot_shotgun = true;
          break;
      default:
          triggerbot_shotgun = false;
      }
      if (g_settings.shotgun_auto_shot && triggerbot_shotgun && isPressed(109)) {
          trigger_ready = true;
      }
      else {
          trigger_ready = false;
      }
      if (zoom_state > 0) { //根据是否开镜选择不同的自瞄范围
        aimbot.max_fov = g_settings.ads_fov;
      } else {
        aimbot.max_fov = g_settings.non_ads_fov;
      }

      // Trigger ring check on F8 key press for over 0.5 seconds
      static std::chrono::steady_clock::time_point tduckStartTime;
      static bool mapRadarTestingEnabled = false;
      if (g_settings.map_radar_testing && isPressed(99)) { // KEY_F8
        if (mapRadarTestingEnabled) {
          MapRadarTesting();
        }

        if (tduckStartTime == std::chrono::steady_clock::time_point()) {
          tduckStartTime = std::chrono::steady_clock::now();
        }

        auto currentTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                            currentTime - tduckStartTime)
                            .count();
        if (duration >= 250) {
          mapRadarTestingEnabled = true;
        }
      } else {
        tduckStartTime = std::chrono::steady_clock::time_point();
        mapRadarTestingEnabled = false;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  cactions_t = false;
}

void ControlLoop() {//根据观战人数闪烁键盘背光
  control_t = true;
  while (control_t) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    spectatorsMtx.lock();
    int spec_count = spectators.size();
    spectatorsMtx.unlock();
    if (spec_count > 0) {
      kbd_backlight_blink(spec_count);
      std::this_thread::sleep_for(std::chrono::milliseconds(10 * 1000 - 100));
    }
  }
  control_t = false;
}

//位于ProcessPlayer
void SetPlayerGlow(Entity &LPlayer, Entity &Target, int index,
                   int frame_number) {
  const auto g_settings = global_settings();
  //int context_id = 0;
  int setting_index = 0;
  std::array<float, 3> highlight_parameter = {0, 0, 0};

  if (!Target.isGlowing() ||
      (int)Target.buffer[OFFSET_GLOW_THROUGH_WALLS_GLOW_VISIBLE_TYPE] != 1) {
    float currentEntityTime = 5000.f;
    if (!isnan(currentEntityTime) && currentEntityTime > 0.f) {
      // set glow color
      if (!(g_settings.firing_range) &&
          (Target.isKnocked() || !Target.isAlive())) {  //不在训练场并且倒地或者没活着
        setting_index = 80;
        highlight_parameter = {g_settings.glow_r_knocked,
                               g_settings.glow_g_knocked,
                               g_settings.glow_b_knocked};
      } else if (Target.lastVisTime() > lastvis_aim[index] ||
                 (Target.lastVisTime() < 0.f && lastvis_aim[index] > 0.f)) {
        setting_index = 81;
        highlight_parameter = {g_settings.glow_r_viz, g_settings.glow_g_viz,
                               g_settings.glow_b_viz};
      } else {
        if (g_settings.player_glow_armor_color) {
          int shield = Target.getShield();
          int health = Target.getHealth();
          if (shield + health <= 100) { // Orange
            setting_index = 91;
            highlight_parameter = {255 / 255.0, 165 / 255.0, 0 / 255.0};
          } else if (shield + health <= 150) { // white
            setting_index = 92;
            highlight_parameter = {247 / 255.0, 247 / 255.0, 247 / 255.0};
          } else if (shield + health <= 175) { // blue
            setting_index = 93;
            highlight_parameter = {39 / 255.0, 178 / 255.0, 255 / 255.0};
          } else if (shield + health <= 200) { // purple
            setting_index = 94;
            highlight_parameter = {206 / 255.0, 59 / 255.0, 255 / 255.0};
          } else if (shield + health <= 225) { // red
            setting_index = 95;
            highlight_parameter = {219 / 255.0, 2 / 255.0, 2 / 255.0};
          } else {
            setting_index = 90;
            highlight_parameter = {2 / 255.0, 2 / 255.0, 2 / 255.0};
          }
        } else {
          setting_index = 82;
          highlight_parameter = {g_settings.glow_r_not, g_settings.glow_g_not,
                                 g_settings.glow_b_not};
        }
      }
      // love player glow
      if (g_settings.player_glow_love_user) {
        auto it = centity_to_index.find(Target.ptr);
        if (it != centity_to_index.end() &&
            Target.check_love_player(it->second)) {
          int frame_frag = frame_number / ((int)g_settings.game_fps);
          if (setting_index == 81 ||
              frame_frag % 2 == 0) { // vis: always, else: 1s time slice
            setting_index = 96;
            rainbowColor(frame_number, highlight_parameter);    //返回一个rgb色彩到highlight_parameter
          }
        }
      }

      // enable glow
      if (g_settings.player_glow) {     //如果设置里开了发光，就执行发光
        Target.enableGlow(setting_index, g_settings.player_glow_inside_value,
            g_settings.player_glow_outline_size, highlight_parameter);
      }
      if (!g_settings.player_glow) {      //如果设置里关闭了发光，并且玩家仍在发光，就将发光效果取消掉
        Target.enableGlow(setting_index, 0, 0, highlight_parameter);
      }
    }
  }
}

//位于DoAction
void ProcessPlayer(Entity &LPlayer, Entity &target, uint64_t entitylist,
                   int index, int frame_number, std::set<uintptr_t> &tmp_specs) {

  const auto g_settings = global_settings();

  int entity_team = target.getTeamId();

  if (!target.isAlive() || !LPlayer.isAlive()) {
    // Update yew to spec checker
    tick_yew(target.ptr, target.GetYaw());
    // Exclude self from list when watching others
    if (target.ptr != LPlayer.ptr && is_spec(target.ptr)) {
      tmp_specs.insert(target.ptr);
    }
    // if (target.ptr != LPlayer.ptr && LPlayer.GetYaw() == target.GetYaw()) {
    // // check yew
    //   tmp_specs.insert(target.ptr);
    // }
    return;
  }

  if (g_settings.tdm_toggle) { // Check if the target entity is on the same
                               // team as the
                               // local player
    // int entity_team = Target.getTeamId();
    // printf("Target Team: %i\n", entity_team);

    uint64_t PlayerLocal;
    apex_mem.Read<uint64_t>(g_Base + OFFSET_LOCAL_ENT, PlayerLocal);
    int PlayerLocalTeamID;
    apex_mem.Read<int>(PlayerLocal + OFFSET_TEAM, PlayerLocalTeamID);

    if (entity_team % 2)
      EntTeam = 1;
    else
      EntTeam = 2;
    if (PlayerLocalTeamID % 2)
      LocTeam = 1;
    else
      LocTeam = 2;

    // printf("Target Team: %i\nLocal Team: %i\n", EntTeam, LocTeam);
    if (EntTeam == LocTeam)
      return;
  }

  // Firing range stuff
  if (!g_settings.firing_range) {
    if (entity_team < 0 || entity_team > 50 ||
        (entity_team == team_player && !g_settings.onevone)) {
      return;
    }
    if (map_testing_local_team != 0 && entity_team == map_testing_local_team) {
      return;
    }
  }

  Vector EntityPosition = target.getPosition();
  Vector LocalPlayerPosition = LPlayer.getPosition();
  float dist = LocalPlayerPosition.DistTo(EntityPosition);

  // aim distance check
  if ((local_held_id == -251 && dist > g_settings.skynade_dist) ||
      dist > g_settings.aim_dist)
    return;

  // Targeting
  const float vis_weights = 12.5f;
  float fov = CalculateFov(LPlayer, target);
  bool vis = target.lastVisTime() > lastvis_aim[index];
  float score =
      (fov * fov) * 100 + (dist * 0.025) * 10 + (vis ? 0 : vis_weights);
  /*
   fov:dist:score
    1  10m  100
    2  40m  400
    3  90m  900
    4  160m 1600
  */
  if (score < aimbot.target_score_max) {
    aimbot.target_score_max = score;
    aimbot.tmp_aimentity = target.ptr;
  }

  if (g_settings.aim == 2) {
    // vis check for shooting current aim entity
    if (local_held_id != -251 && aimbot.aimentity == target.ptr) {
      if (!vis) {
        // turn on safety
        aimbot.gun_safety = true;
      } else {
        aimbot.gun_safety = false;
      }
    }

    // TriggerBot
    if (aimbot.aimentity != 0) {
      uint64_t LocalPlayer = 0;
      apex_mem.Read<uint64_t>(g_Base + OFFSET_LOCAL_ENT, LocalPlayer);

      Entity Target = getEntity(aimbot.aimentity);
      // Entity LPlayer = getEntity(LocalPlayer);


      if (trigger_ready && IsInCrossHair(Target)) {
        TriggerBotRun();
      }
    }
  }
  SetPlayerGlow(LPlayer, target, index, frame_number);
  lastvis_aim[index] = target.lastVisTime();
}

// Main stuff, dont edit.
void DoActions() {
  actions_t = true;
  while (actions_t) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    while (g_Base != 0) {   //读到游戏基址后开始循环
      const auto g_settings = global_settings();

      char MapName[200] = {0};
      uint64_t MapName_ptr;
      apex_mem.Read<uint64_t>(g_Base + OFFSET_HOST_MAP, MapName_ptr);   //根据偏移读取当前地图名
      apex_mem.ReadArray<char>(MapName_ptr, MapName, 200);

      // printf("%s\n", MapName);
      if (strcmp(MapName, "mp_rr_tropic_island_mu1_storm") == 0) {
        map = 1;
      } else if (strcmp(MapName, "mp_rr_canyonlands_mu") == 0) {
        map = 2;
      } else if (strcmp(MapName, "mp_rr_desertlands_hu") == 0) {
        map = 3;
      } else if (strcmp(MapName, "mp_rr_olympus") == 0) {
        map = 4;
      } else if (strcmp(MapName, "mp_rr_divided_moon") == 0) {
        map = 5;
      } else {
        map = 0;
      }

      if (g_settings.firing_range) {    //判断是否在射击场,从而区分应该循环的次数
        playerentcount = 16000;
      } else {
        playerentcount = 61;
      }
      if (g_settings.deathbox) {    //如果开了死亡之箱高亮则需要更多物品循环
        itementcount = 15000;
      } else {
        itementcount = 10000;
      }
      std::this_thread::sleep_for(
          std::chrono::milliseconds(30)); // don't change xD

      uint64_t LocalPlayer = 0;
      apex_mem.Read<uint64_t>(g_Base + OFFSET_LOCAL_ENT, LocalPlayer);  //读取本地玩家的所在地址,即是当前视角的玩家，自己或者你死亡后观战的人
      if (LocalPlayer == 0)
        continue;

      Entity LPlayer = getEntity(LocalPlayer);  //根据地址生成玩家实体对象entity

      team_player = LPlayer.getTeamId();    //获取自己所在队伍的id
      if (team_player < 0 || team_player > 50) {    //id不对开始新的while循环不继续执行
        continue;   
      }
      uint64_t entitylist = g_Base + OFFSET_ENTITYLIST;

      uint64_t baseent = 0;
      apex_mem.Read<uint64_t>(entitylist, baseent); //Check base entity is not Null
      if (baseent == 0) {
        continue;
      }

      {
        static uintptr_t lplayer_ptr = 0;
        if (lplayer_ptr != LPlayer.ptr) {   //如果LPlayer.ptr不为0，即前面读取都是顺利的，则将本地玩家指针传给lplayer_ptr
          lplayer_ptr = LPlayer.ptr;
          init_spec_checker(lplayer_ptr);   //函数定义在rust里，\apex_dma\apexsky\src\lib.rs  \apex_dma\apexsky\src\skyapex\spectators.rs
        }
        tick_yew(lplayer_ptr, LPlayer.GetYaw());
      }

      int frame_number = 0;
      apex_mem.Read<int>(g_Base + OFFSET_GLOBAL_VARS + 0x0008, frame_number);       //读取游戏的实际帧率
      std::set<uintptr_t> tmp_specs;
      aimbot.target_score_max =
          (50 * 50) * 100 + (g_settings.aim_dist * 0.025) * 10; //自瞄距离设200*40, 2500+2000?
      aimbot.tmp_aimentity = 0;
      centity_to_index.clear();
      //tmp_specs.clear();
      if (g_settings.firing_range) {
        int c = 0;
        for (int i = 0; i < playerentcount; i++) {
          uint64_t centity = 0;
          apex_mem.Read<uint64_t>(entitylist + ((uint64_t)i << 5), centity);
          if (centity == 0)
            continue;
          centity_to_index.insert_or_assign(centity, i);
          if (LocalPlayer == centity) {
            continue;
          }

          Entity Target = getEntity(centity);
          if (!Target.isDummy() && !g_settings.onevone) {
            continue;
          }

          ProcessPlayer(LPlayer, Target, entitylist, c, frame_number, tmp_specs);
          c++;
        }
      } else {

        for (int i = 0; i < toRead; i++) {
          uint64_t centity = 0;
          apex_mem.Read<uint64_t>(entitylist + ((uint64_t)i << 5), centity);
          if (centity == 0)
            continue;
          centity_to_index.insert_or_assign(centity, i);

          if (LocalPlayer == centity)
            continue;
          Entity Target = getEntity(centity);
          if (!Target.isPlayer()) {
            continue;
          }

          ProcessPlayer(LPlayer, Target, entitylist, i, frame_number, tmp_specs);
        }
      }

      { // refresh spectators count
        // static uint32_t counter = 0;
        // static std::map<uintptr_t, size_t> specs_test;
        // if (counter < 10) {
        //   auto tmp = tmp_specs;
        //   for (auto it = specs_test.begin(); it != specs_test.end(); it++) {
        //     if (tmp.extract(it->first).empty()) {
        //       // it->second -= 1;
        //     } else {
        //       it->second += 1;
        //     }
        //   }
        //   for (auto it = tmp.begin(); it != tmp.end(); it++) {
        //     assert(!specs_test.contains(*it));
        //     specs_test[*it] = 1;
        //   }
        // } else {
        // std::vector<Entity> tmp_spec, tmp_all_spec;
        // for (auto it = specs_test.begin(); it != specs_test.end(); it++) {
        //   // if (it->second > counter / 2) {
        //     Entity target = getEntity(it->first);
        //     if (target.getTeamId() == team_player) {
        //       tmp_all_spec.push_back(target);
        //     } else {
        //       tmp_spec.push_back(target);
        //     }
        //   // }
        // }
        // spectators = tmp_spec;
        // allied_spectators = tmp_all_spec;

        // specs_test.clear();
        //  counter = 0;
        // }
        // counter++;

          { // refresh spectators count

              std::vector<Entity> tmp_spec, tmp_all_spec;
              spectatorsMtx.lock();
              for (auto it = tmp_specs.begin(); it != tmp_specs.end(); it++) {
                  Entity target = getEntity(*it);
                  if (target.getTeamId() == team_player) {
                      tmp_all_spec.push_back(target);
                  }
                  else {
                      tmp_spec.push_back(target);
                  }
              }
              spectators.clear();
              allied_spectators.clear();
              spectators = tmp_spec;
              allied_spectators = tmp_all_spec;
              spectatorsMtx.unlock();
          }
      }
      // set current aim entity
      if (aimbot.lock) { // locked target
        aimbot.aimentity = aimbot.locked_aimentity;
      } else { // or new target
        aimbot.aimentity = aimbot.tmp_aimentity;
      }
      // disable aimbot safety if vis check is turned off
      if (g_settings.aim == 1 && local_held_id != -251) {
        aimbot.gun_safety = false;
      }
      
      // weapon model glow
      // printf("%d\n", LPlayer.getHealth());
      if (g_settings.weapon_model_glow && LPlayer.getHealth() > 0) {
        std::array<float, 3> highlight_color;
        bool weapon_glow = false;
        int spectators_num = spectators.size();
        if (spectators_num > 6) {
            rainbowColor(frame_number, highlight_color);    //大于6人观战彩色
            weapon_glow = true;
        }
        else if (spectators_num > 4) {
            highlight_color = { 1, 0, 0 }; //大于4人红色
            weapon_glow = true;
        }
        else if (spectators_num > 2) {
            highlight_color = { 1, 0.6, 0 };   //3-4人橙色
            weapon_glow = true;
        }
        else if (spectators_num > 0) {
            highlight_color = { 0, 0.4, 1 };  //1-2人蓝色
            weapon_glow = true;
        }
        else if (allied_spectators.size() > 0) {    //没有敌人但是队友观战绿色
          highlight_color = {0, 1, 0};
          weapon_glow = true;
        } 
        else {
            weapon_glow = false;            //没人观战不发光
        }
        LPlayer.glow_weapon_model(g_Base, weapon_glow, highlight_color);
      } else {
        LPlayer.glow_weapon_model(g_Base, false, {0, 0, 0});
      }
    }
  }
  actions_t = false;
}

// /////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<player> players(toRead);
Matrix view_matrix_data = {};

// ESP loop.. this helps right?
static void EspLoop() {
  esp_t = true;
  while (esp_t) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    while (g_Base != 0 && overlay_t) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
      const auto g_settings = global_settings();

      if (g_settings.esp) {
        valid = false;

        uint64_t LocalPlayer = 0;
        apex_mem.Read<uint64_t>(g_Base + OFFSET_LOCAL_ENT, LocalPlayer);
        if (LocalPlayer == 0) {
          next2 = true;
          while (next2 && g_Base != 0 && overlay_t && g_settings.esp) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
          }
          continue;
        }
        Entity LPlayer = getEntity(LocalPlayer);
        int team_player = LPlayer.getTeamId();
        if (team_player < 0 || team_player > 50) {
          next2 = true;
          while (next2 && g_Base != 0 && overlay_t && g_settings.esp) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
          }
          continue;
        }
        Vector LocalPlayerPosition = LPlayer.getPosition();
        esp_local_pos = LocalPlayerPosition;

        uint64_t viewRenderer = 0;
        apex_mem.Read<uint64_t>(g_Base + OFFSET_RENDER, viewRenderer);
        uint64_t viewMatrix = 0;
        apex_mem.Read<uint64_t>(viewRenderer + OFFSET_MATRIX, viewMatrix);

        apex_mem.Read<Matrix>(viewMatrix, view_matrix_data);

        uint64_t entitylist = g_Base + OFFSET_ENTITYLIST;

        players.clear();

        {
          Vector LocalPlayerPosition = LPlayer.getPosition();
          QAngle localviewangle = LPlayer.GetViewAngles();

          // Ammount of ents to loop, dont edit.
          for (int i = 0; i < toRead; i++) {
            // Read entity pointer
            uint64_t centity = 0;
            apex_mem.Read<uint64_t>(entitylist + ((uint64_t)i << 5), centity);
            if (centity == 0) {
              continue;
            }

            // Exclude self
            if (LocalPlayer == centity) {
              continue;
            }

            // Get entity data
            Entity Target = getEntity(centity);

            // Exclude undesired entity
            if (g_settings.firing_range) {
              if (!Target.isDummy() && !g_settings.onevone) {
                continue;
              }
            } else {
              if (!Target.isPlayer()) {
                continue;
              }
            }

            int entity_team = Target.getTeamId();

            // Exclude invalid team
            if (entity_team < 0 || entity_team > 50) {
              continue;
            }

            // Exlude teammates if not 1v1
            if (entity_team == team_player && !g_settings.onevone) {
              continue;
            }
            // if (map_testing_local_team != 0 &&
            //     entity_team == map_testing_local_team) {
            //   continue;
            // }

            Vector EntityPosition = Target.getPosition();
            float dist = LocalPlayerPosition.DistTo(EntityPosition);

            // Excluding targets that are too far or too close
            if (dist > g_settings.max_dist || dist < 20.0f) {
              continue;
            }

            Vector bs = Vector();
            // Change res to your res here, default is 1080p but can copy paste
            // 1440p here
            WorldToScreen(EntityPosition, view_matrix_data.matrix,
                          g_settings.screen_width, g_settings.screen_height,
                          bs); // 2560, 1440
            if (g_settings.esp) {
              Vector hs = Vector();
              Vector HeadPosition = Target.getBonePositionByHitbox(0);
              // Change res to your res here, default is 1080p but can copy
              // paste 1440p here
              WorldToScreen(HeadPosition, view_matrix_data.matrix,
                            g_settings.screen_width, g_settings.screen_height,
                            hs); // 2560, 1440
              float height = abs(abs(hs.y) - abs(bs.y));
              float width = height / 2.0f;
              float boxMiddle = bs.x - (width / 2.0f);
              int health = Target.getHealth();
              int shield = Target.getShield();
              int maxshield = Target.getMaxshield();
              int armortype = Target.getArmortype();
              Vector EntityPosition = Target.getPosition();
              float targetyaw = Target.GetYaw();
              uint64_t entity_index = i - 1;
              player data_buf = {dist,
                                 entity_team,
                                 boxMiddle,
                                 hs.y,
                                 width,
                                 height,
                                 bs.x,
                                 bs.y,
                                 Target.isKnocked(),
                                 (Target.lastVisTime() > lastvis_esp[i]),
                                 health,
                                 shield,
                                 maxshield,
                                 armortype,
                                 EntityPosition,
                                 LocalPlayerPosition,
                                 localviewangle,
                                 targetyaw,
                                 Target.isAlive(),
                                 Target.check_love_player(entity_index),
                                 false};
              Target.get_name(g_Base, entity_index, &data_buf.name[0]);
              spectatorsMtx.lock();
              for (auto &ent : spectators) {
                if (ent.ptr == centity) {
                  data_buf.is_spectator = true;
                  break;
                }
              }
              spectatorsMtx.unlock();
              players.push_back(data_buf);
              lastvis_esp[i] = Target.lastVisTime();
              valid = true;
            }
          }
        }

        next2 = true;
        while (next2 && g_Base != 0 && overlay_t && g_settings.esp) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      }
    }
  }
  esp_t = false;
}

// Aimbot Loop stuff
inline static void lock_target(uintptr_t target_ptr) {  //锁定目标函数，aimbot是包含自瞄信息的结构体
  aimbot.lock = true;
  aimbot.locked_aimentity = target_ptr;
}
inline static void cancel_targeting() { //取消锁定
  aimbot.lock = false;
  aimbot.locked_aimentity = 0;
}
static void AimbotLoop() {
  aim_t = true;
  while (aim_t) {
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    while (g_Base != 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      const auto g_settings = global_settings();

      // Read LocalPlayer
      uint64_t LocalPlayer = 0;
      apex_mem.Read<uint64_t>(g_Base + OFFSET_LOCAL_ENT, LocalPlayer);

      // Read HeldID
      int HeldID;
      apex_mem.Read<int>(LocalPlayer + OFFSET_OFF_WEAPON, HeldID); // 0x1a1c
      local_held_id = HeldID;   //读取本地玩家手持物品id赋值给local_held_id

      // Read WeaponID
      ulong ehWeaponHandle;
      apex_mem.Read<uint64_t>(LocalPlayer + OFFSET_ACTIVE_WEAPON,ehWeaponHandle);
      ehWeaponHandle &= 0xFFFF;                // eHandle
      ulong pWeapon;
      uint64_t entitylist = g_Base + OFFSET_ENTITYLIST;
      apex_mem.Read<uint64_t>(entitylist + (ehWeaponHandle * 0x20), pWeapon);
      uint32_t weaponID;
      apex_mem.Read<uint32_t>(pWeapon + OFFSET_WEAPON_NAME,weaponID); // 0x1738
      local_weapon_id = weaponID;
      // printf("%d\n", weaponID);
      
      if (g_settings.aim > 0) {     //0为不自喵，1为不检查可见性，2为检查目标可见性
        if (aimbot.aimentity == 0) {    //如果无目标取消锁定
          cancel_targeting();
          continue;
        }

        Entity target = getEntity(aimbot.aimentity);
        // show target indicator before aiming
        aim_target = target.getPosition();  //获取目标的位置

        if (!aimbot.aiming) {   //aimbot的元素值由DoAction和ClientAction函数修改
          cancel_targeting();
          continue;
        }

        lock_target(aimbot.aimentity);
        if (aimbot.gun_safety) {    //gun_safety用于可见性检查
          continue;
        }

        Entity LPlayer = getEntity(LocalPlayer);
        if (LocalPlayer == 0) {
          continue;
        }
        if (LPlayer.isKnocked()) {
          cancel_targeting();
          continue;
        }

        /* Fine-tuning for each weapon */
        // bow
        if (weaponID == 2) {
          // Ctx.BulletSpeed = BulletSpeed - (BulletSpeed*0.08);
          // Ctx.BulletGravity = BulletGrav + (BulletGrav*0.05);
          bulletspeed = 10.08;
          bulletgrav = 10.05;
        }

        if (HeldID == -251) { // auto throw
          QAngle Angles_g = CalculateBestBoneAim(LPlayer, target, 999.9f, aimbot.smooth);
          if (Angles_g.x == 0 && Angles_g.y == 0) {
            cancel_targeting();
            continue;
          }
          LPlayer.SetViewAngles(Angles_g);

        } else {
          QAngle Angles = CalculateBestBoneAim(LPlayer, target, aimbot.max_fov, aimbot.smooth);
          if (Angles.x == 0 && Angles.y == 0) {
            cancel_targeting();
            continue;
          }
          LPlayer.SetViewAngles(Angles);
        }
      }
    }
  }
  aim_t = false;
}
// Item Glow Stuff

static void item_glow_t() {
  item_t = true;
  while (item_t) {
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    while (g_Base != 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(60));
      const auto g_settings = global_settings();
      if (!g_settings.item_glow) {
        break;
      }

      uint64_t entitylist = g_Base + OFFSET_ENTITYLIST;
      // item ENTs to loop, 10k-15k is normal. 10k might be better but will
      // not show all the death boxes i think.

      // for wish list
      std::vector<TreasureClue> new_treasure_clues;//定义了一个容器，用于存储 TreasureClue 结构体的实例（就是各种物品），TreasureClue包含itemid\position\distance三个元素，
      for (size_t i = 0; i < wish_list.size(); i++) {
        TreasureClue clue;
        clue.item_id = wish_list[i];
        clue.position = Vector(0, 0, 0);
        clue.distance = g_settings.aim_dist * 2;
        new_treasure_clues.push_back(clue);
      }//可能是初始化10个类型的对照组物品？不是很明白

      for (int i = 0; i < itementcount; i++) {//开启10000个物品循环
        uint64_t centity = 0;
        apex_mem.Read<uint64_t>(entitylist + ((uint64_t)i << 5), centity);//每个实体是32位数据,所以每次+32，读取结果保存到centity
        if (centity == 0)
          continue;
        Item item = getItem(centity);//读取到的是这个实体的数组地址，使用getitem获取到这个数组，此时item包含了ptr（数组指针）和buffer（包含实体所有数据）两个属性，其它还未读取

        // Item filter glow name setup and search.
        char glowName[200] = {0};
        uint64_t name_ptr;
        apex_mem.Read<uint64_t>(centity + OFFSET_MODELNAME, name_ptr);//这个实体的数组指针再加上名称偏移量，得到实体名称数组的地址
        apex_mem.ReadArray<char>(name_ptr, glowName, 200);//将实体名称存到glowName

        // item ids?
        uint64_t ItemID;
        apex_mem.Read<uint64_t>(centity + OFFSET_ITEM_ID, ItemID);
        /* uint64_t ItemID2;
        ItemID2 = ItemID % 301;
        printf("%ld\n", ItemID2); */
        // printf("Model Name: %s, Item ID: %lu\n", glowName, ItemID);
        // Level name printf
        // char LevelNAME[200] = { 0 };
        // uint64_t levelname_ptr;
        // apex_mem.Read<uint64_t>(g_Base + OFFSET_LEVELNAME, levelname_ptr);
        // apex_mem.ReadArray<char>(levelname_ptr, LevelNAME, 200);

        // printf("%s\n", LevelNAME);

        // Prints stuff you want to console
        // if (strstr(glowName, "mdl/"))
        //{
        // printf("%ld\n", ItemID);
        // }
        // Search model name and if true sets glow, must be a better way to do
        // this.. if only i got the item id to work..
        /*单显卡用不到暂时注释掉
        for (size_t i = 0; i < new_treasure_clues.size(); i++) {
          TreasureClue &clue = new_treasure_clues[i];   //将new_treasure_clues[i]赋值给clue，后续可以使用clue指代new_treasure_clues[i]（或许是这样）
          if (ItemID == new_treasure_clues[i].item_id) {    //如果循环到的实体的ItemID在10个之中（wish为自定义的愿望清单，用于esp显示）
            Vector position = item.getPosition();    //获取这个实体的坐标
            float distance = esp_local_pos.DistTo(position);
            if (distance < clue.distance) { //如果实体距离小于自瞄距离的2倍，将坐标和距离更新到clue中
              clue.position = position;
              clue.distance = distance;//clue貌似只在客户端覆盖中使用
            }
            break;
          }
        }   
        */
        if (g_settings.loot.lightbackpack && ItemID == 207) {       //白包
            std::array<float, 3> highlightParameter = { 1, 1, 1 };  //高亮颜色，111是白色，因为lightbackpack是白包
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.medbackpack && ItemID == 208) {     //蓝包
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.heavybackpack && ItemID == 209) {   //紫包
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };  //#4B0082
            int settingIndex = 74;
            item.enableGlow(settingIndex, 64, highlightParameter);
        }
        else if (g_settings.loot.goldbackpack && ItemID == 210) {     //金包
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 64, highlightParameter);
        }
        // item id would help so much here, cant make them all the same color
        // so went with loba glow for body shield and helmet
        else if (g_settings.loot.shieldupgrade1 &&
            (ItemID == 214748364993 || ItemID == 14073963583897798)) {  //白甲
            std::array<float, 3> highlightParameter = { 1, 1, 1 };
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shieldupgrade2 &&
            (ItemID == 322122547394 || ItemID == 21110945375846599)) {  //蓝甲
            std::array<float, 3> highlightParameter = { 0, 0, 1 };

            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shieldupgrade3 &&
            (ItemID == 429496729795 || ItemID == 52776987629977800)) {      //紫甲
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };

            int settingIndex = 74;
            item.enableGlow(settingIndex, 64, highlightParameter);
        }
        else if (g_settings.loot.shieldupgrade4 && (ItemID == 429496729796)) {   //金甲
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shieldupgrade5 && ItemID == 536870912201) { //红甲
            std::array<float, 3> highlightParameter = { 1, 0, 0 };

            int settingIndex = 67;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shieldupgradehead1 && ItemID == 188) {  //白头
            std::array<float, 3> highlightParameter = { 1, 1, 1 };

            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shieldupgradehead2 && ItemID == 189) {  //蓝头

            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shieldupgradehead3 && ItemID == 190) {  //紫头
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };

            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shieldupgradehead4 && ItemID == 191) {      //金头

            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 64, highlightParameter);
        }
        else if (g_settings.loot.accelerant && ItemID == 182) {      //绝招加速剂
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.phoenix && ItemID == 183) {     //凤凰
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.skull &&
            strstr(glowName,
                "mdl/Weapons/skull_grenade/skull_grenade_base_v.rmdl")) {    //可能没逆向出头骨id，直接对比物品名称
            std::array<float, 3> highlightParameter = { 1, 0, 0 };
            int settingIndex = 67;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.deathbox && item.isBox()) {      //添加死亡之箱的开关判断
            std::array<float, 3> highlightParameter = { 1, 0, 0 };
            int settingIndex = 88;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }

        else if (item.isTrap()) {        //判断名称是否是侵蚀陷阱？毒气罐？好像无效
            std::array<float, 3> highlightParameter = { 0, 1, 0 };
            int settingIndex = 67;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }

        // Gas Trap
        else if (strstr(glowName,
            "mdl/props/caustic_gas_tank/caustic_gas_tank.rmdl")) {   //这里也无效
            std::array<float, 3> highlightParameter = { 0, 1, 0 };
            int settingIndex = 67;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.healthlarge && ItemID == 184) {     //大药包
            std::array<float, 3> highlightParameter = { 1, 1, 1 };
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.healthsmall && ItemID == 185) {     //小药
            std::array<float, 3> highlightParameter = { 1, 1, 1 };
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shieldbattsmall && ItemID == 187) {     //小电
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shieldbattlarge && ItemID == 186) {     //大电
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 48, highlightParameter);
        }
        else if (g_settings.loot.sniperammo && ItemID == 144) {      //狙击弹药
            std::array<float, 3> highlightParameter = { 0.2431, 0.2078, 0.6741 };   //紫色
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.heavyammo && ItemID == 143) {       //重型弹药
            std::array<float, 3> highlightParameter = { 0.2667, 0.5333, 0.4353 };   //改成墨绿色
            int settingIndex = 65;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.optic1xhcog && ItemID == 215) {     //1倍镜
            std::array<float, 3> highlightParameter = { 1, 1, 1 };
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.lightammo && ItemID == 140) {       //轻型子弹
            std::array<float, 3> highlightParameter = { 0.6902, 0.60, 0.3098 }; //土黄
            int settingIndex = 66;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.energyammo && ItemID == 141) {      //能量子弹
            std::array<float, 3> highlightParameter = { 0.2, 1, 0 };        //翠绿
            int settingIndex = 73;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shotgunammo && ItemID == 142) { //霰弹子弹
            std::array<float, 3> highlightParameter = { 0.5, 0.0862, 0 };   //暗红
            int settingIndex = 67;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.lasersight1 && ItemID == 229) { //激光瞄准器，以下3个等级
            std::array<float, 3> highlightParameter = { 1, 1, 1 };
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.lasersight2 && ItemID == 230) {
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.lasersight3 && ItemID == 231) {
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.sniperammomag1 && ItemID == 244) {  //狙击弹匣，以下四个等级
            std::array<float, 3> highlightParameter = { 1, 1, 1 };
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.sniperammomag2 && ItemID == 245) {
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.sniperammomag3 && ItemID == 246) {
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.sniperammomag4 && ItemID == 247) {
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 64, highlightParameter);
        }
        else if (g_settings.loot.energyammomag1 && ItemID == 240) {  //能量弹匣，四个等级
            std::array<float, 3> highlightParameter = { 1, 1, 1 };
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.energyammomag2 && ItemID == 241) {
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.energyammomag3 && ItemID == 242) {
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.energyammomag4 && ItemID == 243) {
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 64, highlightParameter);
        }
        else if (g_settings.loot.stocksniper1 && ItemID == 255) {    //狙击枪托
            std::array<float, 3> highlightParameter = { 1, 1, 1 };
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.stocksniper2 && ItemID == 256) {
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.stocksniper3 && ItemID == 257) {
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.stockregular1 && ItemID == 252) {   //标准枪托
            std::array<float, 3> highlightParameter = { 1, 1, 1 };
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.stockregular2 && ItemID == 253) {
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.stockregular3 && ItemID == 254) {
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        /*取消对白击倒护盾的判断
        if (g_settings.loot.shielddown1 && ItemID == 203) {     //击倒护盾
          std::array<unsigned char, 4> highlightFunctionBits = {
              g_settings
                  .loot_filled, // InsideFunction  HIGHLIGHT_FILL_LOOT_SCANNED
              125,              // OutlineFunction OutlineFunction
                                // HIGHLIGHT_OUTLINE_LOOT_SCANNED
              64, 64};
          std::array<float, 3> highlightParameter = {1, 1, 1};
          apex_mem.Write<uint32_t>(centity + OFFSET_GLOW_THROUGH_WALLS, 2);
          static const int contextId = 0;
          int settingIndex = 72;
          apex_mem.Write<unsigned char>(
              centity + OFFSET_HIGHLIGHTSERVERACTIVESTATES + contextId,
              settingIndex);
          long highlightSettingsPtr;
          apex_mem.Read<long>(g_Base + HIGHLIGHT_SETTINGS,
                              highlightSettingsPtr);
          apex_mem.Write<typeof(highlightFunctionBits)>(
              highlightSettingsPtr + 40 * settingIndex + 4,
              highlightFunctionBits);
          apex_mem.Write<typeof(highlightParameter)>(
              highlightSettingsPtr + 40 * settingIndex + 8, highlightParameter);
        }
        */
        else if (g_settings.loot.shielddown2 && ItemID == 204) {
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shielddown3 && ItemID == 205) {
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shielddown4 && ItemID == 206) {
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 64, highlightParameter);
        }
        else if (g_settings.loot.lightammomag1 && ItemID == 232) {   //轻型弹匣
            std::array<float, 3> highlightParameter = { 1, 1, 1 };
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.lightammomag2 && ItemID == 233) {
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.lightammomag3 && ItemID == 234) {
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.lightammomag4 && ItemID == 235) {
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 64, highlightParameter);
        }
        else if (g_settings.loot.heavyammomag1 && ItemID == 236) {   //重型弹匣
            std::array<float, 3> highlightParameter = { 1, 1, 1 };
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.heavyammomag2 && ItemID == 237) {
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.heavyammomag3 && ItemID == 238) {
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.heavyammomag4 && ItemID == 239) {
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 64, highlightParameter);
        }
        else if (g_settings.loot.optic2xhcog && ItemID == 216) {     //2倍镜
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 48, highlightParameter);
        }
        else if (g_settings.loot.opticholo1x && ItemID == 217) { //圆形1倍镜
            std::array<float, 3> highlightParameter = { 1, 1, 1 };
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.opticholo1x2x && ItemID == 218) {       //1x2x切换
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.opticthreat && ItemID == 219) {     //金1倍
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 64, highlightParameter);
        }
        else if (g_settings.loot.optic3xhcog && ItemID == 220) {     //3倍镜
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.optic2x4x && ItemID == 221) {       //2x4x
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.opticsniper6x && ItemID == 222) {       //6倍镜
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.opticsniper4x8x && ItemID == 223) {     //4x8x
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.opticsniperthreat && ItemID == 224) {       //金狙击镜
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 64, highlightParameter);
        }
        else if (g_settings.loot.suppressor1 && ItemID == 225) {     //枪管？
            std::array<float, 3> highlightParameter = { 1, 1, 1 };
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.suppressor2 && ItemID == 226) {
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.suppressor3 && ItemID == 227) {
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.turbo_charger && ItemID == 258) {       //涡轮增压器
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 48, highlightParameter);
        }
        else if (g_settings.loot.skull_piecer && ItemID == 260) {        //穿颅器
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 48, highlightParameter);
        }
        else if (g_settings.loot.hammer_point && ItemID == 263) {        //锤击点
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 48, highlightParameter);
        }
        else if (g_settings.loot.disruptor_rounds && ItemID == 262) {    //干扰器
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 48, highlightParameter);
        }
        else if (g_settings.loot.boosted_loader && ItemID == 272) {      //加速装填器
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 48, highlightParameter);
        }
        else if (g_settings.loot.shotgunbolt1 && ItemID == 248) {        //霰弹枪栓
            std::array<float, 3> highlightParameter = { 1, 1, 1 };
            int settingIndex = 72;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shotgunbolt2 && ItemID == 249) {
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shotgunbolt3 && ItemID == 250) {
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 74;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.shotgunbolt4 && ItemID == 251) {
            std::array<float, 3> highlightParameter = { 1, 0.8431, 0 };
            int settingIndex = 75;
            item.enableGlow(settingIndex, 48, highlightParameter);
        }
        // Nades
        else if (g_settings.loot.grenade_frag && ItemID == 213) {    //破片手雷
            std::array<float, 3> highlightParameter = { 1, 0, 0 };
            int settingIndex = 67;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }

        else if (g_settings.loot.grenade_thermite && ItemID == 212) {    //铝热剂
            std::array<float, 3> highlightParameter = { 1, 0, 0 };
            int settingIndex = 67;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.grenade_arc_star && ItemID == 214) {        //电弧星
            std::array<float, 3> highlightParameter = { 0, 0, 1 };
            int settingIndex = 70;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        // Weapons
        else if (g_settings.loot.weapon_kraber && ItemID == 1) {     //克莱伯
            std::array<float, 3> highlightParameter = { 1, 0, 0 };
            int settingIndex = 67;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_mastiff && ItemID == 3) {        //敖犬
            std::array<float, 3> highlightParameter = { 1, 0, 0 };
            int settingIndex = 67;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_lstar && ItemID == 7) {      //lstar

            std::array<float, 3> highlightParameter = { 0.2, 1, 0 };
            int settingIndex = 73;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        // new gun, nemesis
        else if (g_settings.loot.weapon_nemesis && ItemID == 135) {      //复仇女神
            std::array<float, 3> highlightParameter = { 0.2, 1, 0 };
            int settingIndex = 73;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }

        else if (g_settings.loot.weapon_havoc && ItemID == 13) {     //哈沃克
            std::array<float, 3> highlightParameter = { 0.2, 1, 0 };
            int settingIndex = 73;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_devotion && ItemID == 18) {  //专注轻机枪
            std::array<float, 3> highlightParameter = { 0.2, 1, 0 };
            int settingIndex = 73;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_triple_take && ItemID == 23) {   //三重狙击枪
            std::array<float, 3> highlightParameter = { 0.2, 1, 0 };
            int settingIndex = 73;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_flatline && ItemID == 28) {      //平行
            std::array<float, 3> highlightParameter = { 0, 1, 1 };
            int settingIndex = 65;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_hemlock && ItemID == 33) {       //汉姆洛克
            std::array<float, 3> highlightParameter = { 0, 1, 1 };
            int settingIndex = 65;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_g7_scout && ItemID == 39) {      //g7
            std::array<float, 3> highlightParameter = { 1, 0.5490, 0 };
            int settingIndex = 66;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_alternator && ItemID == 44) {        //转换者
            std::array<float, 3> highlightParameter = { 1, 0.5490, 0 };
            int settingIndex = 66;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_r99 && ItemID == 49) {
            std::array<float, 3> highlightParameter = { 1, 0.5490, 0 };
            int settingIndex = 66;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_prowler && ItemID == 56) {   //猎兽
            std::array<float, 3> highlightParameter = { 0, 1, 1 };
            int settingIndex = 65;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_volt && ItemID == 60) {  //电能冲锋枪
            std::array<float, 3> highlightParameter = { 0.2, 1, 0 };
            int settingIndex = 73;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_longbow && ItemID == 65) {       //长弓
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_charge_rifle && ItemID == 70) {  //充能步枪
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_spitfire && ItemID == 75) {  //喷火
            std::array<float, 3> highlightParameter = { 1, 0.5490, 0 };
            int settingIndex = 66;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_r301 && ItemID == 80) {
            std::array<float, 3> highlightParameter = { 1, 0.5490, 0 };
            int settingIndex = 66;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_eva8 && ItemID == 85) {
            std::array<float, 3> highlightParameter = { 0.5, 0.0862, 0 };
            int settingIndex = 67;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_peacekeeper && ItemID == 90) {
            std::array<float, 3> highlightParameter = { 0.5, 0.0862, 0 };
            int settingIndex = 67;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_mozambique && ItemID == 95) {
            std::array<float, 3> highlightParameter = { 0.5, 0.0862, 0 };
            int settingIndex = 67;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_wingman && ItemID == 106) {      //小帮手
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_p2020 && ItemID == 111) {
            std::array<float, 3> highlightParameter = { 1, 0.5490, 0 };
            int settingIndex = 66;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_re45 && ItemID == 116) {
            std::array<float, 3> highlightParameter = { 1, 0.5490, 0 };
            int settingIndex = 66;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_sentinel && ItemID == 122) { //哨兵
            std::array<float, 3> highlightParameter = { 0.2941, 0, 0.5098 };
            int settingIndex = 69;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_bow && ItemID == 127) {
            std::array<float, 3> highlightParameter = { 1, 0, 0 };
            int settingIndex = 67;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_3030_repeater && ItemID == 129) {
            std::array<float, 3> highlightParameter = { 0, 1, 1 };
            int settingIndex = 65;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_rampage && ItemID == 146) {      //暴走
            std::array<float, 3> highlightParameter = { 0, 1, 1 };
            int settingIndex = 65;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        else if (g_settings.loot.weapon_car_smg && ItemID == 151) {
            std::array<float, 3> highlightParameter = { 0, 1, 1 };
            int settingIndex = 65;
            item.enableGlow(settingIndex, 32, highlightParameter);
        }
        // CREDITS to Rikkie
        // https://www.unknowncheats.me/forum/members/169606.html for all the
        // weapon ids and item ids code, you are a life saver!

      } // for(item) loop end
      treasure_clues = new_treasure_clues;
    } // while(item_glow) loop end
  }   // while(item_t) loop end
  item_t = false;
}

extern void start_overlay();

void terminal() {
  terminal_t = true;
  run_tui_menu();
}

int main(int argc, char *argv[]) {
  load_settings();

  if (geteuid() != 0) {
    // run as root..
    print_run_as_root();

    // test menu
    run_tui_menu();
    return 0;
  }

  const char *ap_proc = "r5apex.exe";

  std::thread aimbot_thr;
  std::thread esp_thr;
  std::thread actions_thr;
  std::thread cactions_thr;
  // Used to change things on a timer
  // std::thread updateInsideValue_thr;
  std::thread TriggerBotRun_thr;
  std::thread terminal_thr;
  std::thread overlay_thr;
  std::thread itemglow_thr;
  std::thread control_thr;

  if (apex_mem.open_os() != 0) {
    exit(0);
  }

  while (active) {
    if (apex_mem.get_proc_status() != process_status::FOUND_READY) {
      if (aim_t) {
        aim_t = false;
        esp_t = false;
        actions_t = false;
        cactions_t = false;
        // Used to change things on a timer
        updateInsideValue_t = false;
        TriggerBotRun_t = false;
        terminal_t = false;
        overlay_t = false;
        item_t = false;
        control_t = false;
        g_Base = 0;
        quit_tui_menu();

        aimbot_thr.~thread();
        esp_thr.~thread();
        actions_thr.~thread();
        cactions_thr.~thread();
        // Used to change things on a timer
        // updateInsideValue_thr.~thread();
        TriggerBotRun_thr.~thread();
        terminal_thr.~thread();
        overlay_thr.~thread();
        itemglow_thr.~thread();
        control_thr.~thread();
      }

      std::this_thread::sleep_for(std::chrono::seconds(1));
      printf("Searching for apex process...\n");

      apex_mem.open_proc(ap_proc);

      if (apex_mem.get_proc_status() == process_status::FOUND_READY) {
        g_Base = apex_mem.get_proc_baseaddr();
        printf("\nApex process found\n");
        printf("Base: %lx\n", g_Base);

        aimbot_thr = std::thread(AimbotLoop);
        esp_thr = std::thread(EspLoop);
        actions_thr = std::thread(DoActions);
        cactions_thr = std::thread(ClientActions);
        // Used to change things on a timer
        // updateInsideValue_thr = std::thread(updateInsideValue);
        TriggerBotRun_thr = std::thread(TriggerBotRun);
        itemglow_thr = std::thread(item_glow_t);
        control_thr = std::thread(ControlLoop);
        aimbot_thr.detach();
        esp_thr.detach();
        actions_thr.detach();
        cactions_thr.detach();
        // Used to change things on a timer
        // updateInsideValue_thr.detach();
        TriggerBotRun_thr.detach();
        itemglow_thr.detach();
        control_thr.detach();
      }
    } else {
      apex_mem.check_proc();

      const auto g_settings = global_settings();
      if (g_settings.debug_mode) {
        if (terminal_t) {
          quit_tui_menu();
        }
      } else {
        if (!terminal_t) {
          terminal_thr = std::thread(terminal);
          terminal_thr.detach();
        }
        // wish_list.clear();
      }
      if (g_settings.no_overlay) {
        if (overlay_t) {
          overlay_t = false;
        }
      } else {
        if (!overlay_t) {
          overlay_thr = std::thread(start_overlay);
          overlay_thr.detach();
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return 0;
}
