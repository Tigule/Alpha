#include "Frame/CSimpleMessageFrame.h"

#include <lauxlib.h>
#include <lua.h>

static int __fastcall CSimpleMessageFrame_AddMessage(lua_State *L) {
  CSimpleMessageFrame *frame = 0;
  if (lua_type(L, 1) == LUA_TTABLE) {
    lua_rawgeti(L, 1, 0);
    frame = static_cast<CSimpleMessageFrame *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
  } else {
    luaL_error(L, "Attempt to find 'this' in non-table object (used '.' instead of ':' ?)");
  }
  ASSERT(frame);

  if (lua_isstring(L, 2)) {
    const char *message = lua_tostring(L, 2);
    if (message && *message) {
      NTempest::CImVector color(0xFFFFFFFFul);
      float               time = 0.0f;
      int                 permanent = 0;

      if (lua_isnumber(L, 3) && lua_isnumber(L, 4) && lua_isnumber(L, 5)) {
        float red = static_cast<float>(lua_tonumber(L, 3));
        float green = static_cast<float>(lua_tonumber(L, 4));
        float blue = static_cast<float>(lua_tonumber(L, 5));
        float alpha = 1.0f;

        if (lua_isnumber(L, 6)) {
          alpha = static_cast<float>(lua_tonumber(L, 6));
        }

        color.Set(alpha, red, green, blue);
        if (lua_isnumber(L, 7)) {
          time = static_cast<float>(lua_tonumber(L, 7));
          if (lua_isnumber(L, 8)) {
            permanent = static_cast<int>(lua_tonumber(L, 8)) > 0;
          }
        }
      }

      frame->AddMessage(message, color, time, permanent);
    }
  }

  return 0;
}

static FrameScript_Method SimpleMessageFrameMethods[1] = {
    {"AddMessage", CSimpleMessageFrame_AddMessage}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CSimpleMessageFrame::s_scriptMethods;

void __fastcall CSimpleMessageFrame::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(SimpleMessageFrameMethods, 1, s_scriptMethods);
}

void __fastcall CSimpleMessageFrame::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

int CSimpleMessageFrame::LookupScriptMethod(lua_State *L, const char *name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }

  return CSimpleFrame::LookupScriptMethod(L, name);
}
